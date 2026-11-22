// The app's icon in the system tray (README.md), there while the app runs: the
// toolbox, with a dot on its corner while any workbox runs, and a menu to
// start, stop and open a terminal in each. A click shows the window.
#pragma once

#include <QObject>

class KStatusNotifierItem;
class MainWindow;
class WorkboxModel;

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    TrayIcon(WorkboxModel &model, MainWindow &window, QObject *parent = nullptr);

    KStatusNotifierItem *item() const
    {
        return m_item;
    }

private:
    // By name, never a pixmap, so the tray colors it as its other symbols.
    void updateRunningCount();
    void updateMenu();
    void showWindow();

    WorkboxModel &m_model;
    MainWindow &m_window;
    KStatusNotifierItem *m_item;
};
