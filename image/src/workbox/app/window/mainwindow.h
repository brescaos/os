// The app's window (README.md): the workboxes on the left; on the right, the
// selected one with its buttons (start, stop, open in Konsole, remove), its
// terminals and its information; Create opens its dialog. Nothing blocks: everything goes through
// WorkboxModel's jobs. The list is the app's, which the tray icon shares.
#pragma once

#include "workboxmodel.h"

#include <QHash>
#include <QMainWindow>
#include <QSet>

class KMessageWidget;
class QLabel;
class QListView;
class QPushButton;
class QSortFilterProxyModel;
class QStackedWidget;
class QTabWidget;
class WorkboxTerminals;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(WorkboxModel &model, QWidget *parent = nullptr);

    // Selects the workbox id, on its terminals, and opens a shell in it,
    // starting it first if it's stopped.
    void openTerminal(const QString &id);

private:
    QWidget *createSidebar();
    QWidget *createWorkboxPage();
    QWidget *createTerminalTab();
    QWidget *createInformationTab();
    // A Create button, enabled while workbox can run (no problem).
    QPushButton *createCreateButton(const QString &text, const QString &objectName);

    // The selected workbox's row; invalid when there's none.
    QModelIndex selectedIndex() const;
    // The first row when the selected one has gone.
    void keepASelection();
    void select(const QString &id);
    void showSelected();
    void showTerminals(const Workbox &workbox);
    void showInformation(const Workbox &workbox);

    // Starts it, and opens a shell once it runs.
    void startSelected();
    void openSelectedInKonsole();
    void enableCreating();
    void openCreateDialog();
    // Asks to type its id first.
    void confirmRemoval(const Workbox &workbox);
    // Konsole's failure, over the selected workbox.
    void showError(const QString &errorText);

    WorkboxModel &m_model;
    QSortFilterProxyModel *m_filter;
    QListView *m_list;
    QLabel *m_countLabel;
    QList<QPushButton *> m_createButtons;
    QString m_selectedId;
    // Why Konsole couldn't open a terminal, until another workbox is
    // selected.
    QString m_shellError;
    // Workboxes to open a shell in once they run.
    QSet<QString> m_shouldOpenShell;
    QHash<QString, WorkboxTerminals *> m_terminals;

    QStackedWidget *m_content;
    QWidget *m_emptyPage;
    QLabel *m_problemLabel;
    QWidget *m_workboxPage;

    QLabel *m_swatch;
    QLabel *m_nameLabel;
    QLabel *m_detailLabel;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QPushButton *m_openInKonsoleButton;
    QPushButton *m_removeButton;
    KMessageWidget *m_errorMessage;
    QTabWidget *m_tabs;

    QStackedWidget *m_terminalStack;
    QWidget *m_stoppedPage;
    QLabel *m_stoppedTitle;
    QWidget *m_noShellPage;

    QLabel *m_imageLabel;
    QLabel *m_homeLabel;
    QLabel *m_colorLabel;
    QLabel *m_sudoLabel;
    QLabel *m_waylandLabel;
    QLabel *m_volumesLabel;
};
