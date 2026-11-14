// Hexa's top bar: a panel containment with no background, so Plasma draws
// neither a background nor a shadow behind it, whatever the Plasma style.
// Its applets in one row, in the order kept in its configuration
// (AppletOrder): each one as tall as the bar and as wide as it asks, the
// ones that ask to fill sharing the free width; with two spacers, what's
// between them stays centred on the bar while both sides fit. Its widgets
// are fixed: its
// panel template (layout-templates/com.brescaos.hexa.topbar) adds them, and
// nothing can be dropped on it or removed from it, only configured; the whole
// bar can still be removed. Its content is white or black, whichever reads
// better on the wallpaper behind it.
import QtQuick
import QtQuick.Layouts
import com.brescaos.hexa
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

ContainmentItem {
    id: root

    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground

    // The applets take their colors from here: a color set on a theme that
    // inherits stays on its own item. The theme's own color until the
    // wallpaper has been looked at.
    Kirigami.Theme.inherit: false
    Kirigami.Theme.textColor: wallpaperContrast.isValid ? (wallpaperContrast.isDark ? "white" : "black") : undefined

    WallpaperContrast {
        id: wallpaperContrast
        panel: Plasmoid
        thickness: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? root.width : root.height
    }

    // An applet in the row, with the width it asks for, or the one
    // centringWidths gives it, if any.
    component AppletSlot: Item {
        required property Item appletItem
        required property int appletId

        // An applet with nothing to show hides itself (the global menu, with
        // no app's menu), as a Plasma panel hides it.
        readonly property bool isShown: appletItem.plasmoid.status !== PlasmaCore.Types.HiddenStatus
        readonly property bool isSpacer: appletItem.plasmoid.pluginName === "org.kde.plasma.panelspacer"
        // A spacer, or an applet that takes the room there is (the global menu).
        readonly property bool isFilling: appletItem.Layout.fillWidth
        // The least it shows whole in: nothing for a spacer, its minimum if
        // it fills, a square when it asks for no width.
        readonly property real neededWidth: {
            if (isSpacer) {
                return 0;
            }
            if (isFilling) {
                return Math.max(0, appletItem.Layout.minimumWidth);
            }
            return Math.max(appletItem.Layout.minimumWidth,
                appletItem.Layout.preferredWidth > 0 ? appletItem.Layout.preferredWidth : height);
        }
        readonly property bool isCentring: root.centringWidths[appletId] !== undefined

        visible: isShown
        clip: width < neededWidth
        Layout.fillHeight: true
        Layout.fillWidth: isFilling && !isCentring
        Layout.minimumWidth: isCentring ? 0 : appletItem.Layout.minimumWidth
        Layout.preferredWidth: isCentring ? root.centringWidths[appletId] : neededWidth
    }

    // With exactly two spacers, every shown applet's width by its id, so
    // that what's between the spacers is centred on the row, and so on the
    // bar (its margins are the same on both sides). When a side needs more
    // than that leaves it, the middle moves away just enough, up to what
    // the other side needs; past that, the side is cut. Empty otherwise:
    // the applets that fill share the free width.
    readonly property var centringWidths: {
        const all = Array.from(row.children).filter(slot => slot.isShown);
        const spacers = all.filter(slot => slot.isSpacer);
        if (spacers.length !== 2) {
            return {};
        }
        const neededWidthOf = slots => slots.reduce((total, slot) => total + slot.neededWidth, 0);
        const leftSide = all.slice(0, all.indexOf(spacers[0]) + 1);
        const middle = all.slice(leftSide.length, all.indexOf(spacers[1]));
        const rightSide = all.slice(leftSide.length + middle.length);
        const middleWidth = neededWidthOf(middle);
        const leftWidth = Math.max(0, Math.min(
            Math.max((row.width - middleWidth) / 2, neededWidthOf(leftSide)),
            row.width - middleWidth - neededWidthOf(rightSide)));

        const widths = {};
        for (const slot of middle) {
            widths[slot.appletId] = slot.neededWidth;
        }
        // In a side, the applets that fill share what the others leave; its
        // spacer takes it only when there are none.
        const shareSide = (side, sideWidth) => {
            const fixed = side.filter(slot => !slot.isFilling);
            const filling = side.filter(slot => slot.isFilling && !slot.isSpacer);
            const sharing = filling.length > 0 ? filling : side.filter(slot => slot.isSpacer);
            const freeWidth = Math.max(0, sideWidth - neededWidthOf(fixed));
            for (const slot of side) {
                widths[slot.appletId] = slot.isFilling ? 0 : slot.neededWidth;
            }
            for (const slot of sharing) {
                widths[slot.appletId] = freeWidth / sharing.length;
            }
        };
        shareSide(leftSide, leftWidth);
        shareSide(rightSide, Math.max(0, row.width - leftWidth - middleWidth));
        return widths;
    }

    Component {
        id: slotComponent
        AppletSlot {}
    }

    // The slots in the row's order.
    function slots() {
        return Array.from(row.children);
    }

    // The applets' ids in the saved order.
    function savedOrder() {
        return Plasmoid.configuration.AppletOrder.split(";").filter(id => id !== "").map(Number);
    }

    function saveOrder() {
        Plasmoid.configuration.AppletOrder = slots().map(slot => slot.appletId).join(";");
    }

    // Puts the slots in the row in the order of $orderedSlots.
    function arrange(orderedSlots) {
        for (const slot of orderedSlots) {
            slot.parent = null;
        }
        for (const slot of orderedSlots) {
            slot.parent = row;
        }
    }

    // A slot for the applet $applet where the saved order puts it (Plasma
    // restores a bar's applets one by one, in no particular order), or last
    // if it isn't in it; whether the order changed, so it needs saving. The
    // applet is locked: Plasma then doesn't remove it, but still configures it.
    function addSlot(applet) {
        if (slots().some(slot => slot.appletId === applet.id)) {
            return false;
        }
        applet.immutability = PlasmaCore.Types.UserImmutable;
        const order = savedOrder();
        const rank = order.indexOf(applet.id);
        let index = -1;
        if (rank >= 0) {
            // After the slots that come before it in the saved order.
            index = slots().filter(slot => {
                const otherRank = order.indexOf(slot.appletId);
                return otherRank >= 0 && otherRank < rank;
            }).length;
        }
        const appletItem = root.itemFor(applet);
        if (!appletItem) {
            return false;
        }
        const slot = slotComponent.createObject(row, {
            appletItem: appletItem,
            appletId: applet.id
        });
        appletItem.parent = slot;
        appletItem.anchors.fill = slot;
        if (index >= 0) {
            const ordered = slots().filter(other => other !== slot);
            ordered.splice(index, 0, slot);
            arrange(ordered);
        }
        return rank < 0;
    }

    Connections {
        target: Plasmoid

        function onAppletAdded(applet, geometry) {
            if (root.addSlot(applet)) {
                root.saveOrder();
            }
        }
        function onAppletRemoved(applet) {
            const slot = root.slots().find(slot => slot.appletId === applet.id);
            if (slot) {
                slot.destroy();
                Qt.callLater(root.saveOrder);
            }
        }
    }

    Component.onCompleted: {
        let isOrderChanged = false;
        for (const applet of Plasmoid.applets) {
            isOrderChanged = addSlot(applet) || isOrderChanged;
        }
        if (isOrderChanged) {
            saveOrder();
        }
    }

    // A little room between the screen's edges and the first and last
    // applets (the launcher, the system tray's arrow).
    RowLayout {
        id: row
        anchors.fill: parent
        anchors.leftMargin: Kirigami.Units.smallSpacing
        anchors.rightMargin: Kirigami.Units.smallSpacing
        spacing: 0
    }
}
