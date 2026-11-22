#include "dialogs/volume/add.h"

#include "dialogs/formparts.h"
#include "volumes.h"

#include <KLocalizedString>

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{

// ~ for the home, and relative to it: a folder the user types.
QString folderPath(const QString &typed)
{
    const QString home = QDir::homePath();
    const QString expanded = typed == QLatin1String("~") || typed.startsWith(QLatin1String("~/")) ? home + typed.mid(1) : typed;
    return QDir::cleanPath(QDir(home).absoluteFilePath(expanded));
}

}

AddVolumeDialog::AddVolumeDialog(const std::optional<Volume> &volume, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(volume ? i18nc("@title:window", "Edit Volume") : i18nc("@title:window", "Add Volume"));
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    layout->addWidget(createKindSection());
    layout->addWidget(createWhereSection());

    m_readOnlyCheck = new QCheckBox(i18nc("@option:check a shared volume", "Read-only"));
    m_readOnlyCheck->setObjectName(QStringLiteral("readOnlyCheck"));
    connect(m_readOnlyCheck, &QCheckBox::toggled, this, &AddVolumeDialog::updateForm);
    m_readOnlyOption = createOption(m_readOnlyCheck, i18n("The workbox can read your files, but not change them."));
    layout->addWidget(m_readOnlyOption);
    layout->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    m_okButton = buttons->addButton(volume ? i18nc("@action:button a volume", "Save") : i18nc("@action:button a volume", "Add"), QDialogButtonBox::AcceptRole);
    m_okButton->setObjectName(QStringLiteral("okButton"));
    m_okButton->setIcon(QIcon::fromTheme(volume ? QStringLiteral("document-save") : QStringLiteral("list-add")));
    m_okButton->setDefault(true);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    if (volume) {
        m_targetEdit->setText(homeRelativePath(volume->target));
        if (!volume->source.isEmpty()) {
            m_folderEdit->setText(homeRelativePath(volume->source));
            m_readOnlyCheck->setChecked(volume->isReadOnly);
        } else if (!volume->name.isEmpty()) {
            m_nameEdit->setText(volume->name);
            m_namedButton->setChecked(true);
        } else {
            m_unnamedButton->setChecked(true);
        }
    }
    resize(560, sizeHint().height());
    updateForm();
}

QWidget *AddVolumeDialog::createKindSection()
{
    auto *section = new QWidget;
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(createSectionTitle(i18nc("@title a volume's", "Type")));

    auto *kinds = new QButtonGroup(this);
    const auto addKind = [this, layout, kinds](const QString &text, const QString &explanation, const QString &objectName) {
        auto *button = new QRadioButton(text);
        button->setObjectName(objectName);
        kinds->addButton(button);
        layout->addWidget(createOption(button, explanation));
        return button;
    };
    m_sharedButton = addKind(i18nc("@option:radio a volume's type", "Shared Volume"),
                             i18n("A folder of your home. The workbox sees your files there, and can change them unless it's read-only."),
                             QStringLiteral("sharedButton"));
    m_namedButton = addKind(i18nc("@option:radio a volume's type", "Labeled Volume"),
                            i18n("A folder of its own, with a name. Its data stays with the workbox, outside your home."),
                            QStringLiteral("namedButton"));
    m_unnamedButton = addKind(i18nc("@option:radio a volume's type", "Volume"),
                              i18n("An empty folder of its own, without a name."),
                              QStringLiteral("unnamedButton"));
    m_sharedButton->setChecked(true);
    connect(kinds, &QButtonGroup::buttonToggled, this, &AddVolumeDialog::updateForm);
    return section;
}

// The mapping on one line: the source, an arrow, the folder in the workbox.
QWidget *AddVolumeDialog::createWhereSection()
{
    auto *section = new QWidget;
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(createSectionTitle(i18nc("@title a volume's", "Where")));

    auto *mapping = new QGridLayout;
    mapping->setHorizontalSpacing(8);
    m_sourceLabel = createExplanation(QString());
    mapping->addWidget(m_sourceLabel, 0, 0);
    mapping->addWidget(createExplanation(i18nc("@label:textbox a volume's folder", "In the workbox")), 0, 2);

    m_sources = new QStackedWidget;
    m_folderSource = new QWidget;
    auto *folderLayout = new QHBoxLayout(m_folderSource);
    folderLayout->setContentsMargins(0, 0, 0, 0);
    folderLayout->setSpacing(4);
    m_folderEdit = new QLineEdit;
    m_folderEdit->setObjectName(QStringLiteral("folderEdit"));
    m_folderEdit->setPlaceholderText(i18nc("@info:placeholder a folder of the home", "~/folder"));
    m_folderEdit->setAccessibleName(i18nc("@label:textbox", "Folder to share"));
    setMonospace(m_folderEdit);
    connect(m_folderEdit, &QLineEdit::textChanged, this, &AddVolumeDialog::updateForm);
    connect(m_folderEdit, &QLineEdit::editingFinished, this, &AddVolumeDialog::fillTarget);
    folderLayout->addWidget(m_folderEdit);
    auto *chooseButton = new QToolButton;
    chooseButton->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
    chooseButton->setToolTip(i18nc("@info:tooltip", "Choose a folder…"));
    chooseButton->setAccessibleName(chooseButton->toolTip());
    connect(chooseButton, &QToolButton::clicked, this, &AddVolumeDialog::chooseFolder);
    folderLayout->addWidget(chooseButton);
    m_sources->addWidget(m_folderSource);

    m_nameEdit = new QLineEdit;
    m_nameEdit->setObjectName(QStringLiteral("nameEdit"));
    m_nameEdit->setPlaceholderText(i18nc("@info:placeholder a volume's name", "name"));
    m_nameEdit->setAccessibleName(i18nc("@label:textbox", "Volume's name"));
    setMonospace(m_nameEdit);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &AddVolumeDialog::updateForm);
    m_sources->addWidget(m_nameEdit);

    m_unnamedSource = createExplanation(i18nc("@info a volume's source", "Empty, of its own"));
    m_sources->addWidget(m_unnamedSource);
    mapping->addWidget(m_sources, 1, 0);

    auto *arrow = new QLabel;
    arrow->setPixmap(QIcon::fromTheme(QStringLiteral("go-next")).pixmap(16));
    mapping->addWidget(arrow, 1, 1);

    m_targetEdit = new QLineEdit;
    m_targetEdit->setObjectName(QStringLiteral("targetEdit"));
    m_targetEdit->setPlaceholderText(i18nc("@info:placeholder a folder of the workbox's home", "~/folder"));
    m_targetEdit->setAccessibleName(i18nc("@label:textbox", "Folder in the workbox"));
    setMonospace(m_targetEdit);
    connect(m_targetEdit, &QLineEdit::textChanged, this, &AddVolumeDialog::updateForm);
    mapping->addWidget(m_targetEdit, 1, 2);
    mapping->setColumnStretch(0, 1);
    mapping->setColumnStretch(2, 1);
    layout->addLayout(mapping);

    m_hintLabel = createExplanation(QString());
    m_hintLabel->setObjectName(QStringLiteral("hintLabel"));
    m_hintLabel->setTextFormat(Qt::PlainText);
    layout->addWidget(m_hintLabel);
    return section;
}

Volume AddVolumeDialog::volume() const
{
    return m_volume;
}

AddVolumeDialog::Kind AddVolumeDialog::kind() const
{
    if (m_namedButton->isChecked()) {
        return Kind::Named;
    }
    return m_unnamedButton->isChecked() ? Kind::Unnamed : Kind::Shared;
}

std::optional<QString> AddVolumeDialog::argument() const
{
    const QString target = m_targetEdit->text().trimmed();
    if (target.isEmpty()) {
        return std::nullopt;
    }
    switch (kind()) {
    case Kind::Shared: {
        const QString folder = m_folderEdit->text().trimmed();
        if (folder.isEmpty()) {
            return std::nullopt;
        }
        return folderPath(folder) + QLatin1Char(':') + target + (m_readOnlyCheck->isChecked() ? QStringLiteral(":ro") : QString());
    }
    case Kind::Named: {
        const QString name = m_nameEdit->text().trimmed();
        if (name.isEmpty()) {
            return std::nullopt;
        }
        // A name with a "/" would be taken for a folder.
        if (!isValidVolume(name)) {
            throw WorkboxError{
                i18n("invalid volume name %1: up to 32 lowercase letters (a-z), digits and hyphens, not starting with a hyphen, and not home", name)};
        }
        return name + QLatin1Char(':') + target;
    }
    case Kind::Unnamed:
        return target;
    }
    return std::nullopt;
}

void AddVolumeDialog::chooseFolder()
{
    auto *dialog = new QFileDialog(this, i18nc("@title:window", "Choose a Folder to Share"), QDir::homePath());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOption(QFileDialog::ShowDirsOnly);
    connect(dialog, &QFileDialog::fileSelected, this, [this](const QString &folder) {
        m_folderEdit->setText(homeRelativePath(QDir(folder).canonicalPath()));
        fillTarget();
    });
    dialog->open();
}

void AddVolumeDialog::fillTarget()
{
    const QString folder = m_folderEdit->text().trimmed();
    if (!m_targetEdit->text().trimmed().isEmpty() || folder.isEmpty()) {
        return;
    }
    const QString place = homeRelativePath(folderPath(folder));
    if (place.startsWith(QLatin1String("~/"))) {
        m_targetEdit->setText(place);
    }
}

void AddVolumeDialog::updateForm()
{
    const Kind kind = this->kind();
    switch (kind) {
    case Kind::Shared:
        m_sourceLabel->setText(i18nc("@label:textbox a volume's source", "In your home"));
        m_sources->setCurrentWidget(m_folderSource);
        break;
    case Kind::Named:
        m_sourceLabel->setText(i18nc("@label:textbox a volume's source", "Name"));
        m_sources->setCurrentWidget(m_nameEdit);
        break;
    case Kind::Unnamed:
        m_sourceLabel->setText(i18nc("@label a volume's source", "Source"));
        m_sources->setCurrentWidget(m_unnamedSource);
        break;
    }
    m_readOnlyOption->setVisible(kind == Kind::Shared);

    QString problem;
    bool isComplete = false;
    try {
        const std::optional<QString> argument = this->argument();
        if (argument) {
            m_volume = parseVolume(*argument, QDir::homePath());
            isComplete = true;
        }
    } catch (const WorkboxError &error) {
        problem = error.message;
    }
    if (problem.isEmpty()) {
        switch (kind) {
        case Kind::Shared:
            m_hintLabel->setText(i18n("Your folder appears in the workbox at the folder on the right."));
            break;
        case Kind::Named:
            m_hintLabel->setText(i18n("Kept as a volume of the workbox's own, removed with it."));
            break;
        case Kind::Unnamed:
            m_hintLabel->setText(i18n("An empty folder at the folder on the right, removed with the workbox."));
            break;
        }
    } else {
        m_hintLabel->setText(problem);
    }
    setNegative(m_hintLabel, !problem.isEmpty());
    m_okButton->setEnabled(isComplete && problem.isEmpty());
}
