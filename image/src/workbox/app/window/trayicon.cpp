#include "trayicon.h"

#include "mainwindow.h"
#include "workboxdelegate.h"
#include "workboxmodel.h"

#include <KLocalizedString>
#include <KStatusNotifierItem>

#include <QApplication>
#include <QMenu>
#include <QPainter>

namespace
{

// The number of running workboxes ("9+" from ten on) in a pill of the
// palette's highlight, with a ring of its window color that sets it apart
// from the toolbox's lines. The tray draws it on the icon's corner.
QIcon runningCountBadge(int runningCount, const QPalette &palette)
{
    const QString text = runningCount > 9 ? QStringLiteral("9+") : QString::number(runningCount);
    QIcon badge;
    for (const int size : {16, 22, 32, 44}) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QFont font = QApplication::font();
        font.setPixelSize(qRound(size * 0.55));
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        const int height = qRound(size * 0.8);
        const int width = qMin(size, qMax(height, painter.fontMetrics().horizontalAdvance(text) + height / 2));
        const QRectF pill(size - width + 0.5, size - height + 0.5, width - 1, height - 1);
        painter.setPen(QPen(palette.color(QPalette::Window), 1));
        painter.setBrush(palette.color(QPalette::Highlight));
        painter.drawRoundedRect(pill, pill.height() / 2, pill.height() / 2);
        painter.setPen(palette.color(QPalette::HighlightedText));
        painter.drawText(pill, Qt::AlignCenter, text);
        painter.end();
        badge.addPixmap(pixmap);
    }
    return badge;
}

QIcon swatchIcon(const QColor &workboxColor)
{
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    paintSwatch(&painter, pixmap.rect(), workboxColor);
    painter.end();
    return QIcon(pixmap);
}

}

TrayIcon::TrayIcon(WorkboxModel &model, MainWindow &window, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_window(window)
    , m_item(new KStatusNotifierItem(QStringLiteral("com.brescaos.workbox"), this))
{
    m_item->setCategory(KStatusNotifierItem::ApplicationStatus);
    m_item->setStatus(KStatusNotifierItem::Active);
    m_item->setTitle(i18nc("@title the app's name", "Workbox"));
    m_item->setIconByName(QStringLiteral("com.brescaos.workbox-symbolic"));
    m_item->setToolTipIconByName(QStringLiteral("com.brescaos.workbox"));
    m_item->setToolTipTitle(i18nc("@title the app's name", "Workbox"));
    // Its own Quit, at the end of its menu, which it fills itself.
    m_item->setStandardActionsEnabled(false);
    connect(m_item, &KStatusNotifierItem::activateRequested, this, &TrayIcon::showWindow);

    connect(&m_model, &WorkboxModel::runningCountChanged, this, &TrayIcon::updateRunningCount);
    // Queued: a menu's action that starts or stops a workbox changes the
    // list while it's still being triggered, and the menu is filled again.
    for (const auto signal : {&QAbstractItemModel::rowsInserted, &QAbstractItemModel::rowsRemoved}) {
        connect(&m_model, signal, this, &TrayIcon::updateMenu, Qt::QueuedConnection);
    }
    connect(&m_model, &QAbstractItemModel::dataChanged, this, &TrayIcon::updateMenu, Qt::QueuedConnection);
    connect(&m_model, &WorkboxModel::problemChanged, this, &TrayIcon::updateMenu, Qt::QueuedConnection);
    qApp->installEventFilter(this);

    updateRunningCount();
    updateMenu();
}

bool TrayIcon::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == qApp && event->type() == QEvent::ApplicationPaletteChange) {
        updateRunningCount();
    }
    return QObject::eventFilter(watched, event);
}

void TrayIcon::updateRunningCount()
{
    const int runningCount = m_model.runningCount();
    m_item->setOverlayIconByPixmap(runningCount > 0 ? runningCountBadge(runningCount, QApplication::palette()) : QIcon());
    m_item->setToolTipSubTitle(i18np("%1 workbox running", "%1 workboxes running", runningCount));
}

// Each workbox's submenu, with its color: Start or Stop, and Open a Terminal,
// disabled while an operation is under way. Removing is only in the window,
// which asks to type the id first.
void TrayIcon::updateMenu()
{
    QMenu *menu = m_item->contextMenu();
    // Later: one of their actions may be the one being triggered.
    for (QMenu *submenu : menu->findChildren<QMenu *>(Qt::FindDirectChildrenOnly)) {
        submenu->deleteLater();
    }
    menu->clear();

    if (m_model.rowCount() == 0) {
        const QString text = m_model.problem().isEmpty() ? i18nc("@info in the tray's menu", "No workboxes") : m_model.problem();
        menu->addAction(text)->setEnabled(false);
    }
    for (int row = 0; row < m_model.rowCount(); ++row) {
        const QModelIndex index = m_model.index(row);
        const Workbox workbox = index.data(WorkboxModel::WorkboxRole).value<Workbox>();
        const bool isIdle = static_cast<WorkboxModel::Operation>(index.data(WorkboxModel::OperationRole).toInt()) == WorkboxModel::Operation::None;
        QMenu *submenu = menu->addMenu(swatchIcon(workbox.color), workbox.name);
        submenu->setToolTipsVisible(true);
        submenu->menuAction()->setToolTip(statusText(index));

        QAction *startOrStop = workbox.isRunning
            ? submenu->addAction(QIcon::fromTheme(QStringLiteral("media-playback-stop")), i18nc("@action:inmenu a workbox", "Stop"))
            : submenu->addAction(QIcon::fromTheme(QStringLiteral("media-playback-start")), i18nc("@action:inmenu a workbox", "Start"));
        startOrStop->setEnabled(isIdle);
        connect(startOrStop, &QAction::triggered, this, [this, id = workbox.id, isRunning = workbox.isRunning] {
            if (isRunning) {
                m_model.stop(id);
            } else {
                m_model.start(id);
            }
        });

        QAction *openTerminal = submenu->addAction(QIcon::fromTheme(QStringLiteral("utilities-terminal")), i18nc("@action:inmenu", "Open a Terminal"));
        openTerminal->setEnabled(isIdle);
        connect(openTerminal, &QAction::triggered, this, [this, id = workbox.id] {
            m_window.openTerminal(id);
            showWindow();
        });
    }

    menu->addSeparator();
    QAction *quit = menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), i18nc("@action:inmenu", "Quit"));
    connect(quit, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void TrayIcon::showWindow()
{
    // On Wayland a window takes the focus only with the token the tray got
    // for the click; Qt's activateWindow() takes it from XDG_ACTIVATION_TOKEN.
    const QString token = m_item->providedToken();
    if (!token.isEmpty()) {
        qputenv("XDG_ACTIVATION_TOKEN", token.toUtf8());
    }
    m_window.setWindowState(m_window.windowState() & ~Qt::WindowMinimized);
    m_window.show();
    m_window.raise();
    m_window.activateWindow();
}
