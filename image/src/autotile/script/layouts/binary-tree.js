// The binary tree layout (see script/main.js for what a layout implements).
// CMakeLists.txt puts every file of this folder before script/main.js in the
// script's one file: top-level names are shared, so keep them unique.

const MIN_RATIO = 0.1;
const MAX_RATIO = 0.9;

// Every split divides its area in two along the longer side. A new window
// splits the focused one. Which side is the longer is worked out again when a
// window enters or leaves, not on a resize: dragging a split must not turn the
// splits next to it.
function createBinaryTree() {
    let root = null;

    function leaves(node) {
        return node.id !== undefined ? [node] : [...leaves(node.first), ...leaves(node.second)];
    }

    function find(id) {
        return root ? leaves(root).find((leaf) => leaf.id === id) : undefined;
    }

    function forgetSides(node) {
        if (node.id === undefined) {
            node.sideBySide = undefined;
            forgetSides(node.first);
            forgetSides(node.second);
        }
    }

    function layOut(node, rect, result) {
        node.rect = rect;
        if (node.id !== undefined) {
            result.push({ id: node.id, rect });
            return;
        }
        if (node.sideBySide === undefined) {
            node.sideBySide = rect.width >= rect.height;
        }
        const axis = node.sideBySide ? ["x", "width"] : ["y", "height"];
        const size = Math.round(rect[axis[1]] * node.ratio);
        // No object spread: KWin's JavaScript engine doesn't parse it.
        const first = Object.assign({}, rect, { [axis[1]]: size });
        const second = Object.assign({}, rect, { [axis[0]]: rect[axis[0]] + size, [axis[1]]: rect[axis[1]] - size });
        layOut(node.first, first, result);
        layOut(node.second, second, result);
    }

    // The nearest split that has this edge of the leaf as its dividing line.
    function splitAt(leaf, sideBySide, leafIsFirst) {
        let child = leaf;
        for (let node = leaf.parent; node; child = node, node = node.parent) {
            if (node.sideBySide === sideBySide && (node.first === child) === leafIsFirst) {
                return node;
            }
        }
        return null;
    }

    // Moves a split's dividing line to position, along the axis.
    function moveSplit(node, position, start, size) {
        if (node) {
            const ratio = (position - node.rect[start]) / node.rect[size];
            node.ratio = Math.min(MAX_RATIO, Math.max(MIN_RATIO, ratio));
        }
    }

    return {
        insert(id, focusedId) {
            const leaf = { id, parent: null };
            if (!root) {
                root = leaf;
                return;
            }
            const all = leaves(root);
            const target = find(focusedId) || all[all.length - 1];
            const split = { first: target, second: leaf, ratio: 0.5, parent: target.parent };
            if (!target.parent) {
                root = split;
            } else if (target.parent.first === target) {
                target.parent.first = split;
            } else {
                target.parent.second = split;
            }
            target.parent = split;
            leaf.parent = split;
            forgetSides(root);
        },

        // The sibling takes the parent's place.
        remove(id) {
            const leaf = find(id);
            if (!leaf) {
                return;
            }
            const parent = leaf.parent;
            if (!parent) {
                root = null;
                return;
            }
            const sibling = parent.first === leaf ? parent.second : parent.first;
            sibling.parent = parent.parent;
            if (!parent.parent) {
                root = sibling;
            } else if (parent.parent.first === parent) {
                parent.parent.first = sibling;
            } else {
                parent.parent.second = sibling;
            }
            forgetSides(root);
        },

        arrange(area) {
            const result = [];
            if (root) {
                layOut(root, area, result);
            }
            return result;
        },

        // Only the edges that moved count; an edge on the area's border has
        // no split behind it and is put back by the next arrange.
        resize(id, rect) {
            const leaf = find(id);
            if (!leaf || !leaf.rect) {
                return;
            }
            const old = leaf.rect;
            if (rect.x !== old.x) {
                moveSplit(splitAt(leaf, true, false), rect.x, "x", "width");
            }
            if (rect.x + rect.width !== old.x + old.width) {
                moveSplit(splitAt(leaf, true, true), rect.x + rect.width, "x", "width");
            }
            if (rect.y !== old.y) {
                moveSplit(splitAt(leaf, false, false), rect.y, "y", "height");
            }
            if (rect.y + rect.height !== old.y + old.height) {
                moveSplit(splitAt(leaf, false, true), rect.y + rect.height, "y", "height");
            }
        },

        // The places stay, splits included; only who is in them changes.
        swap(id, otherId) {
            const leaf = find(id);
            const other = find(otherId);
            if (leaf && other) {
                leaf.id = otherId;
                other.id = id;
            }
        },

        isEmpty() {
            return root === null;
        },
    };
}
