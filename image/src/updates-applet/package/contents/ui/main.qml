// The system tray's icon for BrescaOS versions (../../../README.md): what the
// bresca-updates@ operations (image/src/bresca-cli/) leave in
// /run/bresca/updates/: check.json (a version available), download.json (one
// downloaded), install.json (one installed) and status.json (the registry's
// TLS). Its popup downloads and installs a version, and it notifies a version
// available, downloaded or installed. The files are read only when they
// change: inotifywait waits for that asleep in the kernel. Only while an
// operation runs, systemd and its progress are asked every second or two.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.notification
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.core as PlasmaCore
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
    readonly property string readStatusCommand: "cat " + resultsFolder + "/status.json"
    // Ends when an operation's result is moved into place or removed (as the
    // operation starts; check.json by an install too); not on
    // download-progress.json, which a download replaces all the time.
    readonly property string waitForChangeCommand: "inotifywait -qq -e moved_to,delete --include '(check|download|install|status)\\.json$' " + resultsFolder
    // Plasma's restart prompt, as Restart in its menu.
    readonly property string promptRestartCommand: "gdbus call --session --dest org.kde.LogoutPrompt --object-path /LogoutPrompt --method org.kde.LogoutPrompt.promptReboot"
    // A user without a polkit rule for it (not in wheel) is asked for an
    // administrator's password; cancelling makes it fail.
    readonly property string startCheckCommand: "systemctl start --no-block bresca-updates@check.service"
    // "activating" while it runs (a oneshot service).
    readonly property string isCheckRunningCommand: "systemctl is-active bresca-updates@check.service"
    // The registry's TLS, for downloading. Never a password dialog out of the
    // blue: without the polkit rule (not in wheel) it just fails.
    readonly property string startStatusCommand: "systemctl --no-ask-password start --no-block bresca-updates@status.service"
    // Started from the popup: polkit asks for an administrator's password.
    readonly property string startOperationCommand: "systemctl start --no-block bresca-updates@"
    // The state of each operation the popup shows ("activating" while it
    // runs), in this order, then the download's progress.
    readonly property var operations: ["download", "download-install", "install"]
    readonly property string operationsStateCommand: "systemctl is-active " + operations.map(operation => "bresca-updates@" + operation + ".service").join(" ")
                                                     + "; cat " + resultsFolder + "/download-progress.json"
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
    // The last download's and install's results, null when there's none.
    property var downloadResult: null
    property var installResult: null
    // The registry's TLS (status.json): true, false, or undefined when unknown.
    property var hasTls: undefined
    // The operation running (one of operations), started here or elsewhere;
    // empty when none.
    property string runningOperation: ""
    // Its systemctl start hasn't returned: the password dialog may be open.
    property bool isOperationStartPending: false
    // The popup was open when a start was asked: the system tray closes it
    // when the password dialog takes the focus, and it opens again once the
    // dialog is gone.
    property bool shouldReopenPopup: false
    // download-progress.json while a download runs, null otherwise.
    property var progress: null
    // The download waiting for the user to accept a registry without TLS
    // (download or download-install), empty when none.
    property string unconfirmedOperation: ""

    // The version installed and waiting for a restart, empty when there's
    // none: /run/bresca/updates/ is emptied at boot.
    readonly property string restartVersion: installResult?.installed_version && !installResult.error ? installResult.installed_version : ""
    // The version downloaded without installing it, empty when there's none
    // or it's installed since.
    readonly property string downloadedVersion: downloadResult?.download_only && downloadResult.update_available && !downloadResult.error
                                                && !restartVersion ? downloadResult.downloaded_version : ""
    // download-install installs once its download left a result: the
    // download removes download.json as it starts.
    readonly property bool isDownloading: runningOperation === "download"
                                          || (runningOperation === "download-install" && !downloadResult)

    // Breeze's update status icons, as Plasma's own update notifier: what's
    // pending shows at a glance.
    readonly property string statusIcon: restartVersion ? "system-reboot"
                                       : availableVersion || downloadedVersion || runningOperation ? "update-medium"
                                       : "update-none"

    Plasmoid.icon: statusIcon
    // Hidden among the tray's idle items while there's nothing to do.
    Plasmoid.status: availableVersion || downloadedVersion || restartVersion || runningOperation
                     ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
    toolTipMainText: i18n("BrescaOS Updates")
    toolTipSubText: statusText

    readonly property string statusText: {
        if (restartVersion) {
            return i18n("BrescaOS %1 is installed", restartVersion);
        }
        if (runningOperation) {
            const version = downloadedVersion || availableVersion;
            if (isDownloading) {
                return version ? i18n("Downloading BrescaOS %1", version) : i18n("Downloading the new version");
            }
            return version ? i18n("Installing BrescaOS %1", version) : i18n("Installing the new version");
        }
        if (downloadedVersion) {
            return i18n("BrescaOS %1 is downloaded", downloadedVersion);
        }
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

    // The download's phase, with its percent when bootc gives one (as the
    // CLI shows it); bootc gives no progress while installing.
    readonly property string progressText: {
        const percent = progress?.percent;
        if (!progress) {
            return isDownloading ? i18n("Checking the new version…") : i18n("Installing…");
        }
        switch (progress.phase) {
        case "downloading":
            if (percent === undefined) {
                return i18n("Downloading…");
            }
            // xgettext:no-javascript-format
            return i18nc("Download progress", "Downloading… %1%", percent);
        case "preparing":
            if (percent === undefined) {
                return i18n("Preparing…");
            }
            // xgettext:no-javascript-format
            return i18nc("Download progress", "Preparing… %1%", percent);
        default:
            return i18n("Checking the new version…");
        }
    }

    // As tall as what it shows on its own. In the system tray's popup, whose
    // size Plasma fixes for every entry, it stays together at the top, as the
    // other entries do, not spread over it.
    fullRepresentation: Item {
        Layout.preferredWidth: Kirigami.Units.gridUnit * 20
        Layout.preferredHeight: popup.implicitHeight + Kirigami.Units.largeSpacing * 2

        ColumnLayout {
            id: popup

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.large
                    Layout.preferredHeight: Kirigami.Units.iconSizes.large
                    source: root.statusIcon
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 3
                        wrapMode: Text.Wrap
                        text: root.statusText
                    }
                    PlasmaComponents3.Label {
                        Layout.fillWidth: true
                        visible: text !== ""
                        wrapMode: Text.Wrap
                        opacity: 0.7
                        text: root.restartVersion ? i18n("Restart to start using it.")
                            : root.downloadedVersion && !root.runningOperation ? i18n("Install it before restarting: restarting discards the download.")
                            : ""
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: !!root.runningOperation && !root.restartVersion
                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents3.ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: root.progress?.percent ?? 0
                    indeterminate: root.progress?.percent === undefined
                }
                PlasmaComponents3.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.progressText
                }
            }

            PlasmaComponents3.ProgressBar {
                Layout.fillWidth: true
                visible: root.isChecking && !root.runningOperation && !root.restartVersion
                indeterminate: true
            }

            // The last download's or install's error, as bootc wrote it.
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                type: Kirigami.MessageType.Error
                visible: !root.runningOperation && text !== ""
                text: root.installResult?.error ? i18n("Couldn't install the new version: %1", root.lastLine(root.installResult.error))
                    : root.downloadResult?.error ? i18n("Couldn't download the new version: %1", root.lastLine(root.downloadResult.error))
                    : ""
            }

            // The CLI only downloads from a registry without TLS with
            // --no-tls: here, once the user accepts it.
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                type: Kirigami.MessageType.Warning
                visible: root.unconfirmedOperation !== ""
                text: root.hasTls === false
                      // The CLI's warning (lib/registry.sh), as a sentence.
                      ? i18n("Updates come from %1 without TLS: anyone on the network path could serve a different image.", root.registry)
                      : i18n("Couldn't find out whether updates come with a secure connection (TLS).")
                actions: [
                    Kirigami.Action {
                        text: i18n("Download Anyway")
                        onTriggered: root.startOperation(root.unconfirmedOperation)
                    },
                    Kirigami.Action {
                        text: i18n("Cancel")
                        onTriggered: root.unconfirmedOperation = ""
                    }
                ]
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                visible: !root.unconfirmedOperation

                PlasmaComponents3.Button {
                    visible: !!root.restartVersion
                    icon.name: "system-reboot"
                    text: i18n("Restart Now")
                    onClicked: root.runCommand(root.promptRestartCommand)
                }
                PlasmaComponents3.Button {
                    visible: !!root.downloadedVersion && !root.runningOperation
                    icon.name: "run-install"
                    text: i18n("Install")
                    onClicked: root.startOperation("install")
                }
                PlasmaComponents3.Button {
                    visible: root.canDownload
                    icon.name: "download"
                    text: i18n("Download")
                    onClicked: root.download("download")
                }
                PlasmaComponents3.Button {
                    visible: root.canDownload
                    icon.name: "run-install"
                    text: i18n("Download & Install")
                    onClicked: root.download("download-install")
                }
                PlasmaComponents3.Button {
                    visible: !root.availableVersion && !root.downloadedVersion && !root.restartVersion && !root.runningOperation
                    enabled: !root.isChecking
                    icon.name: "view-refresh"
                    text: i18n("Check for Updates")
                    onClicked: root.startCheck()
                }
            }
        }
    }

    readonly property bool canDownload: !!availableVersion && !downloadedVersion && !restartVersion && !runningOperation && !isChecking
    // The registry updates come from, for the TLS warning.
    property string registry: ""

    P5Support.DataSource {
        id: executable

        engine: "executable"
        onNewData: (command, output) => {
            executable.disconnectSource(command);
            if (command.startsWith(root.readCommand)) {
                root.showCheckResult(output["stdout"]);
            } else if (command.startsWith(root.readDownloadResultCommand)) {
                root.showDownloadResult(output["stdout"]);
            } else if (command.startsWith(root.readInstallResultCommand)) {
                root.showInstallResult(output["stdout"]);
            } else if (command.startsWith(root.readStatusCommand)) {
                root.showStatus(output["stdout"]);
            } else if (command.startsWith(root.operationsStateCommand)) {
                root.showOperationsState(output["stdout"]);
            } else if (command.startsWith(root.promptRestartCommand) || command.startsWith(root.startStatusCommand)) {
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
                if (root.shouldReopenPopup) {
                    root.expanded = true;
                }
                // Not started (the password dialog cancelled): as before.
                if (output["exit code"] !== 0) {
                    root.isChecking = false;
                }
            } else if (command.startsWith(root.startOperationCommand)) {
                root.isOperationStartPending = false;
                if (root.shouldReopenPopup) {
                    root.expanded = true;
                }
                if (output["exit code"] !== 0) {
                    root.runningOperation = "";
                }
            } else if (output["exit code"] === 0) {
                root.readResults();
                // An operation started elsewhere removes its result as it
                // starts: whether one runs.
                root.runCommand(root.operationsStateCommand);
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

    // While a download or an install runs: its progress, and its end.
    Timer {
        interval: 1000
        repeat: true
        running: !!root.runningOperation && !root.isOperationStartPending
        onTriggered: root.runCommand(root.operationsStateCommand)
    }

    Timer {
        id: retryTimer

        interval: 60000
        onTriggered: root.waitForChange()
    }

    // The tray's configure button, in the popup's header and the context
    // menu, opens System Settings > BrescaOS Updates.
    PlasmaCore.Action {
        id: configureAction

        icon.name: "configure"
        text: i18n("&Configure BrescaOS Updates…")
        onTriggered: root.openSettings()
    }

    Component.onCompleted: {
        Plasmoid.setInternalAction("configure", configureAction);
        runCommand(startStatusCommand);
        readResults();
        runCommand(operationsStateCommand);
        waitForChange();
    }

    Notification {
        id: updateNotification

        componentName: "brescaos-updates"
        eventId: "updateAvailable"
        iconName: "system-software-update"
        title: i18n("BrescaOS %1 is available", root.availableVersion)
        text: i18n("Install it from System Settings.")
        flags: Notification.Persistent
        actions: [
            NotificationAction {
                label: i18n("Install")
                onActivated: root.openSettings()
            }
        ]
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
                onActivated: root.startOperation("install")
            }
        ]
    }

    Notification {
        id: installedNotification

        componentName: "brescaos-updates"
        eventId: "updateInstalled"
        iconName: "system-reboot"
        title: i18n("BrescaOS %1 is installed", root.restartVersion)
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

    function openSettings() {
        KCM.KCMLauncher.openSystemSettings("kcm_brescaos_updates");
    }

    function readResults() {
        readCheckResult();
        runCommand(readDownloadResultCommand);
        runCommand(readInstallResultCommand);
        runCommand(readStatusCommand);
    }

    // The result in resultJson, null when there's none (cat found no file).
    function parseResult(resultJson) {
        try {
            return JSON.parse(resultJson);
        } catch (error) {
            return null;
        }
    }

    // The last line of an operation's error: bootc's own message.
    function lastLine(text) {
        return text.trim().split("\n").pop();
    }

    // Notified only when it's new: a version downloaded without installing it
    // (download-install's has download_only false, its install follows).
    function showDownloadResult(resultJson) {
        downloadResult = parseResult(resultJson);
        const downloadedAt = downloadResult?.downloaded_at ?? "";
        if (lastDownloadedAt !== undefined && downloadedAt !== lastDownloadedAt && downloadedVersion) {
            updateNotification.close();
            downloadedNotification.sendEvent();
        }
        lastDownloadedAt = downloadedAt;
    }

    function showInstallResult(resultJson) {
        installResult = parseResult(resultJson);
        const installedAt = installResult?.installed_at ?? "";
        if (lastInstalledAt !== undefined && installedAt !== lastInstalledAt && restartVersion) {
            updateNotification.close();
            downloadedNotification.close();
            installedNotification.sendEvent();
        }
        lastInstalledAt = installedAt;
    }

    function showStatus(statusJson) {
        const status = parseResult(statusJson);
        hasTls = status?.registry?.tls;
        registry = status?.registry?.registry ?? "";
    }

    // operationsStateCommand's output: one state per operation, then the
    // progress, if any.
    function showOperationsState(output) {
        const lines = output.split("\n");
        const running = operations.find((operation, index) => lines[index] === "activating") ?? "";
        progress = running ? parseResult(lines.slice(operations.length).join("\n")) : null;
        if (running !== runningOperation && !isOperationStartPending) {
            // One that ended: its result.
            if (runningOperation && !running) {
                readResults();
            }
            runningOperation = running;
        }
    }

    // The CLI only downloads from a registry without TLS with --no-tls: here,
    // once the user accepts it; also when it's unknown.
    function download(operation) {
        if (hasTls === true) {
            startOperation(operation);
        } else {
            unconfirmedOperation = operation;
        }
    }

    function startOperation(operation) {
        unconfirmedOperation = "";
        runningOperation = operation;
        progress = null;
        // Until the download removes it: not the last one's.
        if (operation !== "install") {
            downloadResult = null;
        }
        isOperationStartPending = true;
        shouldReopenPopup = expanded;
        runCommand(startOperationCommand + operation + ".service");
    }

    function waitForChange() {
        runCommand(waitForChangeCommand);
    }

    function startCheck() {
        isChecking = true;
        isStartPending = true;
        shouldReopenPopup = expanded;
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
