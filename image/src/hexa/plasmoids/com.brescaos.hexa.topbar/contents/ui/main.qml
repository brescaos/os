// Hexa's top bar: a panel containment with no background, so Plasma draws
// neither a background nor a shadow behind it, whatever the Plasma style.
// Its applets in one row, in the order kept in its configuration
// (AppletOrder): each one as tall as the bar and as wide as it asks, the
// spacers sharing the free width; with two spacers, what's between them is
// centred on the bar, whatever the sides hold. Its widgets are fixed: its
// panel template (layout-templates/com.brescaos.hexa.topbar) adds them, and
// nothing can be dropped on it or removed or configured from it; the whole
// bar can still be removed.
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

ContainmentItem {
    id: root

    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground

    // An applet in the row, with the width it asks for; a spacer (it asks to
    // fill) with the width centringWidths gives it, if any.
    component AppletSlot: Item {
        required property Item appletItem
        required property int appletId

        // An applet with nothing to show hides itself (the global menu, with
        // no app's menu), as a Plasma panel hides it.
        readonly property bool isShown: appletItem.plasmoid.status !== PlasmaCore.Types.HiddenStatus
        readonly property bool isSpacer: appletItem.Layout.fillWidth
        // A square when it asks for no width.
        readonly property real askedWidth: Math.max(appletItem.Layout.minimumWidth,
            appletItem.Layout.preferredWidth > 0 ? appletItem.Layout.preferredWidth : height)
        readonly property bool isCentring: root.centringWidths[appletId] !== undefined

        visible: isShown
        Layout.fillHeight: true
        Layout.fillWidth: isSpacer && !isCentring
        Layout.minimumWidth: isCentring ? 0 : appletItem.Layout.minimumWidth
        Layout.preferredWidth: isCentring ? root.centringWidths[appletId] : askedWidth
    }

    // With exactly two spacers, their widths by applet id, so that what's
    // between them is centred on the bar: the first one fills up to there,
    // the second one the rest. Empty otherwise: spacers share the free width.
    readonly property var centringWidths: {
        const all = Array.from(row.children).filter(slot => slot.isShown);
        const spacers = all.filter(slot => slot.isSpacer);
        if (spacers.length !== 2) {
            return {};
        }
        const first = all.indexOf(spacers[0]);
        const second = all.indexOf(spacers[1]);
        const widthOf = slots => slots.reduce((total, slot) => total + slot.askedWidth, 0);
        const left = widthOf(all.slice(0, first));
        const middle = widthOf(all.slice(first + 1, second));
        const right = widthOf(all.slice(second + 1));
        const firstWidth = Math.max(0, (root.width - middle) / 2 - left);
        return {
            [spacers[0].appletId]: firstWidth,
            [spacers[1].appletId]: Math.max(0, root.width - left - firstWidth - middle - right)
        };
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
    // applet is locked: Plasma then neither removes nor configures it.
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

    RowLayout {
        id: row
        anchors.fill: parent
        spacing: 0
    }
}
