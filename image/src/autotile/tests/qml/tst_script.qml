// The script: its layouts on their own, then the script driving a fake
// workspace (kwinstub.js), and what it shares with AutotileLayout.
//
// scriptSource and autotile* come from scripttest.cpp, which qmllint can't see.
// qmllint disable unqualified
import QtQuick
import QtTest

import "kwinstub.js" as KWin

TestCase {
    id: testCase

    name: "Script"

    readonly property var area: KWin.AREA
    // In a grid of 3, the last window's place: all the width, half the height.
    readonly property var bigPlace: ({ x: 60, y: 340, width: 1000, height: 300 })

    function load(existing, config) {
        return KWin.load(scriptSource, existing, config);
    }

    function layoutFrom(factory) {
        return KWin.internals(scriptSource)[factory]();
    }

    function filled(layout, ids) {
        ids.forEach(id => layout.insert(id, null));
        return layout;
    }

    function geometry(window) {
        return KWin.geometry(window);
    }

    function bigOnLastRow() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const big = KWin.fakeWindow("big", { minSize: { width: 700, height: 500 } });
        return Object.assign({ a, b, big }, load([a, b, big], { Gap: 0, Layout: "grid" }));
    }

    // The binary tree layout.

    function test_one_window_fills_the_area() {
        const layout = filled(layoutFrom("createBinaryTree"), ["a"]);
        compare(KWin.rects(layout), { a: area });
    }

    function test_a_new_window_splits_the_focused_one_along_its_longer_side() {
        const layout = layoutFrom("createBinaryTree");
        layout.insert("a", null);
        layout.insert("b", "a");
        layout.insert("c", "b");
        compare(KWin.rects(layout), {
            a: { x: 60, y: 40, width: 500, height: 600 },
            b: { x: 560, y: 40, width: 500, height: 300 },
            c: { x: 560, y: 340, width: 500, height: 300 },
        });
    }

    function test_without_a_focused_window_the_new_one_splits_the_last() {
        const layout = layoutFrom("createBinaryTree");
        layout.insert("a", null);
        layout.insert("b", "gone");
        compare(Object.keys(KWin.rects(layout)), ["a", "b"]);
    }

    function test_the_places_cover_the_whole_area_data() {
        return [
            { tag: "binary tree", factory: "createBinaryTree", count: 4 },
            { tag: "master", factory: "createMaster", count: 4 },
            { tag: "grid", factory: "createGrid", count: 5 },
        ];
    }

    function test_the_places_cover_the_whole_area(data) {
        const oddArea = { x: 7, y: 3, width: 1001, height: 601 };
        const layout = layoutFrom(data.factory);
        for (let i = 0; i < data.count; i++) {
            layout.insert(`w${i}`, i > 0 ? `w${i - 1}` : null);
        }
        compare(KWin.totalArea(KWin.rects(layout, oddArea)), oddArea.width * oddArea.height);
    }

    function test_when_a_window_goes_its_sibling_takes_its_place() {
        const layout = layoutFrom("createBinaryTree");
        layout.insert("a", null);
        layout.insert("b", "a");
        layout.insert("c", "b");
        layout.remove("c");
        compare(KWin.rects(layout), {
            a: { x: 60, y: 40, width: 500, height: 600 },
            b: { x: 560, y: 40, width: 500, height: 600 },
        });
        layout.remove("a");
        compare(KWin.rects(layout), { b: area });
        layout.remove("b");
        verify(layout.isEmpty());
    }

    function test_resizing_moves_the_split_that_has_that_edge_and_it_stays() {
        const layout = layoutFrom("createBinaryTree");
        layout.insert("a", null);
        layout.insert("b", "a");
        KWin.rects(layout);
        layout.resize("a", { x: 60, y: 40, width: 700, height: 600 });
        compare(KWin.rects(layout).a.width, 700);
        compare(KWin.rects(layout).b.x, 760);
        layout.insert("c", "b");
        compare(KWin.rects(layout).a.width, 700);
    }

    function test_resizing_a_window_from_its_left_edge_moves_the_split_on_its_left() {
        const layout = layoutFrom("createBinaryTree");
        layout.insert("a", null);
        layout.insert("b", "a");
        KWin.rects(layout);
        layout.resize("b", { x: 460, y: 40, width: 600, height: 600 });
        compare(KWin.rects(layout).a.width, 400);
    }

    function test_a_corner_of_a_window_moves_a_split_on_each_axis() {
        const layout = layoutFrom("createBinaryTree");
        layout.insert("a", null);
        layout.insert("b", "a");
        layout.insert("c", "b");
        KWin.rects(layout);
        layout.resize("b", { x: 460, y: 40, width: 600, height: 400 });
        const after = KWin.rects(layout);
        compare(after.a.width, 400);
        compare(after.b.height, 400);
        compare(after.c.y, 440);
    }

    function test_a_window_leaving_turns_the_split_left_behind_to_fit_its_new_area() {
        const layout = layoutFrom("createBinaryTree");
        layout.insert("a", null);
        layout.insert("b", "a");
        layout.insert("c", "b");
        KWin.rects(layout);
        layout.remove("a");
        compare(KWin.rects(layout), {
            b: { x: 60, y: 40, width: 500, height: 600 },
            c: { x: 560, y: 40, width: 500, height: 600 },
        });
    }

    function test_an_edge_on_the_area_border_changes_nothing() {
        const layout = filled(layoutFrom("createBinaryTree"), ["a", "b"]);
        const before = KWin.rects(layout);
        layout.resize("a", { x: 0, y: 40, width: 560, height: 600 });
        compare(KWin.rects(layout), before);
    }

    function test_a_split_cant_be_dragged_out_of_reach() {
        const layout = filled(layoutFrom("createBinaryTree"), ["a", "b"]);
        KWin.rects(layout);
        layout.resize("a", { x: 60, y: 40, width: 1, height: 600 });
        compare(KWin.rects(layout).a.width, 100);
    }

    function test_swapping_trades_the_windows_places_and_keeps_the_splits() {
        const layout = layoutFrom("createBinaryTree");
        layout.insert("a", null);
        layout.insert("b", "a");
        layout.insert("c", "b");
        layout.resize("a", { x: 60, y: 40, width: 700, height: 600 });
        const before = KWin.rects(layout);
        layout.swap("a", "c");
        compare(KWin.rects(layout), { a: before.c, b: before.b, c: before.a });
        layout.swap("a", "gone");
        compare(KWin.rects(layout), { a: before.c, b: before.b, c: before.a });
    }

    // The master layout.

    function test_master_one_window_fills_the_area() {
        compare(KWin.rects(filled(layoutFrom("createMaster"), ["a"])), { a: area });
    }

    function test_master_the_first_on_the_left_the_others_stacked_evenly() {
        compare(KWin.rects(filled(layoutFrom("createMaster"), ["a", "b", "c"])), {
            a: { x: 60, y: 40, width: 500, height: 600 },
            b: { x: 560, y: 40, width: 500, height: 300 },
            c: { x: 560, y: 340, width: 500, height: 300 },
        });
    }

    function test_master_a_new_window_goes_to_the_bottom_whichever_is_focused() {
        const layout = filled(layoutFrom("createMaster"), ["a", "b"]);
        layout.insert("c", "a");
        compare(KWin.rects(layout).c.y, 340);
        compare(KWin.rects(layout).a.width, 500);
    }

    function test_master_resizing_either_side_moves_the_line_within_reach() {
        const layout = filled(layoutFrom("createMaster"), ["a", "b", "c"]);
        layout.arrange(area);
        layout.resize("a", { x: 60, y: 40, width: 700, height: 600 });
        compare(KWin.rects(layout).a.width, 700);
        layout.resize("b", { x: 460, y: 40, width: 600, height: 300 });
        compare(KWin.rects(layout).a.width, 400);
        layout.resize("a", { x: 60, y: 40, width: 5, height: 600 });
        compare(KWin.rects(layout).a.width, 100);
    }

    function test_master_resizing_up_or_down_changes_nothing() {
        const layout = filled(layoutFrom("createMaster"), ["a", "b", "c"]);
        const before = KWin.rects(layout);
        layout.resize("b", { x: 560, y: 40, width: 500, height: 450 });
        compare(KWin.rects(layout), before);
    }

    function test_master_swapping_trades_places_and_removing_closes_the_gap() {
        const layout = filled(layoutFrom("createMaster"), ["a", "b", "c"]);
        const before = KWin.rects(layout);
        layout.swap("a", "c");
        compare(KWin.rects(layout), { c: before.a, b: before.b, a: before.c });
        layout.remove("c");
        compare(KWin.rects(layout).b.width, 500);
        compare(KWin.rects(layout).a.height, 600);
        layout.remove("a");
        layout.remove("b");
        verify(layout.isEmpty());
    }

    // The grid layout.

    function test_grid_rows_as_square_as_can_be_a_shorter_last_row_sharing_the_width() {
        compare(KWin.rects(filled(layoutFrom("createGrid"), ["a", "b", "c"])), {
            a: { x: 60, y: 40, width: 500, height: 300 },
            b: { x: 560, y: 40, width: 500, height: 300 },
            c: { x: 60, y: 340, width: 1000, height: 300 },
        });
    }

    function test_grid_windows_per_row_data() {
        return [
            { tag: "1", count: 1, rows: [1] },
            { tag: "2", count: 2, rows: [2] },
            { tag: "4", count: 4, rows: [2, 2] },
            { tag: "5", count: 5, rows: [3, 2] },
            { tag: "7", count: 7, rows: [3, 3, 1] },
            { tag: "9", count: 9, rows: [3, 3, 3] },
            { tag: "10", count: 10, rows: [4, 4, 2] },
        ];
    }

    function test_grid_windows_per_row(data) {
        const ids = Array.from({ length: data.count }, (_, i) => `w${i}`);
        const countsByY = new Map();
        for (const { rect } of filled(layoutFrom("createGrid"), ids).arrange(area)) {
            countsByY.set(rect.y, (countsByY.get(rect.y) || 0) + 1);
        }
        compare(Array.from(countsByY.values()), data.rows);
    }

    function test_grid_a_resize_changes_nothing_swapping_trades_places_removing_rearranges() {
        const layout = filled(layoutFrom("createGrid"), ["a", "b", "c", "d"]);
        const before = KWin.rects(layout);
        layout.resize("a", { x: 60, y: 40, width: 900, height: 600 });
        compare(KWin.rects(layout), before);
        layout.swap("a", "d");
        compare(KWin.rects(layout).a, before.d);
        layout.remove("b");
        compare(Object.keys(KWin.rects(layout)).length, 3);
        verify(!layout.isEmpty());
    }

    // The script.

    function test_loading_the_script_tiles_the_windows_already_open() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        load([a, b]);
        compare(geometry(a), { x: 60, y: 40, width: 500, height: 600 });
        compare(geometry(b), { x: 560, y: 40, width: 500, height: 600 });
    }

    function test_a_new_window_enters_next_to_the_last_focused_one() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const { workspace } = load([a, b]);
        workspace.windowActivated(a);
        const c = KWin.fakeWindow("c");
        workspace.windowAdded(c);
        compare(geometry(b).height, 600);
        compare(geometry(a).height, 300);
        compare(geometry(c).y, 340);
    }

    function test_stays_out_of_the_layout_data() {
        return [
            { tag: "a dialog", overrides: { normalWindow: false } },
            { tag: "a transient window", overrides: { transient: true } },
            { tag: "a special window", overrides: { specialWindow: true } },
            { tag: "a window of a fixed size", overrides: { resizeable: false } },
            { tag: "a minimized window", overrides: { minimized: true } },
            { tag: "a full screen window", overrides: { fullScreen: true } },
            { tag: "a maximized window", overrides: { maximizeMode: 3 } },
            { tag: "a window on all desktops", overrides: { onAllDesktops: true } },
            { tag: "a window hidden from the task bar and Alt+Tab", overrides: { skipTaskbar: true, skipSwitcher: true } },
            { tag: "Bresca Welcome", overrides: { desktopFileName: "com.brescaos.welcome" } },
        ];
    }

    function test_stays_out_of_the_layout(data) {
        const window = KWin.fakeWindow("a", data.overrides);
        const before = geometry(window);
        load([window]);
        compare(geometry(window), before);
    }

    function test_bresca_welcome_takes_no_place() {
        const a = KWin.fakeWindow("a");
        load([a, KWin.fakeWindow("welcome", { desktopFileName: "com.brescaos.welcome" })]);
        compare(geometry(a).width, area.width);
    }

    function test_a_window_hidden_only_from_the_task_bar_is_still_tiled() {
        const a = KWin.fakeWindow("a", { skipTaskbar: true });
        load([a]);
        compare(geometry(a).width, 1000);
    }

    function test_a_new_window_stays_hidden_until_it_takes_its_size() {
        const { workspace } = load();
        const a = KWin.fakeWindow("a");
        workspace.windowAdded(a);
        compare(a.opacity, 0);
        a.frameGeometryChanged();
        compare(a.opacity, 1);
        verify(!KWin.FakeTimer.last.running);
    }

    function test_a_new_window_that_never_takes_its_size_shows_anyway() {
        const { workspace } = load();
        const a = KWin.fakeWindow("a");
        workspace.windowAdded(a);
        KWin.FakeTimer.last.timeout();
        compare(a.opacity, 1);
    }

    function test_a_new_window_keeps_the_opacity_it_had() {
        const { workspace } = load();
        const a = KWin.fakeWindow("a", { opacity: 0.8 });
        workspace.windowAdded(a);
        a.frameGeometryChanged();
        compare(a.opacity, 0.8);
    }

    function test_a_new_window_already_at_its_size_isnt_hidden() {
        const { workspace } = load();
        const a = KWin.fakeWindow("a", { frameGeometry: KWin.moved(area, {}) });
        workspace.windowAdded(a);
        compare(a.opacity, 1);
    }

    function test_a_window_closed_while_hidden_stops_waiting() {
        const { workspace } = load();
        const a = KWin.fakeWindow("a");
        workspace.windowAdded(a);
        workspace.windowRemoved(a);
        verify(!KWin.FakeTimer.last.running);
    }

    function test_a_window_left_out_of_the_layout_isnt_hidden() {
        const { workspace } = load();
        const dialog = KWin.fakeWindow("dialog", { transient: true });
        workspace.windowAdded(dialog);
        compare(dialog.opacity, 1);
    }

    function test_windows_already_open_arent_hidden_when_the_script_loads() {
        const a = KWin.fakeWindow("a");
        load([a]);
        compare(a.opacity, 1);
    }

    function test_each_desktop_has_its_own_layout() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b", { desktops: [{ id: "desktop-2" }] });
        load([a, b]);
        compare(geometry(a).width, 1000);
        compare(geometry(b).width, 1000);
    }

    function test_minimizing_takes_a_window_out_and_restoring_brings_it_back() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        load([a, b]);
        b.minimized = true;
        b.minimizedChanged();
        compare(geometry(a).width, 1000);
        b.minimized = false;
        b.minimizedChanged();
        compare(geometry(a).width, 500);
    }

    function test_closing_a_window_gives_its_space_back() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const { workspace } = load([a, b]);
        workspace.windowRemoved(b);
        compare(geometry(a).width, 1000);
    }

    function test_after_a_move_a_window_goes_back_to_its_place() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        load([a, b]);
        const place = geometry(a);
        a.interactiveMoveResizeStarted();
        a.frameGeometry = KWin.moved(place, { x: 300, y: 200 });
        a.interactiveMoveResizeFinished();
        compare(geometry(a), place);
        compare(geometry(b).x, 560);
    }

    // A window whose minimum size doesn't fit its place.

    function test_a_window_that_doesnt_fit_its_place_keeps_it_while_not_in_use() {
        compare(geometry(bigOnLastRow().big), bigPlace);
    }

    function test_in_use_it_grows_to_its_minimum_and_those_affected_shrink_with_no_overlap() {
        const { a, b, big, workspace } = bigOnLastRow();
        KWin.activate(workspace, big);
        compare(geometry(big), { x: 60, y: 140, width: 1000, height: 500 });
        compare(geometry(a), { x: 60, y: 40, width: 500, height: 100 });
        compare(geometry(b), { x: 560, y: 40, width: 500, height: 100 });
    }

    function test_out_of_use_everything_goes_back_to_the_layouts_places() {
        const { a, b, big, workspace } = bigOnLastRow();
        const places = [geometry(a), geometry(b)];
        KWin.activate(workspace, big);
        KWin.activate(workspace, a);
        compare(geometry(big), bigPlace);
        compare([geometry(a), geometry(b)], places);
        KWin.activate(workspace, big);
        KWin.activate(workspace, null);
        compare(geometry(big), bigPlace);
    }

    function test_sideways_too_in_the_middle_of_a_row_both_neighbours_shrinking() {
        const a = KWin.fakeWindow("a");
        const big = KWin.fakeWindow("big", { minSize: { width: 600, height: 0 } });
        const c = KWin.fakeWindow("c");
        const others = ["d", "e", "f"].map(id => KWin.fakeWindow(id));
        const { workspace } = load([a, big, c].concat(others), { Gap: 0, Layout: "grid" });
        KWin.activate(workspace, big);
        compare(geometry(big).width, 600);
        compare(geometry(a).x, area.x);
        compare(geometry(a).x + geometry(a).width, geometry(big).x);
        compare(geometry(big).x + geometry(big).width, geometry(c).x);
        compare(geometry(c).x + geometry(c).width, area.x + area.width);
    }

    function test_the_gap_stays_between_windows_and_counts_towards_the_room_needed() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const big = KWin.fakeWindow("big", { minSize: { width: 0, height: 500 } });
        const { workspace } = load([a, b, big], { Gap: 10, Layout: "grid" });
        KWin.activate(workspace, big);
        compare(geometry(big).height, 500);
        compare(geometry(big).y + geometry(big).height, area.y + area.height);
        compare(geometry(a).y + geometry(a).height + 10, geometry(big).y);
    }

    function test_a_minimum_bigger_than_the_area_takes_all_of_it_on_that_axis() {
        const a = KWin.fakeWindow("a");
        const big = KWin.fakeWindow("big", { minSize: { width: 5000, height: 0 } });
        const { workspace } = load([a, big], { Gap: 0, Layout: "master" });
        KWin.activate(workspace, big);
        compare(geometry(big).x, area.x);
        compare(geometry(big).width, area.width);
    }

    function test_a_dialog_of_it_keeps_the_room() {
        const { big, workspace } = bigOnLastRow();
        KWin.activate(workspace, big);
        const room = geometry(big);
        const dialog = KWin.fakeWindow("dialog", { normalWindow: false, transient: true, transientFor: big });
        workspace.windowAdded(dialog);
        KWin.activate(workspace, dialog);
        compare(geometry(big), room);
    }

    function test_a_window_that_fits_changes_nothing_when_it_gets_the_focus() {
        const { a, b, big, workspace } = bigOnLastRow();
        const places = [geometry(a), geometry(b), geometry(big)];
        KWin.activate(workspace, a);
        compare([geometry(a), geometry(b), geometry(big)], places);
    }

    function test_the_room_is_given_in_any_layout_and_follows_when_others_rearrange() {
        const { b, big, workspace } = bigOnLastRow();
        KWin.activate(workspace, big);
        workspace.windowRemoved(b);
        compare(geometry(big).width, 700);
        compare(geometry(big).height, 600);
    }

    // Dragging.

    function test_a_click_on_a_title_bar_doesnt_swap_it() {
        const { a, big, workspace } = bigOnLastRow();
        KWin.activate(workspace, big);
        const placeOfA = geometry(a);
        big.interactiveMoveResizeStarted();
        workspace.cursorPos = { x: placeOfA.x + 10, y: placeOfA.y + 50 };
        big.interactiveMoveResizeFinished();
        compare(geometry(a), placeOfA);
    }

    function test_dropping_on_a_window_uses_where_its_shown_shrunk_by_the_one_in_use() {
        const { a, b, big, workspace } = bigOnLastRow();
        KWin.activate(workspace, big);
        const placeOfBig = geometry(big);
        a.interactiveMoveResizeStarted();
        a.frameGeometry = KWin.moved(geometry(a), { x: 300 });
        workspace.cursorPos = { x: placeOfBig.x + 10, y: placeOfBig.y + 10 };
        a.interactiveMoveResizeFinished();
        compare(geometry(b).y, area.y);
        compare(geometry(big).y, area.y);
    }

    function test_a_window_dropped_on_another_trades_places_with_it() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const c = KWin.fakeWindow("c");
        const { workspace } = load([a, b, c]);
        const places = { a: geometry(a), b: geometry(b), c: geometry(c) };
        a.interactiveMoveResizeStarted();
        a.frameGeometry = KWin.moved(places.a, { x: 400, y: 300 });
        workspace.cursorPos = { x: places.c.x + 10, y: places.c.y + 10 };
        a.interactiveMoveResizeFinished();
        compare(geometry(a), places.c);
        compare(geometry(c), places.a);
        compare(geometry(b), places.b);
    }

    function test_a_gap_between_two_windows_still_counts_as_dropping_on_one() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const { workspace } = load([a, b], { Gap: 16 });
        const places = { a: geometry(a), b: geometry(b) };
        a.interactiveMoveResizeStarted();
        a.frameGeometry = KWin.moved(places.a, { x: 300 });
        workspace.cursorPos = { x: places.b.x - 1, y: places.b.y };
        a.interactiveMoveResizeFinished();
        compare(geometry(a), places.b);
        compare(geometry(b), places.a);
    }

    function test_a_window_dropped_on_its_own_place_stays() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const { workspace } = load([a, b]);
        const places = { a: geometry(a), b: geometry(b) };
        a.interactiveMoveResizeStarted();
        a.frameGeometry = KWin.moved(places.a, { x: 100 });
        workspace.cursorPos = { x: places.a.x + 10, y: places.a.y + 10 };
        a.interactiveMoveResizeFinished();
        compare(geometry(a), places.a);
        compare(geometry(b), places.b);
    }

    function test_a_window_dropped_on_a_floating_ones_spot_or_a_different_layout_doesnt_swap() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const other = KWin.fakeWindow("other", { desktops: [{ id: "desktop-2" }] });
        const { workspace, shortcuts } = load([a, b, other]);
        const places = { a: geometry(a), b: geometry(b) };
        workspace.activeWindow = b;
        shortcuts["Meta+Shift+F"]();
        const alone = geometry(a);
        a.interactiveMoveResizeStarted();
        a.frameGeometry = KWin.moved(alone, { x: 100 });
        workspace.cursorPos = { x: places.b.x + 10, y: places.b.y + 10 };
        a.interactiveMoveResizeFinished();
        compare(geometry(a), alone);
        compare(geometry(other).width, 1000);
    }

    function test_resizing_a_window_with_the_cursor_over_another_doesnt_swap() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const { workspace } = load([a, b]);
        a.interactiveMoveResizeStarted();
        a.frameGeometry = KWin.moved(geometry(a), { width: 700 });
        workspace.cursorPos = { x: geometry(b).x + 10, y: geometry(b).y + 10 };
        a.interactiveMoveResizeFinished();
        compare(geometry(a).x, 60);
        compare(geometry(a).width, 700);
    }

    function test_after_a_resize_the_split_follows_and_the_sibling_makes_room() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        load([a, b]);
        a.interactiveMoveResizeStarted();
        a.frameGeometry = KWin.moved(geometry(a), { width: 700 });
        a.interactiveMoveResizeFinished();
        compare(geometry(a).width, 700);
        compare(geometry(b), { x: 760, y: 40, width: 300, height: 600 });
    }

    function test_a_window_being_dragged_isnt_placed_when_another_one_opens() {
        const a = KWin.fakeWindow("a");
        const { workspace } = load([a]);
        a.interactiveMoveResizeStarted();
        const dragged = { x: 300, y: 200, width: 1000, height: 600 };
        a.frameGeometry = dragged;
        workspace.windowAdded(KWin.fakeWindow("b"));
        compare(geometry(a), dragged);
    }

    function test_a_window_dragged_to_another_screen_enters_that_screens_layout() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        load([a, b]);
        b.interactiveMoveResizeStarted();
        b.output = { name: "Virtual-2" };
        b.outputChanged();
        compare(geometry(a).width, 500);
        b.interactiveMoveResizeFinished();
        compare(geometry(a).width, 1000);
        compare(geometry(b).width, 1000);
    }

    // Settings: edge snapping, the layout and the gap.

    function test_turns_off_snapping_to_the_screens_edges_also_after_KWin_reloads_its_settings() {
        const { options } = load();
        verify(!options.electricBorderMaximize);
        verify(!options.electricBorderTiling);
        KWin.reloadSettings(options);
        verify(!options.electricBorderMaximize);
        verify(!options.electricBorderTiling);
    }

    function test_follows_a_layout_saved_by_the_applet_or_the_KCM() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const c = KWin.fakeWindow("c");
        const { options, config } = load([a, b, c]);
        config.Layout = "grid";
        KWin.reloadSettings(options);
        compare(geometry(a).height, 300);
        compare(geometry(b).height, 300);
        compare(geometry(c).width, area.width);
    }

    function test_switching_layout_puts_the_active_window_first_then_by_most_recent_use() {
        const [a, b, c, d] = ["a", "b", "c", "d"].map(id => KWin.fakeWindow(id));
        const { workspace, options, config } = load([a, b, c, d]);
        workspace.activeWindow = c;
        config.Layout = "master";
        KWin.reloadSettings(options);
        compare(geometry(c), { x: 60, y: 40, width: 500, height: 600 });
        compare(geometry(a), { x: 560, y: 40, width: 500, height: 200 });
        compare(geometry(b), { x: 560, y: 240, width: 500, height: 200 });
        compare(geometry(d), { x: 560, y: 440, width: 500, height: 200 });
    }

    function test_back_in_the_binary_tree_a_new_window_splits_the_active_one() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const { workspace, options, config } = load([a, b], { Gap: 0, Layout: "grid" });
        workspace.activeWindow = a;
        config.Layout = "binaryTree";
        KWin.reloadSettings(options);
        workspace.windowAdded(KWin.fakeWindow("c"));
        compare(geometry(b).height, 600);
        compare(geometry(a).height, 300);
    }

    function test_a_window_floated_by_hand_stays_floating_across_a_switch() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const { workspace, shortcuts, options, config } = load([a, b]);
        workspace.activeWindow = b;
        shortcuts["Meta+Shift+F"]();
        config.Layout = "master";
        KWin.reloadSettings(options);
        compare(geometry(a).width, area.width);
    }

    function test_a_saved_layout_that_doesnt_exist_falls_back_to_the_default() {
        const a = KWin.fakeWindow("a");
        load([a], { Gap: 0, Layout: "kde" });
        compare(geometry(a).width, area.width);
    }

    function test_the_gap_is_between_windows_not_at_the_screens_edges() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const { workspace } = load([a, b], { Gap: 10 });
        workspace.windowActivated(b);
        const c = KWin.fakeWindow("c");
        workspace.windowAdded(c);
        compare(geometry(a), { x: 60, y: 40, width: 495, height: 600 });
        compare(geometry(b), { x: 565, y: 40, width: 495, height: 295 });
        compare(geometry(c), { x: 565, y: 345, width: 495, height: 295 });
    }

    function test_an_odd_gap_is_still_that_many_pixels() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        load([a, b], { Gap: 5 });
        compare(geometry(b).x - (geometry(a).x + geometry(a).width), 5);
    }

    function test_one_window_alone_has_no_gap() {
        const a = KWin.fakeWindow("a");
        load([a], { Gap: 10 });
        compare(geometry(a), area);
    }

    function test_a_move_with_a_gap_puts_the_window_back_without_moving_the_split() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        load([a, b], { Gap: 10 });
        const place = geometry(a);
        a.interactiveMoveResizeStarted();
        a.frameGeometry = KWin.moved(place, { x: 300, y: 200 });
        a.interactiveMoveResizeFinished();
        compare(geometry(a), place);
    }

    function test_a_resize_with_a_gap_moves_the_split_by_what_the_edge_moved() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        load([a, b], { Gap: 10 });
        a.interactiveMoveResizeStarted();
        a.frameGeometry = KWin.moved(geometry(a), { width: 695 });
        a.interactiveMoveResizeFinished();
        compare(geometry(a).width, 695);
        compare(geometry(b), { x: 765, y: 40, width: 295, height: 600 });
    }

    function test_a_reload_applies_the_new_gap_saved_by_the_KCM() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const { options, config } = load([a, b], { Gap: 0 });
        compare(geometry(a).width, 500);
        config.Gap = 20;
        options.configChanged();
        compare(geometry(a).width, 490);
        compare(geometry(b).x, 570);
    }

    // The float shortcut.

    function test_the_shortcut_floats_the_active_window_and_tiles_it_again() {
        const a = KWin.fakeWindow("a");
        const b = KWin.fakeWindow("b");
        const { workspace, shortcuts } = load([a, b]);
        workspace.activeWindow = b;
        shortcuts["Meta+Shift+F"]();
        compare(geometry(a).width, 1000);
        shortcuts["Meta+Shift+F"]();
        compare(geometry(a).width, 500);
    }

    function test_the_shortcut_does_nothing_without_an_active_window() {
        load().shortcuts["Meta+Shift+F"]();
    }

    // What the script and AutotileLayout must agree on.

    function test_the_script_and_AutotileLayout_list_the_same_layouts_and_defaults() {
        const script = KWin.internals(scriptSource);
        compare(Object.keys(script.LAYOUTS), autotileLayoutNames);
        compare(script.DEFAULT_LAYOUT, autotileDefaultLayout);
        compare(script.DEFAULT_GAP, autotileDefaultGap);
    }
}
