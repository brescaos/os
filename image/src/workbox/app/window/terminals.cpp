#include "terminals.h"

#include "terminalcolors.h"
#include "workboxmodel.h"

#include <KLocalizedString>
#include <KParts/PartLoader>
#include <KParts/ReadOnlyPart>
#include <KPluginMetaData>
#include <kde_terminal_interface.h>

#include <QToolButton>

QStringList commandLineWithColors(const QColor &workboxColor, const QStringList &commandLine)
{
    // The sequence is sh's first argument, so nothing in it is read as a
    // command; "sh" is its $0.
    return QStringList{QStringLiteral("/bin/sh"),
                       QStringLiteral("-c"),
                       QStringLiteral("printf '%s' \"$1\"; shift; exec \"$@\""),
                       QStringLiteral("sh"),
                       colorsSequence(terminalColorsFor(workboxColor))}
    + commandLine;
}

WorkboxTerminals::WorkboxTerminals(WorkboxModel &model, const QString &id, const QColor &workboxColor, QWidget *parent)
    : QTabWidget(parent)
    , m_model(model)
    , m_id(id)
    , m_workboxColor(workboxColor)
{
    setDocumentMode(true);
    setTabsClosable(true);
    setMovable(true);
    // Konsole's part goes with its widget, and its shell with it.
    connect(this, &QTabWidget::tabCloseRequested, this, [this](int index) {
        delete widget(index);
    });

    auto *newShellButton = new QToolButton(this);
    newShellButton->setObjectName(QStringLiteral("newShellButton"));
    newShellButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    newShellButton->setAutoRaise(true);
    newShellButton->setToolTip(i18nc("@info:tooltip", "New terminal"));
    newShellButton->setAccessibleName(i18nc("@action:button", "New terminal"));
    connect(newShellButton, &QToolButton::clicked, this, &WorkboxTerminals::openShell);
    setCornerWidget(newShellButton, Qt::TopRightCorner);
}

void WorkboxTerminals::openShell()
{
    const auto loaded = KParts::PartLoader::instantiatePart<KParts::ReadOnlyPart>(KPluginMetaData(QStringLiteral("kf6/parts/konsolepart")), this, this);
    auto *terminal = qobject_cast<TerminalInterface *>(loaded.plugin);
    if (!terminal) {
        delete loaded.plugin;
        Q_EMIT failed(i18n("Konsole's terminal couldn't be loaded: %1", loaded.errorText));
        return;
    }
    ++m_openedShells;
    const QString title = m_openedShells == 1 ? i18nc("@title:tab a terminal", "Shell") : i18nc("@title:tab a terminal", "Shell %1", m_openedShells);
    // Konsole's part deletes itself and its widget when its shell ends, and
    // the tab goes with the widget.
    QWidget *shell = loaded.plugin->widget();
    setCurrentIndex(addTab(shell, title));
    const QStringList commandLine = commandLineWithColors(m_workboxColor, m_model.shellCommandLine(m_id));
    terminal->startProgram(commandLine.first(), commandLine);
    shell->setFocus();
}

void WorkboxTerminals::tabInserted(int index)
{
    QTabWidget::tabInserted(index);
    Q_EMIT shellsChanged();
}

void WorkboxTerminals::tabRemoved(int index)
{
    QTabWidget::tabRemoved(index);
    Q_EMIT shellsChanged();
}
