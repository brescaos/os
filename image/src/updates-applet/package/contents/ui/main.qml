// The system tray's icon for BrescaOS versions (../../../README.md): what the
// bresca-updates@ operations (image/src/bresca-cli/) leave in
// /run/bresca/updates/: check.json (a version available), download.json (one
// downloaded) and install.json (one installed). It shows only with one of
// them, a click opens System Settings > BrescaOS Updates, and it notifies
// each. The files are read only when they change: inotifywait waits for that
// asleep in the kernel.
pragma ComponentBehavior: Bound

import QtQuick
import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.notification
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: root

    // Created at boot (bresca-updates.tmpfiles), so it can be watched before
    // the first check writes check.json.
    readonly property string resultsFolder: "/run/bresca/updates"
    readonly property string readCheckResultCommand: "cat " + resultsFolder + "/check.json"
    readonly property string readDownloadResultCommand: "cat " + resultsFolder + "/download.json"
    readonly property string readInstallResultCommand: "cat " + resultsFolder + "/install.json"
    // Ends when an operation's result is moved into place or removed (as the
    // operation starts; check.json by an install too).
    readonly property string waitForChangeCommand: "inotifywait -qq -e moved_to,delete --include '(check|download|install)\\.json$' " + resultsFolder
    // Plasma's restart prompt, as Restart in its menu.
    readonly property string promptRestartCommand: "gdbus call --session --dest org.kde.LogoutPrompt --object-path /LogoutPrompt --method org.kde.LogoutPrompt.promptReboot"
    // The version the last check found, empty when there's none.
    property string availableVersion: ""
    // What it downloads, as the user's locale writes it; empty when the check
    // didn't say.
    property string availableDownloadSize: ""
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

    // The version installed and waiting for a restart, empty when there's
    // none: /run/bresca/updates/ is emptied at boot.
    readonly property string restartVersion: installResult?.installed_version && !installResult.error ? installResult.installed_version : ""
    // The version downloaded without installing it, empty when there's none
    // or it's installed since.
    readonly property string downloadedVersion: downloadResult?.download_only && downloadResult.update_available && !downloadResult.error
                                                && !restartVersion ? downloadResult.downloaded_version : ""

    // Not in the tray at all while there's nothing to do. A version being
    // downloaded or installed is still the available one: check.json stays
    // until its install succeeds.
    Plasmoid.status: availableVersion || downloadedVersion || restartVersion
                     ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.HiddenStatus
    toolTipMainText: i18n("BrescaOS Updates")
    toolTipSubText: restartVersion ? i18n("BrescaOS %1 is installed", restartVersion)
                  : downloadedVersion ? i18n("BrescaOS %1 is downloaded", downloadedVersion)
                  : i18n("BrescaOS %1 is available", availableVersion)

    // No popup: System Settings > BrescaOS Updates does everything. Plasma
    // only creates the icon for an applet with a full representation, so an
    // empty one, never shown.
    preferredRepresentation: compactRepresentation
    fullRepresentation: Item {}
    // BrescaOS Updates' symbol (../icons/), in the panel's text color, with
    // the seal of the KCM's state icons at its top right, in the logo's amber.
    compactRepresentation: MouseArea {
        hoverEnabled: true
        onClicked: root.openSettings()

        Kirigami.Icon {
            anchors.fill: parent
            isMask: true
            color: Kirigami.Theme.textColor
            source: root.restartVersion ? Qt.resolvedUrl("../icons/restart-sealed-symbolic.svg")
                                        : Qt.resolvedUrl("../icons/updates-sealed-symbolic.svg")
        }
        Kirigami.Icon {
            anchors.fill: parent
            isMask: true
            color: "#e3a12f"
            source: Qt.resolvedUrl("../icons/seal.svg")
        }
    }

    P5Support.DataSource {
        id: executable

        engine: "executable"
        onNewData: (command, output) => {
            executable.disconnectSource(command);
            if (command.startsWith(root.readCheckResultCommand)) {
                root.showCheckResult(output["stdout"]);
            } else if (command.startsWith(root.readDownloadResultCommand)) {
                root.showDownloadResult(output["stdout"]);
            } else if (command.startsWith(root.readInstallResultCommand)) {
                root.showInstallResult(output["stdout"]);
            } else if (command.startsWith(root.promptRestartCommand)) {
                // Nothing to do with its output.
            } else if (output["exit code"] === 0) {
                // Later, never from inside this handler.
                Qt.callLater(root.waitForChange);
            } else {
                // It couldn't watch (no folder): no retrying in a loop.
                retryTimer.start();
            }
        }
    }

    // The results are read a moment after each wait starts, not when the last
    // one ends: by then the new wait is watching, so a result that came in
    // between (an install writes install.json and removes check.json at once)
    // isn't missed, and several changes are read once.
    Timer {
        id: readTimer

        interval: 500
        onTriggered: root.readResults()
    }

    Timer {
        id: retryTimer

        interval: 60000
        onTriggered: root.waitForChange()
    }

    // The tray's configure action, in the context menu, opens System Settings >
    // BrescaOS Updates too, not an empty settings dialog.
    PlasmaCore.Action {
        id: configureAction

        icon.name: "configure"
        text: i18n("&Configure BrescaOS Updates…")
        onTriggered: root.openSettings()
    }

    Component.onCompleted: {
        Plasmoid.setInternalAction("configure", configureAction);
        waitForChange();
    }

    Notification {
        id: updateNotification

        componentName: "brescaos-updates"
        eventId: "updateAvailable"
        iconName: "system-software-update"
        title: i18n("BrescaOS %1 is available", root.availableVersion)
        text: root.availableDownloadSize ? i18n("Download size: %1. Install it from System Settings.", root.availableDownloadSize)
                                         : i18n("Install it from System Settings.")
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
                // The module installs it (its "install" argument).
                onActivated: root.openSettings(["install"])
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

    function openSettings(moduleArguments = []) {
        KCM.KCMLauncher.openSystemSettings("kcm_brescaos_updates", moduleArguments);
    }

    function waitForChange() {
        runCommand(waitForChangeCommand);
        readTimer.restart();
    }

    function readResults() {
        runCommand(readCheckResultCommand);
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

    // No file (a check removed it as it started, or an install did) and a
    // failed check (no network) keep the version shown.
    function showCheckResult(resultJson) {
        const checkResult = parseResult(resultJson);
        if (!checkResult || checkResult.error !== undefined) {
            return;
        }
        availableVersion = checkResult.update_available ? checkResult.available_version : "";
        availableDownloadSize = checkResult.update_available && checkResult.download_size
                                ? Qt.locale().formattedDataSize(checkResult.download_size, 1, Locale.DataSizeSIFormat) : "";
        if (availableVersion && availableVersion !== notifiedVersion) {
            notifiedVersion = availableVersion;
            updateNotification.sendEvent();
        }
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
}
