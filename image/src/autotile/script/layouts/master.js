// The master layout (see script/main.js for what a layout implements).
// CMakeLists.txt puts every file of this folder before script/main.js in the
// script's one file: top-level names are shared, so keep them unique.

// The first window on the left, the others stacked on the right, all of a height. A
// new window goes last, at the bottom of the stack. Resizing moves the line between
// the two sides; nothing else has one to move. MIN_RATIO and MAX_RATIO are
// binary-tree.js's, swapInList grid.js's.
function createMaster() {
    let ids = [];
    let ratio = 0.5;
    let lastArea = null;
    let places = new Map();

    return {
        insert(id) {
            ids.push(id);
        },

        remove(id) {
            ids = ids.filter((other) => other !== id);
        },

        arrange(area) {
            lastArea = area;
            const result = ids.map((id, i) => {
                if (ids.length === 1) {
                    return { id, rect: Object.assign({}, area) };
                }
                const masterWidth = Math.round(area.width * ratio);
                if (i === 0) {
                    return { id, rect: { x: area.x, y: area.y, width: masterWidth, height: area.height } };
                }
                const stacked = ids.length - 1;
                const y = area.y + Math.round((area.height * (i - 1)) / stacked);
                return {
                    id,
                    rect: {
                        x: area.x + masterWidth,
                        y,
                        width: area.width - masterWidth,
                        height: area.y + Math.round((area.height * i) / stacked) - y,
                    },
                };
            });
            places = new Map(result.map(({ id, rect }) => [id, rect]));
            return result;
        },

        resize(id, rect) {
            const old = places.get(id);
            if (!old || !lastArea || ids.length < 2) {
                return;
            }
            let line = null;
            if (id === ids[0] && rect.x + rect.width !== old.x + old.width) {
                line = rect.x + rect.width;
            } else if (id !== ids[0] && rect.x !== old.x) {
                line = rect.x;
            }
            if (line !== null) {
                const moved = (line - lastArea.x) / lastArea.width;
                ratio = Math.min(MAX_RATIO, Math.max(MIN_RATIO, moved));
            }
        },

        swap(id, otherId) {
            swapInList(ids, id, otherId);
        },

        isEmpty() {
            return ids.length === 0;
        },
    };
}
