// Creating a workbox (README.md), as `workbox create`: a form with its name
// and id, its color, its volumes, sudo and Wayland; then its progress, with
// the engine's output; then a terminal in it, if the user wants one. Nothing
// blocks: the model's job creates it.
#pragma once

#include "workboxmodel.h"

#include <QDialog>
#include <QPointer>

#include <optional>

class KMessageWidget;
class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;

class CreateWorkboxDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateWorkboxDialog(WorkboxModel &model, QWidget *parent = nullptr);

    // Stops creating, if it's under way.
    void reject() override;

Q_SIGNALS:
    // Its row is in the model.
    void created(const QString &id);
    // Once created, the user asked for a terminal in it.
    void terminalRequested(const QString &id);

private:
    QWidget *createFormPage();
    QWidget *createIdentitySection();
    QWidget *createVolumesSection();
    QWidget *createAccessSection();
    QWidget *createProgressPage();
    QWidget *createDonePage();

    // AddVolumeDialog, to add a volume or, with its index, to edit it.
    void openVolumeDialog(std::optional<qsizetype> index = std::nullopt);
    // A row per volume, each with its buttons to edit and remove it.
    void showVolumes();
    // Why the id can't be, or nothing.
    QString idProblem() const;
    void setColor(const QColor &color);
    // The color a dialog picks, starting from the one chosen.
    void chooseColor();

    // Everything that follows from what's typed: the header, the color, the
    // id's hint, the footer and whether it can be created.
    void updateForm();
    void create();
    void showCreated(const Workbox &workbox);

    WorkboxModel &m_model;
    QPointer<Commands::CreateJob> m_job;
    // The id follows the name until the user types one.
    bool m_isIdTyped = false;

    QLabel *m_swatch;
    QLabel *m_titleLabel;
    QLabel *m_idPreviewLabel;
    QStackedWidget *m_pages;

    QWidget *m_formPage;
    KMessageWidget *m_errorMessage;
    QLineEdit *m_nameEdit;
    QLineEdit *m_idEdit;
    QLabel *m_idHintLabel;
    // At first the best of the suggestions.
    QColor m_color;
    QLabel *m_colorSwatch;
    // Checked by AddVolumeDialog, as parseVolume() gives them.
    QList<Volume> m_volumes;
    // Its rows; hidden while there's none.
    QWidget *m_volumesList;
    QLabel *m_noVolumesLabel;
    QCheckBox *m_waylandCheck;
    QCheckBox *m_sudoCheck;
    QLabel *m_footerNote;
    QPushButton *m_createButton;

    QWidget *m_progressPage;
    QLabel *m_progressTitle;
    QLabel *m_outputLabel;

    QWidget *m_donePage;
    QLabel *m_doneIcon;
    QLabel *m_doneTitle;
    QLabel *m_doneText;
    QString m_createdId;
};
