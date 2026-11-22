#include "dialogs/workbox/create.h"

#include "color.h"
#include "dialogs/formparts.h"
#include "dialogs/volume/add.h"
#include "volumes.h"
#include "workboxdelegate.h"

#include <KLocalizedString>
#include <KMessageWidget>

#include <QCheckBox>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace
{

constexpr int headerSwatchSize = 44;
constexpr int colorSwatchSize = 22;
constexpr int suggestionSize = 16;
constexpr int suggestionCount = 5;

// The name in id's characters: lowercase, accents dropped, anything else one
// hyphen, up to 32 characters.
QString idFromName(const QString &name)
{
    QString id;
    for (const QChar character : name.normalized(QString::NormalizationForm_D).toLower()) {
        if ((character >= u'a' && character <= u'z') || (character >= u'0' && character <= u'9')) {
            id += character;
        } else if (character.category() != QChar::Mark_NonSpacing && !id.isEmpty() && !id.endsWith(u'-')) {
            id += u'-';
        }
    }
    id.truncate(32);
    while (id.endsWith(u'-')) {
        id.chop(1);
    }
    return id;
}

QPixmap colorPixmap(const QColor &color, int size, qreal pixelRatio)
{
    QPixmap pixmap(QSize(size, size) * pixelRatio);
    pixmap.setDevicePixelRatio(pixelRatio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(0, 0, size, size), size * 0.25, size * 0.25);
    return pixmap;
}

// Where the workbox has it, from where: "<folder> → <target>", "<name> →
// <target>" or "<target>".
QString volumeMapping(const Volume &volume)
{
    const QString target = homeRelativePath(volume.target);
    const QString source = volume.source.isEmpty() ? volume.name : homeRelativePath(volume.source);
    return source.isEmpty() ? target : i18nc("@info a volume's source and its folder in the workbox", "%1 → %2", source, target);
}

QString volumeKind(const Volume &volume)
{
    if (!volume.source.isEmpty()) {
        return volume.isReadOnly ? i18nc("@info a volume's type", "Shared volume · Read-only") : i18nc("@info a volume's type", "Shared volume");
    }
    return volume.name.isEmpty() ? i18nc("@info a volume's type", "Volume") : i18nc("@info a volume's type", "Labeled volume");
}

}

CreateWorkboxDialog::CreateWorkboxDialog(WorkboxModel &model, QWidget *parent)
    : QDialog(parent)
    , m_model(model)
{
    setWindowTitle(i18nc("@title:window", "Create Workbox"));
    auto *layout = new QVBoxLayout(this);

    auto *header = new QHBoxLayout;
    header->setSpacing(14);
    m_swatch = new QLabel;
    header->addWidget(m_swatch);
    auto *titles = new QVBoxLayout;
    titles->setSpacing(2);
    m_titleLabel = new QLabel;
    m_titleLabel->setObjectName(QStringLiteral("titleLabel"));
    m_titleLabel->setTextFormat(Qt::PlainText);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.3);
    titleFont.setWeight(QFont::DemiBold);
    m_titleLabel->setFont(titleFont);
    titles->addWidget(m_titleLabel);
    m_idPreviewLabel = new QLabel;
    m_idPreviewLabel->setForegroundRole(QPalette::PlaceholderText);
    setMonospace(m_idPreviewLabel);
    titles->addWidget(m_idPreviewLabel);
    header->addLayout(titles, 1);
    layout->addLayout(header);

    m_pages = new QStackedWidget;
    m_pages->addWidget(createFormPage());
    m_pages->addWidget(createProgressPage());
    m_pages->addWidget(createDonePage());
    layout->addWidget(m_pages, 1);
    resize(720, 760);
    updateForm();
}

QWidget *CreateWorkboxDialog::createFormPage()
{
    m_formPage = new QWidget;
    m_formPage->setObjectName(QStringLiteral("formPage"));
    auto *layout = new QVBoxLayout(m_formPage);
    layout->setContentsMargins(0, 0, 0, 0);

    m_errorMessage = new KMessageWidget;
    m_errorMessage->setObjectName(QStringLiteral("errorMessage"));
    m_errorMessage->setMessageType(KMessageWidget::Error);
    m_errorMessage->setWordWrap(true);
    m_errorMessage->hide();
    layout->addWidget(m_errorMessage);

    auto *content = new QWidget;
    auto *sections = new QVBoxLayout(content);
    sections->setSpacing(24);
    sections->addWidget(createIdentitySection());
    sections->addWidget(createVolumesSection());
    sections->addWidget(createAccessSection());
    sections->addStretch();
    auto *scrollArea = new QScrollArea;
    scrollArea->setWidget(content);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    layout->addWidget(scrollArea, 1);

    auto *footer = new QHBoxLayout;
    m_footerNote = new QLabel;
    m_footerNote->setObjectName(QStringLiteral("footerNote"));
    m_footerNote->setForegroundRole(QPalette::PlaceholderText);
    footer->addWidget(m_footerNote, 1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    m_createButton = buttons->addButton(i18nc("@action:button", "Create Workbox"), QDialogButtonBox::ActionRole);
    m_createButton->setObjectName(QStringLiteral("createWorkboxButton"));
    m_createButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    m_createButton->setDefault(true);
    connect(m_createButton, &QPushButton::clicked, this, &CreateWorkboxDialog::create);
    connect(buttons, &QDialogButtonBox::rejected, this, &CreateWorkboxDialog::reject);
    footer->addWidget(buttons);
    layout->addLayout(footer);
    return m_formPage;
}

QWidget *CreateWorkboxDialog::createIdentitySection()
{
    auto *section = new QWidget;
    auto *layout = new QFormLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setVerticalSpacing(12);

    m_nameEdit = new QLineEdit;
    m_nameEdit->setObjectName(QStringLiteral("nameEdit"));
    m_nameEdit->setPlaceholderText(i18nc("@info:placeholder a workbox's name", "For example, Client 02"));
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this](const QString &name) {
        if (!m_isIdTyped) {
            m_idEdit->setText(idFromName(name));
        }
        updateForm();
    });
    layout->addRow(i18nc("@label:textbox a workbox's", "Name:"), m_nameEdit);

    auto *idField = new QVBoxLayout;
    idField->setSpacing(4);
    m_idEdit = new QLineEdit;
    m_idEdit->setObjectName(QStringLiteral("idEdit"));
    setMonospace(m_idEdit);
    connect(m_idEdit, &QLineEdit::textEdited, this, [this] {
        m_isIdTyped = true;
    });
    connect(m_idEdit, &QLineEdit::textChanged, this, &CreateWorkboxDialog::updateForm);
    idField->addWidget(m_idEdit);
    m_idHintLabel = new QLabel;
    m_idHintLabel->setObjectName(QStringLiteral("idHintLabel"));
    m_idHintLabel->setWordWrap(true);
    m_idHintLabel->setTextFormat(Qt::PlainText);
    idField->addWidget(m_idHintLabel);
    layout->addRow(i18nc("@label:textbox a workbox's", "Id:"), idField);

    auto *imageLabel = new QLabel(m_model.image());
    imageLabel->setObjectName(QStringLiteral("imageLabel"));
    imageLabel->setTextFormat(Qt::PlainText);
    imageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addRow(i18nc("@label a workbox's", "Image:"), imageLabel);

    // The one chosen, at first what create would pick, and a dialog to change
    // it; under it, smaller, what create would pick after it.
    QList<QColor> colorsInUse;
    for (int row = 0; row < m_model.rowCount(); ++row) {
        colorsInUse.append(m_model.index(row).data(WorkboxModel::ColorRole).value<QColor>());
    }
    const QList<QColor> suggestedColors = WorkboxColor::suggestions(colorsInUse, suggestionCount);
    m_color = suggestedColors.first();
    auto *colorField = new QHBoxLayout;
    m_colorSwatch = new QLabel;
    m_colorSwatch->setObjectName(QStringLiteral("colorSwatch"));
    colorField->setSpacing(10);
    colorField->addWidget(m_colorSwatch);
    auto *changeButton = new QPushButton(QIcon::fromTheme(QStringLiteral("color-picker")), i18nc("@action:button a workbox's color", "Change…"));
    changeButton->setObjectName(QStringLiteral("changeColorButton"));
    connect(changeButton, &QPushButton::clicked, this, &CreateWorkboxDialog::chooseColor);
    colorField->addWidget(changeButton);
    colorField->addStretch();
    layout->addRow(i18nc("@label a workbox's", "Color:"), colorField);

    // Part of the color, under it, in the same column.
    auto *suggestions = new QHBoxLayout;
    suggestions->setSpacing(0);
    QLabel *suggestionsLabel = createExplanation(i18nc("@label a workbox's colors", "Suggestions:"));
    suggestionsLabel->setWordWrap(false);
    suggestions->addWidget(suggestionsLabel);
    suggestions->addSpacing(4);
    for (const QColor &color : suggestedColors) {
        auto *button = new QToolButton;
        button->setAutoRaise(true);
        button->setIcon(colorPixmap(color, suggestionSize, devicePixelRatioF()));
        button->setIconSize(QSize(suggestionSize, suggestionSize));
        button->setToolTip(color.name());
        button->setAccessibleName(color.name());
        connect(button, &QToolButton::clicked, this, [this, color] {
            setColor(color);
        });
        suggestions->addWidget(button);
    }
    suggestions->addStretch();
    layout->addRow(QString(), suggestions);
    return section;
}

QWidget *CreateWorkboxDialog::createVolumesSection()
{
    auto *section = new QWidget;
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(createSectionTitle(i18nc("@title", "Volumes")));
    layout->addWidget(createExplanation(
        i18n("The workbox sees only the folders of your home shared with it. Its other volumes keep their data in folders of its own, without sharing anything.")));

    auto *frame = new QFrame;
    frame->setFrameShape(QFrame::StyledPanel);
    m_volumesList = frame;
    new QVBoxLayout(m_volumesList);
    layout->addWidget(m_volumesList);
    m_noVolumesLabel = createExplanation(i18n("No volumes: it has only its home."));
    m_noVolumesLabel->setObjectName(QStringLiteral("noVolumesLabel"));
    layout->addWidget(m_noVolumesLabel);

    auto *addButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18nc("@action:button", "Add Volume…"));
    addButton->setObjectName(QStringLiteral("addVolumeButton"));
    connect(addButton, &QPushButton::clicked, this, [this] {
        openVolumeDialog();
    });
    layout->addWidget(addButton, 0, Qt::AlignLeft);
    m_volumesList->hide();
    return section;
}

QWidget *CreateWorkboxDialog::createAccessSection()
{
    auto *section = new QWidget;
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(createSectionTitle(i18nc("@title", "System Access")));
    layout->addWidget(createExplanation(i18n("These can't be changed later without creating the workbox again.")));

    m_waylandCheck = new QCheckBox(i18nc("@option:check", "Open graphical apps"));
    m_waylandCheck->setObjectName(QStringLiteral("waylandCheck"));
    layout->addWidget(createOption(m_waylandCheck, i18n("Shares your Wayland session, so its apps can show windows.")));
    m_sudoCheck = new QCheckBox(i18nc("@option:check", "Allow sudo in the workbox"));
    m_sudoCheck->setObjectName(QStringLiteral("sudoCheck"));
    layout->addWidget(createOption(m_sudoCheck, i18n("To install packages in it. It gives no administrator rights on your system.")));

    auto *network = new QHBoxLayout;
    auto *networkIcon = new QLabel;
    networkIcon->setPixmap(QIcon::fromTheme(QStringLiteral("network-wired")).pixmap(16));
    network->addWidget(networkIcon, 0, Qt::AlignTop);
    network->addWidget(createExplanation(i18n("Its own network, always: it reaches the internet, but nothing reaches it.")), 1);
    layout->addLayout(network);
    return section;
}

QWidget *CreateWorkboxDialog::createProgressPage()
{
    m_progressPage = new QWidget;
    m_progressPage->setObjectName(QStringLiteral("progressPage"));
    auto *layout = new QVBoxLayout(m_progressPage);
    layout->addStretch();
    m_progressTitle = new QLabel;
    m_progressTitle->setTextFormat(Qt::PlainText);
    layout->addWidget(m_progressTitle);
    auto *progressBar = new QProgressBar;
    // How long it takes isn't known: downloading the image is most of it.
    progressBar->setRange(0, 0);
    layout->addWidget(progressBar);
    // The engine's last line, whole: an image's download.
    m_outputLabel = new QLabel;
    m_outputLabel->setObjectName(QStringLiteral("outputLabel"));
    m_outputLabel->setTextFormat(Qt::PlainText);
    m_outputLabel->setForegroundRole(QPalette::PlaceholderText);
    setMonospace(m_outputLabel);
    layout->addWidget(m_outputLabel);
    layout->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::rejected, this, [this] {
        if (m_job) {
            m_job->kill();
        }
        m_pages->setCurrentWidget(m_formPage);
    });
    layout->addWidget(buttons);
    return m_progressPage;
}

QWidget *CreateWorkboxDialog::createDonePage()
{
    m_donePage = new QWidget;
    m_donePage->setObjectName(QStringLiteral("donePage"));
    auto *layout = new QVBoxLayout(m_donePage);
    layout->addStretch();
    m_doneIcon = new QLabel;
    layout->addWidget(m_doneIcon, 0, Qt::AlignHCenter);
    m_doneTitle = new QLabel;
    m_doneTitle->setObjectName(QStringLiteral("doneTitle"));
    m_doneTitle->setTextFormat(Qt::PlainText);
    m_doneTitle->setAlignment(Qt::AlignHCenter);
    QFont titleFont = m_doneTitle->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.2);
    titleFont.setWeight(QFont::DemiBold);
    m_doneTitle->setFont(titleFont);
    layout->addWidget(m_doneTitle);
    m_doneText = createExplanation(QString());
    m_doneText->setAlignment(Qt::AlignHCenter);
    layout->addWidget(m_doneText);
    layout->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton *openButton = buttons->addButton(i18nc("@action:button", "Open a Terminal"), QDialogButtonBox::AcceptRole);
    openButton->setObjectName(QStringLiteral("openTerminalButton"));
    openButton->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
    openButton->setDefault(true);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        Q_EMIT terminalRequested(m_createdId);
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &CreateWorkboxDialog::reject);
    layout->addWidget(buttons);
    return m_donePage;
}

void CreateWorkboxDialog::openVolumeDialog(std::optional<qsizetype> index)
{
    auto *dialog = new AddVolumeDialog(index ? std::optional<Volume>(m_volumes[*index]) : std::nullopt, this);
    dialog->setObjectName(QStringLiteral("volumeDialog"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, [this, dialog, index] {
        if (index) {
            m_volumes[*index] = dialog->volume();
        } else {
            m_volumes.append(dialog->volume());
        }
        showVolumes();
    });
    dialog->open();
}

void CreateWorkboxDialog::showVolumes()
{
    QLayout *rows = m_volumesList->layout();
    while (QLayoutItem *row = rows->takeAt(0)) {
        delete row->widget();
        delete row;
    }
    for (qsizetype index = 0; index < m_volumes.size(); ++index) {
        const Volume &volume = m_volumes[index];
        auto *row = new QWidget;
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        auto *kindIcon = new QLabel;
        kindIcon->setPixmap(QIcon::fromTheme(volume.source.isEmpty() ? QStringLiteral("drive-harddisk") : QStringLiteral("folder")).pixmap(16));
        layout->addWidget(kindIcon);
        auto *texts = new QVBoxLayout;
        texts->setSpacing(0);
        auto *mappingLabel = new QLabel(volumeMapping(volume));
        mappingLabel->setObjectName(QStringLiteral("volumeMappingLabel"));
        mappingLabel->setTextFormat(Qt::PlainText);
        setMonospace(mappingLabel);
        texts->addWidget(mappingLabel);
        texts->addWidget(createExplanation(volumeKind(volume)));
        layout->addLayout(texts, 1);

        auto *editButton = new QToolButton;
        editButton->setObjectName(QStringLiteral("editVolumeButton"));
        editButton->setAutoRaise(true);
        editButton->setIcon(QIcon::fromTheme(QStringLiteral("document-edit")));
        editButton->setToolTip(i18nc("@info:tooltip a volume", "Edit…"));
        editButton->setAccessibleName(editButton->toolTip());
        connect(editButton, &QToolButton::clicked, this, [this, index] {
            openVolumeDialog(index);
        });
        layout->addWidget(editButton);
        auto *removeButton = new QToolButton;
        removeButton->setObjectName(QStringLiteral("removeVolumeButton"));
        removeButton->setAutoRaise(true);
        removeButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
        removeButton->setToolTip(i18nc("@info:tooltip a volume", "Remove"));
        removeButton->setAccessibleName(removeButton->toolTip());
        // After the click: the button goes with its row.
        connect(
            removeButton,
            &QToolButton::clicked,
            this,
            [this, index] {
                m_volumes.removeAt(index);
                showVolumes();
            },
            Qt::QueuedConnection);
        layout->addWidget(removeButton);
        rows->addWidget(row);
    }
    m_volumesList->setVisible(!m_volumes.isEmpty());
    m_noVolumesLabel->setVisible(m_volumes.isEmpty());
    updateForm();
}

QString CreateWorkboxDialog::idProblem() const
{
    const QString id = m_idEdit->text();
    if (id.isEmpty()) {
        return i18n("Give it an id.");
    }
    if (!isValidId(id)) {
        return i18n("Only lowercase letters (a-z), digits and hyphens, up to 32, not starting with a hyphen.");
    }
    for (int row = 0; row < m_model.rowCount(); ++row) {
        if (m_model.index(row).data(WorkboxModel::IdRole).toString() == id) {
            return i18n("There's already a workbox with this id.");
        }
    }
    return {};
}

void CreateWorkboxDialog::setColor(const QColor &color)
{
    m_color = color;
    updateForm();
}

void CreateWorkboxDialog::chooseColor()
{
    auto *dialog = new QColorDialog(m_color, this);
    dialog->setObjectName(QStringLiteral("colorDialog"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(i18nc("@title:window", "Choose a Color"));
    connect(dialog, &QColorDialog::colorSelected, this, &CreateWorkboxDialog::setColor);
    dialog->open();
}

void CreateWorkboxDialog::updateForm()
{
    const QString name = m_nameEdit->text().trimmed();
    const QString id = m_idEdit->text();
    m_swatch->setPixmap(swatchPixmap(m_color, headerSwatchSize, devicePixelRatioF()));
    m_colorSwatch->setPixmap(colorPixmap(m_color, colorSwatchSize, devicePixelRatioF()));
    m_colorSwatch->setToolTip(m_color.name());
    m_colorSwatch->setAccessibleName(m_color.name());
    m_titleLabel->setText(name.isEmpty() ? i18nc("@title", "New Workbox") : name);
    m_idPreviewLabel->setText(id);

    const QString idProblem = this->idProblem();
    // No error for an empty id until something's typed.
    const bool showsIdProblem = !idProblem.isEmpty() && (!id.isEmpty() || !name.isEmpty() || m_isIdTyped);
    m_idHintLabel->setText(showsIdProblem ? idProblem
                                          : i18n("For commands, and its home, the volume %1. It can't be changed later.",
                                                 volumeName(id.isEmpty() ? QStringLiteral("<id>") : id, homeVolume)));
    setNegative(m_idHintLabel, showsIdProblem);

    const auto sharedFolderCount = std::ranges::count_if(m_volumes, [](const Volume &volume) {
        return !volume.source.isEmpty();
    });

    QString footerNote;
    if (name.isEmpty()) {
        footerNote = i18n("It needs a name.");
    } else if (!idProblem.isEmpty()) {
        footerNote = i18n("Check its id.");
    } else if (sharedFolderCount == 0) {
        footerNote = i18n("It shares no folder with your system.");
    } else {
        footerNote = i18np("It shares 1 folder with your system.", "It shares %1 folders with your system.", static_cast<int>(sharedFolderCount));
    }
    m_footerNote->setText(footerNote);
    m_createButton->setEnabled(!name.isEmpty() && idProblem.isEmpty());
}

void CreateWorkboxDialog::create()
{
    Commands::CreateOptions options;
    options.name = m_nameEdit->text().trimmed();
    options.color = m_color.name();
    options.hasSudo = m_sudoCheck->isChecked();
    options.hasWayland = m_waylandCheck->isChecked();
    for (const Volume &volume : std::as_const(m_volumes)) {
        options.volumes.append(volumeArgument(volume));
    }

    m_errorMessage->hide();
    m_progressTitle->setText(i18nc("@info", "Creating “%1”…", *options.name));
    m_outputLabel->clear();
    m_pages->setCurrentWidget(m_progressPage);
    m_job = m_model.create(m_idEdit->text(), options);
    connect(m_job, &EngineJob::output, m_outputLabel, &QLabel::setText);
    connect(m_job, &KJob::result, this, [this, job = m_job.data()] {
        if (job->error()) {
            m_errorMessage->setText(job->errorText());
            m_errorMessage->show();
            m_pages->setCurrentWidget(m_formPage);
            return;
        }
        showCreated(job->workbox());
    });
}

void CreateWorkboxDialog::showCreated(const Workbox &workbox)
{
    m_createdId = workbox.id;
    m_doneIcon->setPixmap(swatchPixmap(workbox.color, 64, devicePixelRatioF()));
    m_doneTitle->setText(i18nc("@title", "“%1” is ready", workbox.name));
    m_doneText->setText(i18n("Its home is the volume %1.", volumeName(workbox.id, homeVolume)));
    m_pages->setCurrentWidget(m_donePage);
    Q_EMIT created(workbox.id);
}

void CreateWorkboxDialog::reject()
{
    if (m_job) {
        m_job->kill();
    }
    QDialog::reject();
}
