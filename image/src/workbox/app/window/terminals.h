// A workbox's terminals in the window (README.md): tabs of Konsole's part,
// each a login shell in the workbox with its colors. A tab goes when its
// shell ends: the user's exit, or the workbox stopped.
#pragma once

#include <QColor>
#include <QTabWidget>

class WorkboxModel;

// commandLine, run by sh once it has set the terminal's background and text
// to those of a workbox of workboxColor (terminalcolors.h), as workbox enter
// does in a terminal.
QStringList commandLineWithColors(const QColor &workboxColor, const QStringList &commandLine);

class WorkboxTerminals : public QTabWidget
{
    Q_OBJECT

public:
    WorkboxTerminals(WorkboxModel &model, const QString &id, const QColor &workboxColor, QWidget *parent = nullptr);

    // A new tab with a shell, which gets the focus.
    void openShell();

Q_SIGNALS:
    // A tab came or went.
    void shellsChanged();
    // Konsole's part couldn't be loaded: why, translated.
    void failed(const QString &errorText);

protected:
    void tabInserted(int index) override;
    void tabRemoved(int index) override;

private:
    WorkboxModel &m_model;
    QString m_id;
    QColor m_workboxColor;
    int m_openedShells = 0;
};
