// A stub of KWin's scripting API, to run the script (scriptSource, from
// scripttest.cpp) on a fake workspace. Each run is a function of its own, with
// KWin's globals as its arguments, so nothing is shared between two runs.
//
// No object spread: KWin's JavaScript engine, this one, doesn't parse it.

const AREA = { x: 60, y: 40, width: 1000, height: 600 };
const DESKTOP = { id: "desktop-1" };
const OUTPUT = { name: "Virtual-1" };

// Called, it fires. QML's own Function.prototype.connect and disconnect are
// read-only, so assigning them wouldn't take.
function signal() {
    const handlers = [];
    const fire = (...args) => handlers.forEach((handler) => handler(...args));
    Object.defineProperty(fire, "connect", { value: (handler) => handlers.push(handler) });
    Object.defineProperty(fire, "disconnect", {
        value: (handler) => {
            const at = handlers.indexOf(handler);
            if (at >= 0) {
                handlers.splice(at, 1);
            }
        },
    });
    return fire;
}

// FakeTimer.last is the last one started.
function FakeTimer() {
    this.timeout = signal();
    this.running = false;
}
FakeTimer.prototype.start = function () {
    this.running = true;
    FakeTimer.last = this;
};
FakeTimer.prototype.stop = function () {
    this.running = false;
};

function fakeWindow(id, overrides) {
    return Object.assign({
        internalId: id,
        normalWindow: true,
        transient: false,
        specialWindow: false,
        moveable: true,
        resizeable: true,
        minimized: false,
        fullScreen: false,
        maximizeMode: 0,
        onAllDesktops: false,
        skipTaskbar: false,
        skipSwitcher: false,
        desktops: [DESKTOP],
        output: OUTPUT,
        minSize: { width: 0, height: 0 },
        frameGeometry: { x: 0, y: 0, width: 300, height: 200 },
        desktopsChanged: signal(),
        outputChanged: signal(),
        minimizedChanged: signal(),
        fullScreenChanged: signal(),
        maximizedChanged: signal(),
        opacity: 1,
        frameGeometryChanged: signal(),
        interactiveMoveResizeStarted: signal(),
        interactiveMoveResizeFinished: signal(),
    }, overrides);
}

// Runs the script with these globals, then the code epilogue, whose return
// value it returns.
function run(source, globals, epilogue) {
    const names = Object.keys(globals);
    const script = new Function(...names, `${source}\n;${epilogue}`);
    return script(...names.map((name) => globals[name]));
}

// The script loaded with the windows existing open (from the most recently
// used) and the settings config, which a test may change before a reload.
function load(source, existing, config) {
    existing = existing || [];
    config = config || { Gap: 0 };
    const shortcuts = {};
    const options = { configChanged: signal(), electricBorderMaximize: true, electricBorderTiling: true };
    const workspace = {
        activeWindow: null,
        cursorPos: { x: -1, y: -1 },
        // KWin's goes from bottom to top.
        stackingOrder: existing.slice().reverse(),
        windowAdded: signal(),
        windowRemoved: signal(),
        windowActivated: signal(),
        screensChanged: signal(),
        virtualScreenGeometryChanged: signal(),
        clientArea: () => AREA,
    };
    run(source, {
        workspace,
        KWin: { MaximizeArea: 2 },
        QTimer: FakeTimer,
        options,
        readConfig: (key, fallback) => (key in config ? config[key] : fallback),
        registerShortcut: (name, text, keys, handler) => {
            shortcuts[keys] = handler;
        },
    }, "");
    return { workspace, shortcuts, options, config };
}

// The script's top-level names, from a run that has nothing to do.
function internals(source) {
    return run(source, {
        workspace: {
            stackingOrder: [],
            windowAdded: signal(),
            windowRemoved: signal(),
            windowActivated: signal(),
            screensChanged: signal(),
            virtualScreenGeometryChanged: signal(),
        },
        KWin: {},
        QTimer: FakeTimer,
        options: { configChanged: signal() },
        readConfig: (key, fallback) => fallback,
        registerShortcut: () => {},
    }, "return { LAYOUTS, DEFAULT_LAYOUT, DEFAULT_GAP, createBinaryTree, createMaster, createGrid };");
}

// A reconfigure: the user's own settings come back, then the script is told.
function reloadSettings(options) {
    options.electricBorderMaximize = true;
    options.electricBorderTiling = true;
    options.configChanged();
}

function activate(workspace, window) {
    workspace.activeWindow = window;
    workspace.windowActivated(window);
}

// Each window's place in the layout's arrangement of area, by id.
function rects(layout, area) {
    const byId = {};
    for (const { id, rect } of layout.arrange(area || AREA)) {
        byId[id] = { x: rect.x, y: rect.y, width: rect.width, height: rect.height };
    }
    return byId;
}

function totalArea(byId) {
    return Object.keys(byId).reduce((sum, id) => sum + byId[id].width * byId[id].height, 0);
}

function geometry(window) {
    const { x, y, width, height } = window.frameGeometry;
    return { x, y, width, height };
}

// rect with some of its fields changed.
function moved(rect, changes) {
    return Object.assign({}, rect, changes);
}
