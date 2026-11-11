// The grid layout (see script/main.js for what a layout implements).
// CMakeLists.txt puts every file of this folder before script/main.js in the
// script's one file: top-level names are shared, so keep them unique.

// For the layouts that keep their windows in a list: grid's and master's.
function swapInList(ids, id, otherId) {
    const at = ids.indexOf(id);
    const otherAt = ids.indexOf(otherId);
    if (at >= 0 && otherAt >= 0) {
        ids[at] = otherId;
        ids[otherAt] = id;
    }
}

// As square as it can be, rows from top to bottom, every row filling the width: a
// last row with fewer windows shares it among them. The same size for all, so a
// resize changes nothing. A new window goes last.
function createGrid() {
    let ids = [];

    return {
        insert(id) {
            ids.push(id);
        },

        remove(id) {
            ids = ids.filter((other) => other !== id);
        },

        arrange(area) {
            const columns = Math.ceil(Math.sqrt(ids.length));
            const rows = Math.ceil(ids.length / (columns || 1));
            return ids.map((id, i) => {
                const row = Math.floor(i / columns);
                const inRow = row === rows - 1 ? ids.length - columns * (rows - 1) : columns;
                const column = i - row * columns;
                const x = area.x + Math.round((area.width * column) / inRow);
                const y = area.y + Math.round((area.height * row) / rows);
                return {
                    id,
                    rect: {
                        x,
                        y,
                        width: area.x + Math.round((area.width * (column + 1)) / inRow) - x,
                        height: area.y + Math.round((area.height * (row + 1)) / rows) - y,
                    },
                };
            });
        },

        resize() {},

        swap(id, otherId) {
            swapInList(ids, id, otherId);
        },

        isEmpty() {
            return ids.length === 0;
        },
    };
}
