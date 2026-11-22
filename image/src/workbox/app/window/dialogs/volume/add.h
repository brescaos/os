// Adding a volume to a new workbox, or editing one (README.md), as -v takes
// it (volumes.h): its type (a folder of the home, shared; a named volume of
// its own, or one without a name), where it is in the home and in the
// workbox, and, for a shared folder, read-only. It's checked as create checks
// it: Add is enabled only once it can be.
#pragma once

#include "workbox.h"

#include <QDialog>

#include <optional>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QStackedWidget;

class AddVolumeDialog : public QDialog
{
    Q_OBJECT

public:
    // With a volume, to edit it.
    explicit AddVolumeDialog(const std::optional<Volume> &volume = std::nullopt, QWidget *parent = nullptr);

    // As parseVolume() gives it, with its real paths: once accepted.
    Volume volume() const;

private:
    enum class Kind {
        Shared,
        Named,
        Unnamed,
    };

    QWidget *createKindSection();
    QWidget *createWhereSection();

    Kind kind() const;
    // Its -v argument, or nothing while a field is empty; throws a
    // WorkboxError for a volume's name that isn't one.
    std::optional<QString> argument() const;
    // The folder a dialog picks, as the shared one.
    void chooseFolder();
    // When the workbox's folder is empty, the shared one's place in the home.
    void fillTarget();
    // Everything that follows from the type and what's typed: the source,
    // read-only, the hint or why it can't be, and whether it can be added.
    void updateForm();

    QRadioButton *m_sharedButton;
    QRadioButton *m_namedButton;
    QRadioButton *m_unnamedButton;
    QLabel *m_sourceLabel;
    QStackedWidget *m_sources;
    QWidget *m_folderSource;
    QLineEdit *m_folderEdit;
    QLineEdit *m_nameEdit;
    QLabel *m_unnamedSource;
    QLineEdit *m_targetEdit;
    QLabel *m_hintLabel;
    QWidget *m_readOnlyOption;
    QCheckBox *m_readOnlyCheck;
    QPushButton *m_okButton;
    Volume m_volume;
};
