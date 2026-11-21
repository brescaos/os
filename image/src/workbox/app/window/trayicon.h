// The app's icon in the system tray (README.md), there while the app runs: the
// toolbox, with how many workboxes run in a badge on its corner, and a menu to
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

protected:
    // The badge again in the new palette's colors.
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateRunningCount();
    void updateMenu();
    void showWindow();

    WorkboxModel &m_model;
    MainWindow &m_window;
    KStatusNotifierItem *m_item;
};
