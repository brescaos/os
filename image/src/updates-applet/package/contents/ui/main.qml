// The system tray's icon for a new BrescaOS version (../../../README.md): what
// the last check (bresca-updates@check) left in check.json. The file is read
// only when it changes: inotifywait waits for that asleep in the kernel, no
// polling.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.notification
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasma5support as P5Support
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: root

    // Created at boot (bresca-updates.tmpfiles), so it can be watched before
    // the first check writes check.json.
    readonly property string resultsFolder: "/run/bresca/updates"
    readonly property string checkResultPath: resultsFolder + "/check.json"
    readonly property string readCommand: "cat " + checkResultPath
    // Ends when check.json is moved into place (a check) or removed (an
    // install); not on download-progress.json, which a download replaces
    // all the time.
    readonly property string waitForChangeCommand: "inotifywait -qq -e moved_to,delete --include 'check\\.json$' " + resultsFolder
    readonly property string installCommand: "sudo bresca updates install --download"
    // The version the last check found, empty when there's none.
    property string availableVersion: ""
    // The executable engine keeps a finished command's output and hands it
    // back at once when the same command is connected again (stale, and in a
    // loop for the watcher): each run gets its number as a shell comment, so
    // it's a new command and runs.
    property int commandRuns: 0
    // So each version is notified once per session: at every login while it
    // isn't installed, the reminder, and when a check finds it.
    property string notifiedVersion: ""

    Plasmoid.icon: "system-software-update"
    // Hidden among the tray's idle items while there's nothing to install.
    Plasmoid.status: availableVersion ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
    toolTipMainText: i18n("BrescaOS Updates")
    toolTipSubText: availableVersion ? i18n("BrescaOS %1 is available", availableVersion) : i18n("BrescaOS is up to date")

    fullRepresentation: Item {
        Layout.preferredWidth: Kirigami.Units.gridUnit * 18
        Layout.preferredHeight: Kirigami.Units.gridUnit * 10

        PlasmaExtras.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 2
            iconName: "system-software-update"
            text: root.availableVersion ? i18n("BrescaOS %1 is available", root.availableVersion) : i18n("BrescaOS is up to date")
            explanation: root.availableVersion ? i18n("Install it with: %1", root.installCommand) : ""
        }
    }

    P5Support.DataSource {
        id: executable

        engine: "executable"
        onNewData: (command, output) => {
            executable.disconnectSource(command);
            if (command.startsWith(root.readCommand)) {
                root.showCheckResult(output["stdout"]);
            } else if (output["exit code"] === 0) {
                root.readCheckResult();
                // Later, never from inside this handler.
                Qt.callLater(root.waitForChange);
            } else {
                // It couldn't watch (no folder): no retrying in a loop.
                retryTimer.start();
            }
        }
    }

    Timer {
        id: retryTimer

        interval: 60000
        onTriggered: root.waitForChange()
    }

    Component.onCompleted: {
        readCheckResult();
        waitForChange();
    }

    Notification {
        id: updateNotification

        componentName: "brescaos-updates"
        eventId: "updateAvailable"
        iconName: "system-software-update"
        title: i18n("BrescaOS %1 is available", root.availableVersion)
        text: i18n("Install it with: %1", root.installCommand)
        flags: Notification.Persistent
    }

    function runCommand(command) {
        commandRuns++;
        executable.connectSource(command + " #" + commandRuns);
    }

    function readCheckResult() {
        runCommand(readCommand);
    }

    function waitForChange() {
        runCommand(waitForChangeCommand);
    }

    // No file (a check is running) or a failed check (no network): what was
    // shown stays.
    function showCheckResult(checkResultJson) {
        let checkResult;
        try {
            checkResult = JSON.parse(checkResultJson);
        } catch (error) {
            return;
        }
        if (checkResult.error !== undefined) {
            return;
        }
        availableVersion = checkResult.update_available ? checkResult.available_version : "";
        if (availableVersion && availableVersion !== notifiedVersion) {
            notifiedVersion = availableVersion;
            updateNotification.sendEvent();
        }
    }
}
