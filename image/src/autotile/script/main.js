// Autotiling: KWin places windows in a layout on their own. Turned on and off
// by the KCM (ui/), the tray applet (applet/) and this script's checkbox in
// System Settings > KWin Scripts; when it's unloaded, windows stay where they
// are.
//
// The gap between windows and the layout are in its settings group in kwinrc
// (Gap, Layout): the KCM sets both, the tray applet the layout. They save it and
// have KWin reconfigure, and the script follows what's saved.
//
// KWin loads one file and scripts can't include others, so the build puts
// layouts/*.js before this file in contents/code/main.js (CMakeLists.txt).
//
// Layouts know nothing of KWin, only ids and rectangles, so a new one is a file
// in layouts/ and one more entry in LAYOUTS. Each implements:
//   insert(id, focusedId)  add a window next to the focused one
//   remove(id)             take a window out
//   arrange(area)          [{ id, rect }], every window's place in the area
//   resize(id, rect)       a window was resized to rect: adapt the layout
//   swap(id, otherId)      two windows trade places
//   isEmpty()

// AutotileLayout::layouts() lists them too, in this order, for the tray applet
// and the KCM (tests/qml/tst_script.qml).
const LAYOUTS = { binaryTree: createBinaryTree, master: createMaster, grid: createGrid };
const DEFAULT_LAYOUT = "binaryTree";
const DEFAULT_GAP = 8;
// Kept free along the screen's free area: KWin draws the line around the
// window with the focus (Hexa's window decoration) 2 px outside it, which
// would fall off the screen or under a panel. Its width is
// WINDOW_BORDER_WIDTH in image/src/hexa/decoration/decoration.cpp: change
// both together.
const EDGE_MARGIN = 2;
// How long a new window stays hidden at most, if it never takes its new size.
const REVEAL_TIMEOUT_MS = 300;
// Bresca Welcome always floats: a first-login assistant, not a window to work in.
const WELCOME = "com.brescaos.welcome";

// One layout per virtual desktop and screen: key -> { layout, output, desktop }.
const groups = new Map();
// Windows in a layout: window id -> { window, key, rect, frame }. rect is its
// place in the layout, gap included; frame, the window's frame in it.
const tiled = new Map();
// Windows taken out of the layout by hand, by window id.
const floating = new Set();
// Windows being dragged: window id -> their frame when the drag began. They
// aren't placed or moved between layouts until it ends.
const dragged = new Map();
// The window a new one splits: the last focused one that is in a layout.
let lastFocusedId = null;
// New windows hidden until they take their size: window id -> { timer, reveal }.
const hidden = new Map();
let gap = readConfig("Gap", DEFAULT_GAP);
// For every desktop and screen: edge snapping, which it decides, is KWin's alone.
let currentLayout = DEFAULT_LAYOUT;

function idOf(window) {
    return String(window.internalId);
}

// Windows hidden from both the task bar and Alt+Tab can't be reached by the
// user, and some are never seen at all (xwaylandvideobridge calls itself a normal
// window, with opacity 0): they'd keep an empty place in the layout.
function isEligible(window) {
    return window.desktopFileName !== WELCOME && window.normalWindow && !window.transient && !window.specialWindow
        && !(window.skipTaskbar && window.skipSwitcher)
        && window.moveable && window.resizeable
        && !window.minimized && !window.fullScreen && window.maximizeMode === 0
        && !window.onAllDesktops && window.desktops.length === 1
        && !floating.has(idOf(window));
}

function keyOf(window) {
    return `${window.desktops[0].id}|${window.output.name}`;
}

// The window's frame in its place: half the gap off each side that has another
// window next to it, none off the tiling area's edges.
function withGap(rect, area) {
    const left = rect.x > area.x ? Math.ceil(gap / 2) : 0;
    const top = rect.y > area.y ? Math.ceil(gap / 2) : 0;
    const right = rect.x + rect.width < area.x + area.width ? Math.floor(gap / 2) : 0;
    const bottom = rect.y + rect.height < area.y + area.height ? Math.floor(gap / 2) : 0;
    return {
        x: rect.x + left,
        y: rect.y + top,
        width: Math.max(1, rect.width - left - right),
        height: Math.max(1, rect.height - top - bottom),
    };
}

// Where windows go on output's desktop: the screen's free area, EDGE_MARGIN in.
function tilingArea(output, desktop) {
    const area = workspace.clientArea(KWin.MaximizeArea, output, desktop);
    return {
        x: area.x + EDGE_MARGIN,
        y: area.y + EDGE_MARGIN,
        width: area.width - 2 * EDGE_MARGIN,
        height: area.height - 2 * EDGE_MARGIN,
    };
}

function retile(key) {
    const group = groups.get(key);
    if (!group) {
        return;
    }
    const whole = tilingArea(group.output, group.desktop);
    for (const { id, rect } of group.layout.arrange(whole)) {
        const entry = tiled.get(id);
        if (!entry) {
            continue;
        }
        entry.rect = rect;
        entry.frame = withGap(rect, whole);
        if (!dragged.has(id)) {
            entry.window.frameGeometry = entry.frame;
        }
    }
}

function untile(id) {
    const entry = tiled.get(id);
    if (!entry) {
        return null;
    }
    tiled.delete(id);
    const group = groups.get(entry.key);
    group.layout.remove(id);
    if (group.layout.isEmpty()) {
        groups.delete(entry.key);
    }
    return entry.key;
}

function tile(window) {
    const key = keyOf(window);
    if (!groups.has(key)) {
        groups.set(key, {
            layout: LAYOUTS[currentLayout](),
            output: window.output,
            desktop: window.desktops[0],
        });
    }
    const id = idOf(window);
    groups.get(key).layout.insert(id, lastFocusedId);
    tiled.set(id, { window, key });
    return key;
}

// Puts a window where its state says it belongs, and rearranges what changed.
function update(window) {
    const id = idOf(window);
    if (dragged.has(id)) {
        return;
    }
    const entry = tiled.get(id);
    const wanted = isEligible(window) ? keyOf(window) : null;
    if ((entry ? entry.key : null) === wanted) {
        return;
    }
    retile(untile(id));
    if (wanted) {
        retile(tile(window));
    }
}

function dragStarted(window) {
    const { x, y, width, height } = window.frameGeometry;
    dragged.set(idOf(window), { x, y, width, height });
}

// The other window of its layout whose place holds the point, if any.
function tiledAt(point, entry, id) {
    for (const [otherId, other] of tiled) {
        const { x, y, width, height } = other.rect;
        if (otherId !== id && other.key === entry.key
            && point.x >= x && point.x < x + width && point.y >= y && point.y < y + height) {
            return otherId;
        }
    }
    return null;
}

// A frame of a different size was resized, and the split follows it. One of the
// same size that moved trades places with the window it was dropped on.
// Either way, the window then goes back to its place in the layout.
function dragFinished(window) {
    const id = idOf(window);
    const before = dragged.get(id);
    dragged.delete(id);
    const entry = tiled.get(id);
    if (entry && before) {
        const { x, y, width, height } = window.frameGeometry;
        if (width !== before.width || height !== before.height) {
            // The layout knows nothing of gaps: what each edge moved, on its place.
            const left = entry.rect.x + x - before.x;
            const top = entry.rect.y + y - before.y;
            const right = entry.rect.x + entry.rect.width + x + width - before.x - before.width;
            const bottom = entry.rect.y + entry.rect.height + y + height - before.y - before.height;
            groups.get(entry.key).layout.resize(id, { x: left, y: top, width: right - left, height: bottom - top });
        } else if (x !== before.x || y !== before.y) {
            const otherId = tiledAt(workspace.cursorPos, entry, id);
            if (otherId) {
                groups.get(entry.key).layout.swap(id, otherId);
            }
        }
    }
    update(window);
    const current = tiled.get(id);
    if (current) {
        retile(current.key);
    }
}

function retileAll() {
    for (const key of groups.keys()) {
        retile(key);
    }
}

// Dropping a window on a screen edge would maximize it or give it half or a
// quarter of the screen, against the layout. Only KWin's running options change,
// not the user's settings: KWin reads them back on every reconfigure, which what
// turns the script off (AutotileLayout) also asks for.
function disableEdgeSnapping() {
    options.electricBorderMaximize = false;
    options.electricBorderTiling = false;
}

// A layout that isn't known, from a hand-edited kwinrc, is the default.
function savedLayout() {
    const name = readConfig("Layout", DEFAULT_LAYOUT);
    return name in LAYOUTS ? name : DEFAULT_LAYOUT;
}

// For placing every window at once: the active one first, then from the most to
// the least recently used. With no focused window to split, each new one splits
// the last, so the first gets the biggest place and they get smaller from there.
function byUse() {
    const windows = Array.from(workspace.stackingOrder).reverse();
    const active = workspace.activeWindow;
    if (!active) {
        return windows;
    }
    return windows.filter((window) => idOf(window) === idOf(active))
        .concat(windows.filter((window) => idOf(window) !== idOf(active)));
}

function windowFocused(window) {
    if (window && tiled.has(idOf(window))) {
        lastFocusedId = idOf(window);
    }
}

function setLayout(name) {
    currentLayout = name;
    // All out first, without arranging what's left: windows must stay put.
    Array.from(tiled.keys()).forEach(untile);
    lastFocusedId = null;
    byUse().forEach(update);
    windowFocused(workspace.activeWindow);
}

function manage(window) {
    for (const signal of ["desktopsChanged", "outputChanged", "minimizedChanged", "fullScreenChanged", "maximizedChanged"]) {
        window[signal].connect(() => update(window));
    }
    window.interactiveMoveResizeStarted.connect(() => dragStarted(window));
    window.interactiveMoveResizeFinished.connect(() => dragFinished(window));
    update(window);
}

// KWin only tells a script about a window once the app has drawn it, at the size
// it chose. The window is told its new size, and stays hidden until it has drawn
// that one, so its first size is never seen; the others have already made room.
function windowOpened(window) {
    const { width, height } = window.frameGeometry;
    const id = idOf(window);
    manage(window);
    const entry = tiled.get(id);
    if (!entry || !entry.frame || (entry.frame.width === width && entry.frame.height === height)) {
        return;
    }
    const opacity = window.opacity;
    window.opacity = 0;
    const timer = new QTimer();
    const reveal = () => {
        timer.stop();
        window.frameGeometryChanged.disconnect(reveal);
        hidden.delete(id);
        window.opacity = opacity;
    };
    hidden.set(id, { timer, reveal });
    window.frameGeometryChanged.connect(reveal);
    timer.singleShot = true;
    timer.interval = REVEAL_TIMEOUT_MS;
    timer.timeout.connect(reveal);
    timer.start();
}

function windowGone(window) {
    const id = idOf(window);
    if (hidden.has(id)) {
        hidden.get(id).timer.stop();
        hidden.delete(id);
    }
    floating.delete(id);
    dragged.delete(id);
    if (lastFocusedId === id) {
        lastFocusedId = null;
    }
    retile(untile(id));
}

// A window that didn't fit its place grows to its minimum size, towards the
// tiling area where it would leave it: from a bottom right place, up and to
// the left. minSize is the app's, without the title bar and borders KWin adds.
function growToMinimumSize(window) {
    const frame = window.frameGeometry;
    const client = window.clientGeometry;
    const width = Math.max(frame.width, window.minSize.width + frame.width - client.width);
    const height = Math.max(frame.height, window.minSize.height + frame.height - client.height);
    if (width === frame.width && height === frame.height) {
        return;
    }
    const area = tilingArea(window.output, window.desktops[0]);
    const inArea = (start, size, areaStart, areaSize) =>
        Math.max(areaStart, Math.min(start, areaStart + areaSize - size));
    window.frameGeometry = {
        x: inArea(frame.x, width, area.x, area.width),
        y: inArea(frame.y, height, area.y, area.height),
        width,
        height,
    };
}

// Floated, a window keeps its size, grown to its minimum if its place was smaller;
// tiled again, it takes its place.
function toggleFloating() {
    const window = workspace.activeWindow;
    if (!window) {
        return;
    }
    const id = idOf(window);
    if (floating.has(id)) {
        floating.delete(id);
        update(window);
    } else {
        floating.add(id);
        update(window);
        growToMinimumSize(window);
    }
}

workspace.windowAdded.connect(windowOpened);
workspace.windowRemoved.connect(windowGone);
workspace.windowActivated.connect(windowFocused);
// Screens and panels changing what's free.
workspace.screensChanged.connect(retileAll);
workspace.virtualScreenGeometryChanged.connect(retileAll);
// KWin has just read its settings back, the user's edge snapping among them.
options.configChanged.connect(() => {
    gap = readConfig("Gap", DEFAULT_GAP);
    const saved = savedLayout();
    disableEdgeSnapping();
    if (saved !== currentLayout) {
        setLayout(saved);
    } else {
        retileAll();
    }
});
currentLayout = savedLayout();
disableEdgeSnapping();

registerShortcut("BrescaAutotileFloat", "Autotiling: float or tile the active window", "Meta+Shift+F", toggleFloating);

byUse().forEach(manage);
windowFocused(workspace.activeWindow);
