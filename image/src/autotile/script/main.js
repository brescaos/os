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
// How long a new window stays hidden at most, if it never takes its new size.
const REVEAL_TIMEOUT_MS = 300;
// Bresca Welcome always floats: a first-login assistant, not a window to work in.
const WELCOME = "com.brescaos.welcome";

// One layout per virtual desktop and screen: key -> { layout, output, desktop }.
const groups = new Map();
// Windows in a layout: window id -> { window, key, rect, shown, frame }. rect is
// its place in the layout, gap included; shown, that place after making room for
// the window in use; frame, the window's frame in it.
const tiled = new Map();
// Windows taken out of the layout by hand, by window id.
const floating = new Set();
// Windows being dragged: window id -> their frame when the drag began. They
// aren't placed or moved between layouts until it ends.
const dragged = new Map();
// The window a new one splits: the last focused one that is in a layout.
let lastFocusedId = null;
// The tiled window that has the room it needs while in use (withRoomForInUse).
let inUseId = null;
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
// window next to it, none off the screen's edges.
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

// The tiled window in use: the active one, or the one it's a dialog of.
function inUse() {
    for (let window = workspace.activeWindow; window; window = window.transientFor) {
        if (tiled.has(idOf(window))) {
            return idOf(window);
        }
    }
    return null;
}

// One axis of the area, its dividing lines moved so the span start..end takes size:
// the rest shrinks evenly on both sides of it, the area's own edges staying put. All
// the windows go through the same lines, so they stay side by side without overlaps.
function stretchedAxis(start, end, size, areaStart, areaEnd) {
    const rest = areaEnd - areaStart - (end - start);
    const scale = rest > 0 ? (areaEnd - areaStart - size) / rest : 0;
    const newStart = areaStart + (start - areaStart) * scale;
    const newEnd = newStart + size;
    return (position) => {
        if (position <= start) {
            return Math.round(areaStart + (position - areaStart) * scale);
        }
        if (position >= end) {
            return Math.round(newEnd + (position - end) * scale);
        }
        return Math.round(newStart + ((position - start) * size) / (end - start));
    };
}

// The layout's places, with the window in use given room for its minimum size, which
// may be more than its place has. Only while it's in use: the layout doesn't change.
function withRoomForInUse(arranged, area) {
    const id = inUse();
    const place = arranged.find((item) => item.id === id);
    if (!place) {
        return arranged;
    }
    const { minSize } = tiled.get(id).window;
    const frame = withGap(place.rect, area);
    const needed = (axis, minimum, areaSize) => Math.max(place.rect[axis],
        Math.min(areaSize, minimum + place.rect[axis] - frame[axis]));
    const width = needed("width", minSize.width, area.width);
    const height = needed("height", minSize.height, area.height);
    if (width === place.rect.width && height === place.rect.height) {
        return arranged;
    }
    const mapX = stretchedAxis(place.rect.x, place.rect.x + place.rect.width, width, area.x, area.x + area.width);
    const mapY = stretchedAxis(place.rect.y, place.rect.y + place.rect.height, height, area.y, area.y + area.height);
    return arranged.map((item) => {
        const x = mapX(item.rect.x);
        const y = mapY(item.rect.y);
        return {
            id: item.id,
            rect: {
                x,
                y,
                width: mapX(item.rect.x + item.rect.width) - x,
                height: mapY(item.rect.y + item.rect.height) - y,
            },
        };
    });
}

// The focus moved: the window it left gives the room back, the new one takes it.
function focusChanged() {
    const id = inUse();
    if (id === inUseId) {
        return;
    }
    const keys = [inUseId, id].map((windowId) => tiled.get(windowId)).filter(Boolean).map((entry) => entry.key);
    inUseId = id;
    new Set(keys).forEach(retile);
}

function retile(key) {
    const group = groups.get(key);
    if (!group) {
        return;
    }
    const area = workspace.clientArea(KWin.MaximizeArea, group.output, group.desktop);
    const whole = { x: area.x, y: area.y, width: area.width, height: area.height };
    const arranged = group.layout.arrange(whole);
    const shown = new Map(withRoomForInUse(arranged, whole).map(({ id, rect }) => [id, rect]));
    for (const { id, rect } of arranged) {
        const entry = tiled.get(id);
        if (!entry) {
            continue;
        }
        entry.rect = rect;
        entry.shown = shown.get(id);
        entry.frame = withGap(entry.shown, whole);
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
        const { x, y, width, height } = other.shown;
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
    focusChanged();
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

function toggleFloating() {
    const window = workspace.activeWindow;
    if (!window) {
        return;
    }
    const id = idOf(window);
    if (floating.has(id)) {
        floating.delete(id);
    } else {
        floating.add(id);
    }
    update(window);
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
