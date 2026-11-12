// System Settings > BrescaOS Updates. What it shows comes from the results
// of the bresca-updates@ operations, which the module (kcm.cpp) reads and
// starts; this page only decides what to show from them.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Shapes
import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

KCM.SimpleKCM {
    id: page

    // The module (kcm.cpp), which System Settings sets as the context
    // property kcm: qmllint can't see it.
    readonly property var module: kcm // qmllint disable unqualified
    readonly property var results: module.results
    readonly property var running: module.runningOperations
    readonly property var status: results.status ?? null
    readonly property var check: results.check ?? null
    readonly property var downloadProgress: results["download-progress"] ?? null
    readonly property bool hasTls: status?.registry?.tls ?? true
    readonly property string newVersion: check?.update_available ? check.available_version : ""

    // What the page shows: the first that applies.
    readonly property string view: {
        if (running.includes("download") || (running.includes("download-install") && downloadProgress)) {
            return "downloading";
        }
        if (running.includes("download-install") || running.includes("install")) {
            return "installing";
        }
        if (running.includes("rollback")) {
            return "rollingBack";
        }
        if (!status) {
            return "loading";
        }
        if (status.rollback_queued) {
            return "rollbackQueued";
        }
        if (status.staged) {
            return status.staged.download_only ? "downloaded" : "installed";
        }
        if (running.includes("check")) {
            return "checking";
        }
        if (check?.update_available) {
            return "available";
        }
        return check && !check.error ? "upToDate" : "unknown";
    }
    readonly property bool isBusy: ["downloading", "installing", "rollingBack", "loading", "checking"].includes(view)
    readonly property bool isRestartPending: view === "installed" || view === "rollbackQueued"
    readonly property bool canGoBack: !!status?.rollback && !status.staged && !status.rollback_queued && !isBusy

    // The last line of an operation's error: bootc's own message.
    function lastLine(text) {
        return text.trim().split("\n").pop();
    }

    function formatTime(isoTime) {
        return new Date(isoTime).toLocaleString(Qt.locale(), Locale.ShortFormat);
    }

    // install asked for (installRequested): done once the page knows what
    // to show, with the TLS question when it applies.
    property bool isInstallPending: false

    function installIfPending() {
        if (!isInstallPending || view === "loading" || view === "checking") {
            return;
        }
        isInstallPending = false;
        if (view === "available") {
            download("download-install");
        } else if (view === "downloaded") {
            module.start("install");
        }
    }

    onViewChanged: installIfPending()

    Connections {
        target: page.module

        function onInstallRequested() {
            page.isInstallPending = true;
            page.installIfPending();
        }
    }

    // The CLI only downloads from a registry without TLS with --no-tls: here,
    // once the user accepts it.
    function download(operation) {
        if (hasTls) {
            module.start(operation);
        } else {
            noTlsDialog.operation = operation;
            noTlsDialog.open();
        }
    }

    // The previous version, pinned to the bottom of the window.
    footer: ColumnLayout {
        visible: page.canGoBack || page.view === "installed"
        spacing: 0

        Kirigami.Separator {
            Layout.fillWidth: true
        }
        RowLayout {
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                QQC2.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: page.view === "installed"
                          ? i18n("Previous version: BrescaOS %1 after restarting", page.status?.current?.version ?? "")
                          : i18n("Previous version: BrescaOS %1", page.status?.rollback?.version ?? "")
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    font: Kirigami.Theme.smallFont
                    text: page.view === "installed"
                          ? i18n("Kept so you can go back to it if %1 doesn't work well.", page.status?.staged?.version ?? "")
                          : i18n("Kept in case this version doesn't work well.")
                }
            }
            QQC2.Button {
                visible: page.canGoBack
                icon.name: "edit-undo"
                text: i18n("Go Back…")
                onClicked: goBackDialog.open()
            }
        }
    }

    actions: [
        Kirigami.Action {
            icon.name: "view-refresh"
            text: i18n("Check for Updates")
            enabled: !page.isBusy
            visible: !page.isRestartPending
            onTriggered: page.module.start("check")
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing * 3

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Kirigami.InlineMessage {
                Layout.fillWidth: true
                type: Kirigami.MessageType.Error
                visible: !!page.check?.error && page.view !== "checking"
                text: i18n("Couldn't check for updates: %1", page.lastLine(page.check?.error ?? ""))
                actions: [
                    Kirigami.Action {
                        text: i18n("Try Again")
                        onTriggered: page.module.start("check")
                    }
                ]
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                type: Kirigami.MessageType.Error
                visible: !!page.results.download?.error && page.view !== "downloading"
                text: i18n("Couldn't download the new version: %1", page.lastLine(page.results.download?.error ?? ""))
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                type: Kirigami.MessageType.Error
                visible: !!page.results.install?.error && page.view !== "installing"
                text: i18n("Couldn't install the new version: %1", page.lastLine(page.results.install?.error ?? ""))
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                type: Kirigami.MessageType.Error
                visible: !!page.results.rollback?.error && page.view !== "rollingBack"
                text: i18n("Couldn't go back to the previous version: %1", page.lastLine(page.results.rollback?.error ?? ""))
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                type: Kirigami.MessageType.Warning
                visible: !page.hasTls
                // The CLI's warning (lib/registry.sh), as a sentence.
                text: i18n("Updates come from %1 without TLS: anyone on the network path could serve a different image.",
                           page.status?.registry?.registry ?? "")
            }
        }

        // The state: an icon, what's going on and its action.
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: Kirigami.Units.gridUnit * 26
            spacing: Kirigami.Units.smallSpacing

            // A spinning arc, as big as the icons' circle: Breeze's
            // BusyIndicator is a turning gear. RotationAnimator turns it on
            // the render thread, only while it shows.
            Shape {
                id: spinner

                readonly property real lineWidth: Kirigami.Units.largeSpacing

                Kirigami.Theme.colorGroup: Kirigami.Theme.Active
                Kirigami.Theme.inherit: false
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: Kirigami.Units.iconSizes.huge
                implicitHeight: Kirigami.Units.iconSizes.huge
                visible: page.view === "loading" || page.view === "checking" || page.view === "rollingBack"
                preferredRendererType: Shape.CurveRenderer

                ShapePath {
                    fillColor: "transparent"
                    strokeColor: Kirigami.Theme.highlightColor
                    strokeWidth: spinner.lineWidth
                    capStyle: ShapePath.RoundCap

                    PathAngleArc {
                        centerX: spinner.width / 2
                        centerY: spinner.height / 2
                        radiusX: (spinner.width - spinner.lineWidth) / 2
                        radiusY: radiusX
                        startAngle: -90
                        sweepAngle: 270
                    }
                }
                RotationAnimator on rotation {
                    from: 0
                    to: 360
                    duration: 1000
                    loops: Animation.Infinite
                    running: spinner.visible
                }
            }
            // Breeze has no big colored icons for these: a symbolic one on a
            // circle of the theme's color.
            Rectangle {
                // Breeze's inactive colors (an unfocused window) turn the
                // highlight's text dark: keep the active ones.
                Kirigami.Theme.colorGroup: Kirigami.Theme.Active
                Kirigami.Theme.inherit: false
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: Kirigami.Units.iconSizes.huge
                implicitHeight: Kirigami.Units.iconSizes.huge
                radius: width / 2
                visible: !["loading", "checking", "rollingBack"].includes(page.view)
                // Green when there's nothing left to fetch (up to date, or
                // downloaded and waiting to be installed); blue while there's
                // something to do.
                color: page.view === "upToDate" || page.view === "downloaded" || page.view === "installing" ? Kirigami.Theme.positiveTextColor
                     : page.view === "unknown" ? Kirigami.Theme.disabledTextColor
                     : Kirigami.Theme.highlightColor

                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: Kirigami.Units.iconSizes.medium
                    height: Kirigami.Units.iconSizes.medium
                    isMask: true
                    color: Kirigami.Theme.highlightedTextColor
                    source: page.view === "upToDate" ? "checkmark"
                          : page.view === "unknown" ? "question"
                          : page.isRestartPending ? "system-reboot"
                          : page.view === "downloaded" || page.view === "installing" ? "run-install"
                          : "download"
                }
            }
            Kirigami.Heading {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                level: 2
                visible: text !== ""
                text: {
                    switch (page.view) {
                    case "checking":
                        return i18n("Checking for updates…");
                    case "upToDate":
                        return i18n("BrescaOS is up to date");
                    case "available":
                        return i18n("BrescaOS %1 is available", page.newVersion);
                    case "downloading":
                        return page.newVersion ? i18n("Downloading BrescaOS %1", page.newVersion) : i18n("Downloading the new version");
                    case "installing":
                        return page.newVersion ? i18n("Installing BrescaOS %1", page.newVersion) : i18n("Installing the new version");
                    case "downloaded":
                        return i18n("BrescaOS %1 is downloaded", page.status.staged.version);
                    case "installed":
                        return i18n("BrescaOS %1 is ready", page.status.staged.version);
                    case "rollingBack":
                        return i18n("Preparing the previous version…");
                    case "rollbackQueued":
                        return i18n("BrescaOS %1 starts after restarting", page.status.rollback.version);
                    case "unknown":
                        return i18n("BrescaOS %1", page.status.current.version);
                    default:
                        return "";
                    }
                }
            }
            QQC2.ProgressBar {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.largeSpacing
                visible: page.view === "downloading" || page.view === "installing"
                from: 0
                to: 100
                value: page.downloadProgress?.percent ?? 0
                indeterminate: page.view === "installing" || page.downloadProgress?.percent === undefined
            }
            QQC2.Label {
                Layout.alignment: Qt.AlignHCenter
                visible: page.view === "downloading" || page.view === "installing"
                // The download's phase (download-progress.json, as the CLI
                // shows it), with its percent when bootc gives one; bootc
                // gives no progress while installing.
                text: {
                    const percent = page.downloadProgress?.percent;
                    if (page.view === "installing") {
                        return i18n("Installing…");
                    }
                    switch (page.downloadProgress?.phase) {
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
            }
            QQC2.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: text !== ""
                text: {
                    switch (page.view) {
                    case "upToDate":
                        return i18n("Last checked %1.", page.formatTime(page.check.checked_at));
                    case "unknown":
                        return page.check?.checked_at ? i18n("Last checked %1.", page.formatTime(page.check.checked_at)) : i18n("Not checked for updates yet.");
                    case "available":
                        return i18n("It downloads in the background while you keep working, and starts the next time you restart.");
                    case "downloaded":
                        return i18n("Install it before restarting: restarting discards the download.");
                    case "installed":
                    case "rollbackQueued":
                        return i18n("Restart to start using it. Your files, apps and settings stay as they are.");
                    default:
                        return "";
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.largeSpacing
                visible: page.view === "available"
                spacing: 0

                QQC2.Button {
                    icon.name: "download"
                    text: i18n("Download and Install")
                    onClicked: page.download("download-install")
                }
                QQC2.Button {
                    icon.name: "arrow-down"
                    display: QQC2.AbstractButton.IconOnly
                    text: i18n("More install options")
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                    onClicked: installOptions.popup(this, 0, height)

                    QQC2.Menu {
                        id: installOptions

                        QQC2.MenuItem {
                            icon.name: "download"
                            text: i18n("Download Only")
                            onTriggered: page.download("download")
                        }
                    }
                }
            }
            QQC2.Button {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.largeSpacing
                visible: page.view === "downloaded"
                icon.name: "run-install"
                text: i18n("Install")
                onClicked: page.module.start("install")
            }
            QQC2.Button {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.largeSpacing
                visible: page.isRestartPending
                icon.name: "system-reboot"
                text: i18n("Restart Now")
                onClicked: page.module.restart()
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
            visible: !!page.status
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true
            visible: !!page.status

            QQC2.Label {
                Kirigami.FormData.label: i18n("Installed version:")
                text: page.view === "installed"
                      ? i18n("BrescaOS %1 (%2 after restarting)", page.status?.current?.version ?? "", page.status.staged.version)
                      : i18n("BrescaOS %1", page.status?.current?.version ?? "")
            }
            RowLayout {
                Kirigami.FormData.label: i18n("Updates from:")
                visible: !!page.status?.registry

                QQC2.Label {
                    font.family: Kirigami.Theme.fixedWidthFont.family
                    text: page.status?.registry?.image ?? ""
                }
                Kirigami.Icon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.small
                    Layout.preferredHeight: Kirigami.Units.iconSizes.small
                    visible: !page.hasTls
                    source: "data-warning"
                }
            }
            // Only one channel for now.
            QQC2.Label {
                Kirigami.FormData.label: i18n("Channel:")
                text: i18n("Stable")
            }

        }
    }

    Kirigami.PromptDialog {
        id: goBackDialog

        title: i18n("Go back to BrescaOS %1?", page.status?.rollback?.version ?? "")
        subtitle: i18n("The computer will start BrescaOS %1 the next time you restart. Your files, apps and settings stay as they are. You can update to a newer version again from here afterwards.",
                       page.status?.rollback?.version ?? "")
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                icon.name: "edit-undo"
                text: i18n("Go Back")
                onTriggered: {
                    page.module.start("rollback");
                    goBackDialog.close();
                }
            }
        ]
    }

    Kirigami.PromptDialog {
        id: noTlsDialog

        // download or download-install, once accepted.
        property string operation

        title: i18n("Download without a secure connection?")
        subtitle: i18n("Updates come from %1 without TLS: anyone on the network path could serve a different image. Accept it only on a development install.",
                       page.status?.registry?.registry ?? "")
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                icon.name: "data-warning"
                text: i18n("Download Anyway")
                onTriggered: {
                    page.module.start(noTlsDialog.operation);
                    noTlsDialog.close();
                }
            }
        ]
    }
}
