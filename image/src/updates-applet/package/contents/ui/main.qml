// The system tray's icon for a new BrescaOS version (../../../README.md): what
// the last check (bresca-updates@check) left in check.json; and the
// notifications for a version downloaded (download.json) or installed
// (install.json). The files are read only when they change: inotifywait waits
// for that asleep in the kernel, no polling.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.notification
import org.kde.plasma.components as PlasmaComponents3
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
    readonly property string readDownloadResultCommand: "cat " + resultsFolder + "/download.json"
    readonly property string readInstallResultCommand: "cat " + resultsFolder + "/install.json"
    // Ends when check.json, download.json or install.json is moved into place
    // (an operation's result) or removed (as the operation starts; check.json
    // by an install too); not on download-progress.json, which a download
    // replaces all the time.
    readonly property string waitForChangeCommand: "inotifywait -qq -e moved_to,delete --include '(check|download|install)\\.json$' " + resultsFolder
    // Plasma's restart prompt, as Restart in its menu.
    readonly property string promptRestartCommand: "gdbus call --session --dest org.kde.LogoutPrompt --object-path /LogoutPrompt --method org.kde.LogoutPrompt.promptReboot"
    readonly property string installCommand: "sudo bresca updates install --download"
    // A user without a polkit rule for it (not in wheel) is asked for an
    // administrator's password; cancelling makes it fail.
    readonly property string startCheckCommand: "systemctl start --no-block bresca-updates@check.service"
    // "activating" while it runs (a oneshot service).
    readonly property string isCheckRunningCommand: "systemctl is-active bresca-updates@check.service"
    // The version the last check found, empty when there's none.
    property string availableVersion: ""
    // A check is running: from the button, or the timer's.
    property bool isChecking: false
    // The button's systemctl start hasn't returned: the password dialog may be
    // open, and the check hasn't started yet.
    property bool isStartPending: false
    property bool isLastCheckFailed: false
    // The executable engine keeps a finished command's output and hands it
    // back at once when the same command is connected again (stale, and in a
    // loop for the watcher): each run gets its number as a shell comment, so
    // it's a new command and runs.
    property int commandRuns: 0
    // So each version is notified once per session: at every login while it
    // isn't installed, the reminder, and when a check finds it.
    property string notifiedVersion: ""
    // When the download and the install read last finished (downloaded_at,
    // installed_at; empty with no result), undefined until the first read: a
    // result is notified only when it changes, so not one from before the
    // applet started (a login, a restarted plasmashell).
    property var lastDownloadedAt: undefined
    property var lastInstalledAt: undefined
    property string downloadedVersion: ""
    property string installedVersion: ""

    Plasmoid.icon: "system-software-update"
    // Hidden among the tray's idle items while there's nothing to install.
    Plasmoid.status: availableVersion ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
    toolTipMainText: i18n("BrescaOS Updates")
    toolTipSubText: statusText

    readonly property string statusText: {
        if (isChecking) {
            return i18n("Checking for updates…");
        }
        if (availableVersion) {
            return i18n("BrescaOS %1 is available", availableVersion);
        }
        if (isLastCheckFailed) {
            return i18n("Couldn't check for updates");
        }
        return i18n("BrescaOS is up to date");
    }

    fullRepresentation: Item {
        Layout.preferredWidth: Kirigami.Units.gridUnit * 18
        Layout.preferredHeight: Kirigami.Units.gridUnit * 12

        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 2
            spacing: Kirigami.Units.largeSpacing

            PlasmaExtras.PlaceholderMessage {
                Layout.fillWidth: true
                iconName: "system-software-update"
                text: root.statusText
                explanation: root.availableVersion && !root.isChecking ? i18n("Install it with: %1", root.installCommand) : ""
            }

            // Hidden with opacity, not visible: its space stays, so the button
            // doesn't move when a check starts or ends.
            PlasmaComponents3.ProgressBar {
                Layout.fillWidth: true
                opacity: root.isChecking ? 1 : 0
                indeterminate: root.isChecking
            }

            PlasmaComponents3.Button {
                Layout.alignment: Qt.AlignHCenter
                visible: !root.availableVersion
                enabled: !root.isChecking
                icon.name: "view-refresh"
                text: i18n("Check for Updates")
                onClicked: root.startCheck()
            }
        }
    }

    P5Support.DataSource {
        id: executable

        engine: "executable"
        onNewData: (command, output) => {
            executable.disconnectSource(command);
            if (command.startsWith(root.readCommand)) {
                root.showCheckResult(output["stdout"]);
            } else if (command.startsWith(root.readDownloadResultCommand)) {
                root.notifyDownloadResult(output["stdout"]);
            } else if (command.startsWith(root.readInstallResultCommand)) {
                root.notifyInstallResult(output["stdout"]);
            } else if (command.startsWith(root.promptRestartCommand)) {
                // Nothing to do with its output.
            } else if (command.startsWith(root.isCheckRunningCommand)) {
                const isCheckRunning = output["stdout"].trim() === "activating";
                // A check that ended while no watcher was listening: its result.
                if (root.isChecking && !isCheckRunning) {
                    root.readCheckResult();
                }
                root.isChecking = isCheckRunning;
            } else if (command.startsWith(root.startCheckCommand)) {
                root.isStartPending = false;
                // Not started (the password dialog cancelled): as before.
                if (output["exit code"] !== 0) {
                    root.isChecking = false;
                }
            } else if (output["exit code"] === 0) {
                root.readResults();
                // Later, never from inside this handler.
                Qt.callLater(root.waitForChange);
            } else {
                // It couldn't watch (no folder): no retrying in a loop.
                retryTimer.start();
            }
        }
    }

    // A check's result can come while the applet starts its next wait, and
    // then nothing wakes it: while a check runs, systemd is asked every 2 s,
    // so it shows at most that late. Only then: at rest only inotifywait.
    Timer {
        interval: 2000
        repeat: true
        running: root.isChecking && !root.isStartPending
        onTriggered: root.runCommand(root.isCheckRunningCommand)
    }

    Timer {
        id: retryTimer

        interval: 60000
        onTriggered: root.waitForChange()
    }

    Component.onCompleted: {
        readResults();
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

    // Persistent: restarting discards a version only downloaded.
    Notification {
        id: downloadedNotification

        componentName: "brescaos-updates"
        eventId: "updateDownloaded"
        iconName: "run-install"
        title: i18n("BrescaOS %1 is downloaded", root.downloadedVersion)
        text: i18n("Install it before restarting: restarting discards the download.")
        flags: Notification.Persistent
        actions: [
            NotificationAction {
                label: i18n("Install")
                onActivated: KCM.KCMLauncher.openSystemSettings("kcm_brescaos_updates")
            }
        ]
    }

    Notification {
        id: installedNotification

        componentName: "brescaos-updates"
        eventId: "updateInstalled"
        iconName: "system-reboot"
        title: i18n("BrescaOS %1 is installed", root.installedVersion)
        text: i18n("Restart to start using it.")
        flags: Notification.Persistent
        actions: [
            NotificationAction {
                label: i18n("Restart Now")
                onActivated: root.runCommand(root.promptRestartCommand)
            }
        ]
    }

    function runCommand(command) {
        commandRuns++;
        executable.connectSource(command + " #" + commandRuns);
    }

    function readCheckResult() {
        runCommand(readCommand);
    }

    function readResults() {
        readCheckResult();
        runCommand(readDownloadResultCommand);
        runCommand(readInstallResultCommand);
    }

    // The result in resultJson, null when there's none (cat found no file).
    function parseResult(resultJson) {
        try {
            return JSON.parse(resultJson);
        } catch (error) {
            return null;
        }
    }

    // A version downloaded without installing it: download-install's isn't
    // (download_only false), its install follows.
    function notifyDownloadResult(resultJson) {
        const result = parseResult(resultJson);
        const downloadedAt = result?.downloaded_at ?? "";
        if (lastDownloadedAt !== undefined && downloadedAt !== lastDownloadedAt
                && result?.download_only && result.update_available && !result.error) {
            downloadedVersion = result.downloaded_version;
            downloadedNotification.sendEvent();
        }
        lastDownloadedAt = downloadedAt;
    }

    function notifyInstallResult(resultJson) {
        const result = parseResult(resultJson);
        const installedAt = result?.installed_at ?? "";
        if (lastInstalledAt !== undefined && installedAt !== lastInstalledAt
                && result?.installed_version && !result.error) {
            installedVersion = result.installed_version;
            installedNotification.sendEvent();
        }
        lastInstalledAt = installedAt;
    }

    function waitForChange() {
        runCommand(waitForChangeCommand);
    }

    function startCheck() {
        isChecking = true;
        isStartPending = true;
        runCommand(startCheckCommand);
    }

    // No file: a check removed it as it started, or an install did; systemd
    // says which. A failed check (no network) keeps the version shown.
    function showCheckResult(checkResultJson) {
        let checkResult;
        try {
            checkResult = JSON.parse(checkResultJson);
        } catch (error) {
            runCommand(isCheckRunningCommand);
            return;
        }
        isChecking = false;
        isLastCheckFailed = checkResult.error !== undefined;
        if (isLastCheckFailed) {
            return;
        }
        availableVersion = checkResult.update_available ? checkResult.available_version : "";
        if (availableVersion && availableVersion !== notifiedVersion) {
            notifiedVersion = availableVersion;
            updateNotification.sendEvent();
        }
    }
}
