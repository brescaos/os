#include "mainwindow.h"

#include "dialogs/workbox/create.h"
#include "terminals.h"
#include "volumes.h"
#include "workboxdelegate.h"

#include <KLocalizedString>
#include <KMessageWidget>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>

namespace
{

constexpr int headerSwatchSize = 44;

// The rows whose name or id has the search's text.
class WorkboxFilter : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        return index.data(WorkboxModel::NameRole).toString().contains(filterRegularExpression())
            || index.data(WorkboxModel::IdRole).toString().contains(filterRegularExpression());
    }
};

QFont scaledFont(const QFont &font, double scale, QFont::Weight weight = QFont::Normal)
{
    QFont scaled = font;
    scaled.setPointSizeF(font.pointSizeF() * scale);
    scaled.setWeight(weight);
    return scaled;
}

QPushButton *createButton(const QString &iconName, const QString &text, const QString &objectName)
{
    auto *button = new QPushButton(QIcon::fromTheme(iconName), text);
    button->setObjectName(objectName);
    return button;
}

// A big icon, title, text and button, centered: what's there when there's
// nothing to show.
QWidget *createPlaceholder(const QString &iconName, QLabel *title, const QString &text, QPushButton *button)
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->addStretch();
    auto *icon = new QLabel;
    icon->setPixmap(QIcon::fromTheme(iconName).pixmap(64));
    layout->addWidget(icon, 0, Qt::AlignHCenter);
    title->setFont(scaledFont(title->font(), 1.3, QFont::DemiBold));
    title->setAlignment(Qt::AlignHCenter);
    title->setWordWrap(true);
    layout->addWidget(title);
    // Without an alignment in the layout: a label that wraps only gets the
    // height of all its lines across the layout's whole width.
    if (!text.isEmpty()) {
        auto *textLabel = new QLabel(text);
        textLabel->setWordWrap(true);
        textLabel->setAlignment(Qt::AlignHCenter);
        textLabel->setForegroundRole(QPalette::PlaceholderText);
        layout->addWidget(textLabel);
    }
    layout->addSpacing(8);
    layout->addWidget(button, 0, Qt::AlignHCenter);
    layout->addStretch();
    return page;
}

// Not wrapped: an image's reference or a volume's name would break at a "/"
// or "-", and the form gives a wrapped label only its first line.
QLabel *createValueLabel()
{
    auto *label = new QLabel;
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setTextFormat(Qt::PlainText);
    return label;
}

QString yesOrNo(bool value)
{
    return value ? i18nc("@info an option of a workbox", "Yes") : i18nc("@info an option of a workbox", "No");
}

}

MainWindow::MainWindow(WorkboxModel &model, QWidget *parent)
    : QMainWindow(parent)
    , m_model(model)
    , m_filter(new WorkboxFilter(this))
{
    m_filter->setSourceModel(&m_model);
    m_filter->setFilterCaseSensitivity(Qt::CaseInsensitive);

    auto *splitter = new QSplitter;
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(createSidebar());

    m_content = new QStackedWidget;
    m_emptyPage = createPlaceholder(QStringLiteral("com.brescaos.workbox"),
                                    new QLabel(i18nc("@title", "No workboxes yet")),
                                    i18n("A workbox is a separate place to work: it doesn't see your home folder or the rest of the system, only "
                                         "what you share with it."),
                                    createCreateButton(i18nc("@action:button", "Create Workbox"), QStringLiteral("emptyCreateButton")));
    m_emptyPage->setObjectName(QStringLiteral("emptyPage"));
    m_content->addWidget(m_emptyPage);
    m_problemLabel = new QLabel;
    m_problemLabel->setObjectName(QStringLiteral("problemLabel"));
    m_problemLabel->setAlignment(Qt::AlignCenter);
    m_problemLabel->setWordWrap(true);
    m_content->addWidget(m_problemLabel);
    m_content->addWidget(createWorkboxPage());
    splitter->addWidget(m_content);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({280, 820});
    setCentralWidget(splitter);
    resize(1100, 720);

    const auto updateCount = [this] {
        m_countLabel->setText(i18nc("@info running workboxes of all", "%1 of %2 running", m_model.runningCount(), m_model.rowCount()));
    };
    connect(&m_model, &WorkboxModel::runningCountChanged, this, updateCount);
    connect(&m_model, &QAbstractItemModel::rowsInserted, this, updateCount);
    connect(&m_model, &QAbstractItemModel::rowsRemoved, this, updateCount);
    // A removed workbox's terminals go with it.
    connect(&m_model, &QAbstractItemModel::rowsAboutToBeRemoved, this, [this](const QModelIndex &, int first, int last) {
        for (int row = first; row <= last; ++row) {
            const QString id = m_model.index(row).data(WorkboxModel::IdRole).toString();
            delete m_terminals.take(id);
            m_shouldOpenShell.remove(id);
        }
    });
    connect(&m_model, &QAbstractItemModel::rowsInserted, this, &MainWindow::keepASelection);
    connect(&m_model, &QAbstractItemModel::rowsRemoved, this, &MainWindow::keepASelection);
    connect(&m_model, &QAbstractItemModel::dataChanged, this, &MainWindow::showSelected);
    connect(&m_model, &WorkboxModel::problemChanged, this, &MainWindow::showSelected);
    connect(&m_model, &WorkboxModel::problemChanged, this, &MainWindow::enableCreating);
    connect(m_list->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current) {
        if (current.isValid()) {
            m_selectedId = current.data(WorkboxModel::IdRole).toString();
            m_shellError.clear();
            showSelected();
        }
    });
    updateCount();
    showSelected();
    enableCreating();
}

QPushButton *MainWindow::createCreateButton(const QString &text, const QString &objectName)
{
    QPushButton *button = createButton(QStringLiteral("list-add"), text, objectName);
    connect(button, &QPushButton::clicked, this, &MainWindow::openCreateDialog);
    m_createButtons.append(button);
    return button;
}

QWidget *MainWindow::createSidebar()
{
    auto *sidebar = new QWidget;
    auto *layout = new QVBoxLayout(sidebar);

    auto *header = new QHBoxLayout;
    auto *titles = new QVBoxLayout;
    titles->setSpacing(0);
    auto *title = new QLabel(i18nc("@title", "Workboxes"));
    title->setFont(scaledFont(title->font(), 1.15, QFont::DemiBold));
    titles->addWidget(title);
    m_countLabel = new QLabel;
    m_countLabel->setObjectName(QStringLiteral("countLabel"));
    m_countLabel->setForegroundRole(QPalette::PlaceholderText);
    titles->addWidget(m_countLabel);
    header->addLayout(titles);
    header->addStretch();
    header->addWidget(createCreateButton(i18nc("@action:button", "Create"), QStringLiteral("createButton")));
    layout->addLayout(header);

    auto *search = new QLineEdit;
    search->setObjectName(QStringLiteral("searchEdit"));
    search->setPlaceholderText(i18nc("@info:placeholder", "Search…"));
    search->setClearButtonEnabled(true);
    connect(search, &QLineEdit::textChanged, m_filter, &QSortFilterProxyModel::setFilterFixedString);
    layout->addWidget(search);

    m_list = new QListView;
    m_list->setObjectName(QStringLiteral("workboxList"));
    m_list->setModel(m_filter);
    m_list->setItemDelegate(new WorkboxDelegate(m_list));
    m_list->setUniformItemSizes(true);
    m_list->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_list);
    return sidebar;
}

QWidget *MainWindow::createWorkboxPage()
{
    m_workboxPage = new QWidget;
    m_workboxPage->setObjectName(QStringLiteral("workboxPage"));
    auto *layout = new QVBoxLayout(m_workboxPage);

    auto *header = new QHBoxLayout;
    header->setSpacing(16);
    m_swatch = new QLabel;
    header->addWidget(m_swatch);
    auto *titles = new QVBoxLayout;
    titles->setSpacing(2);
    m_nameLabel = new QLabel;
    m_nameLabel->setObjectName(QStringLiteral("nameLabel"));
    m_nameLabel->setFont(scaledFont(m_nameLabel->font(), 1.4, QFont::DemiBold));
    m_nameLabel->setTextFormat(Qt::PlainText);
    titles->addWidget(m_nameLabel);
    m_detailLabel = new QLabel;
    m_detailLabel->setObjectName(QStringLiteral("detailLabel"));
    m_detailLabel->setForegroundRole(QPalette::PlaceholderText);
    titles->addWidget(m_detailLabel);
    header->addLayout(titles, 1);

    m_startButton = createButton(QStringLiteral("media-playback-start"), i18nc("@action:button a workbox", "Start"), QStringLiteral("startButton"));
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startSelected);
    header->addWidget(m_startButton);
    m_stopButton = createButton(QStringLiteral("media-playback-stop"), i18nc("@action:button a workbox", "Stop"), QStringLiteral("stopButton"));
    connect(m_stopButton, &QPushButton::clicked, this, [this] {
        m_model.stop(m_selectedId);
    });
    header->addWidget(m_stopButton);
    m_openInKonsoleButton =
        createButton(QStringLiteral("utilities-terminal"), i18nc("@action:button", "Open in Konsole"), QStringLiteral("openInKonsoleButton"));
    m_openInKonsoleButton->setToolTip(i18nc("@info:tooltip", "Open a shell in the workbox in a Konsole window of its own"));
    connect(m_openInKonsoleButton, &QPushButton::clicked, this, &MainWindow::openSelectedInKonsole);
    header->addWidget(m_openInKonsoleButton);
    m_removeButton = createButton(QStringLiteral("edit-delete"), i18nc("@action:button a workbox", "Remove"), QStringLiteral("removeButton"));
    connect(m_removeButton, &QPushButton::clicked, this, [this] {
        confirmRemoval(selectedIndex().data(WorkboxModel::WorkboxRole).value<Workbox>());
    });
    header->addWidget(m_removeButton);
    layout->addLayout(header);

    m_errorMessage = new KMessageWidget;
    m_errorMessage->setObjectName(QStringLiteral("errorMessage"));
    m_errorMessage->setMessageType(KMessageWidget::Error);
    m_errorMessage->setWordWrap(true);
    m_errorMessage->setCloseButtonVisible(false);
    m_errorMessage->hide();
    layout->addWidget(m_errorMessage);

    m_tabs = new QTabWidget;
    m_tabs->setDocumentMode(true);
    m_tabs->addTab(createTerminalTab(), QIcon::fromTheme(QStringLiteral("utilities-terminal")), i18nc("@title:tab", "Terminal"));
    m_tabs->addTab(createInformationTab(), QIcon::fromTheme(QStringLiteral("documentinfo")), i18nc("@title:tab", "Information"));
    layout->addWidget(m_tabs, 1);
    return m_workboxPage;
}

QWidget *MainWindow::createTerminalTab()
{
    m_terminalStack = new QStackedWidget;

    m_stoppedTitle = new QLabel;
    m_stoppedTitle->setTextFormat(Qt::PlainText);
    QPushButton *startButton = createButton(QStringLiteral("media-playback-start"),
                                            i18nc("@action:button", "Start and Open a Terminal"),
                                            QStringLiteral("startAndOpenButton"));
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startSelected);
    m_stoppedPage = createPlaceholder(QStringLiteral("utilities-terminal"),
                                      m_stoppedTitle,
                                      i18n("Start it to open a terminal in it. Its home and its volumes are still there."),
                                      startButton);
    m_stoppedPage->setObjectName(QStringLiteral("stoppedPage"));
    m_terminalStack->addWidget(m_stoppedPage);

    QPushButton *openButton = createButton(QStringLiteral("utilities-terminal"), i18nc("@action:button", "Open a Terminal"), QStringLiteral("openShellButton"));
    connect(openButton, &QPushButton::clicked, this, [this] {
        if (WorkboxTerminals *terminals = m_terminals.value(m_selectedId)) {
            terminals->openShell();
        }
    });
    m_noShellPage = createPlaceholder(QStringLiteral("utilities-terminal"), new QLabel(i18nc("@title", "No terminal open")), QString(), openButton);
    m_noShellPage->setObjectName(QStringLiteral("noShellPage"));
    m_terminalStack->addWidget(m_noShellPage);
    return m_terminalStack;
}

QWidget *MainWindow::createInformationTab()
{
    auto *tab = new QWidget;
    auto *layout = new QFormLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setVerticalSpacing(10);
    m_imageLabel = createValueLabel();
    m_homeLabel = createValueLabel();
    m_homeLabel->setObjectName(QStringLiteral("homeLabel"));
    m_colorLabel = new QLabel;
    m_colorLabel->setObjectName(QStringLiteral("colorLabel"));
    m_sudoLabel = createValueLabel();
    m_waylandLabel = createValueLabel();
    m_volumesLabel = createValueLabel();
    m_volumesLabel->setObjectName(QStringLiteral("volumesLabel"));
    layout->addRow(i18nc("@label a workbox's", "Image:"), m_imageLabel);
    layout->addRow(i18nc("@label a workbox's", "Home:"), m_homeLabel);
    layout->addRow(i18nc("@label a workbox's", "Color:"), m_colorLabel);
    layout->addRow(i18nc("@label a workbox's", "Sudo:"), m_sudoLabel);
    layout->addRow(i18nc("@label a workbox's", "Wayland:"), m_waylandLabel);
    layout->addRow(i18nc("@label a workbox's", "Volumes:"), m_volumesLabel);
    return tab;
}

QModelIndex MainWindow::selectedIndex() const
{
    for (int row = 0; row < m_model.rowCount(); ++row) {
        const QModelIndex index = m_model.index(row);
        if (index.data(WorkboxModel::IdRole).toString() == m_selectedId) {
            return index;
        }
    }
    return {};
}

void MainWindow::keepASelection()
{
    if (!selectedIndex().isValid() && m_model.rowCount() > 0) {
        m_selectedId = m_model.index(0).data(WorkboxModel::IdRole).toString();
        m_shellError.clear();
    }
    const QModelIndex shown = m_filter->mapFromSource(selectedIndex());
    if (shown.isValid() && m_list->currentIndex() != shown) {
        m_list->setCurrentIndex(shown);
    }
    showSelected();
}

void MainWindow::showSelected()
{
    const QModelIndex index = selectedIndex();
    if (!index.isValid()) {
        m_problemLabel->setText(m_model.problem());
        m_content->setCurrentWidget(m_model.problem().isEmpty() ? m_emptyPage : static_cast<QWidget *>(m_problemLabel));
        return;
    }
    m_content->setCurrentWidget(m_workboxPage);

    const Workbox workbox = index.data(WorkboxModel::WorkboxRole).value<Workbox>();
    const auto operation = static_cast<WorkboxModel::Operation>(index.data(WorkboxModel::OperationRole).toInt());
    const bool isIdle = operation == WorkboxModel::Operation::None;
    if (static_cast<WorkboxModel::Operation>(index.data(WorkboxModel::FailedOperationRole).toInt()) == WorkboxModel::Operation::Starting) {
        m_shouldOpenShell.remove(workbox.id);
    }

    m_swatch->setPixmap(swatchPixmap(workbox.color, headerSwatchSize, devicePixelRatioF()));
    m_nameLabel->setText(workbox.name);
    m_detailLabel->setText(i18nc("@info a workbox's id and status", "%1 · %2", workbox.id, statusText(index)));

    m_startButton->setVisible(!workbox.isRunning);
    m_stopButton->setVisible(workbox.isRunning);
    m_openInKonsoleButton->setVisible(workbox.isRunning);
    for (QPushButton *button : {m_startButton, m_stopButton, m_openInKonsoleButton, m_removeButton}) {
        button->setEnabled(isIdle);
    }

    QString errorText = index.data(WorkboxModel::ErrorTextRole).toString();
    for (const QString &other : {m_model.problem(), m_shellError}) {
        if (errorText.isEmpty()) {
            errorText = other;
        }
    }
    if (errorText.isEmpty()) {
        m_errorMessage->hide();
    } else {
        m_errorMessage->setText(errorText);
        m_errorMessage->show();
    }

    showTerminals(workbox);
    showInformation(workbox);
}

void MainWindow::showTerminals(const Workbox &workbox)
{
    if (!workbox.isRunning) {
        m_stoppedTitle->setText(i18nc("@title", "%1 is stopped", workbox.name));
        m_terminalStack->setCurrentWidget(m_stoppedPage);
        return;
    }
    WorkboxTerminals *terminals = m_terminals.value(workbox.id);
    if (!terminals) {
        terminals = new WorkboxTerminals(m_model, workbox.id, workbox.color);
        m_terminalStack->addWidget(terminals);
        m_terminals.insert(workbox.id, terminals);
        connect(terminals, &WorkboxTerminals::shellsChanged, this, &MainWindow::showSelected);
        connect(terminals, &WorkboxTerminals::failed, this, &MainWindow::showError);
        // The first time it's shown running, a shell in it at once.
        m_shouldOpenShell.insert(workbox.id);
    }
    if (m_shouldOpenShell.remove(workbox.id)) {
        terminals->openShell();
    }
    m_terminalStack->setCurrentWidget(terminals->count() > 0 ? static_cast<QWidget *>(terminals) : m_noShellPage);
}

void MainWindow::showInformation(const Workbox &workbox)
{
    m_imageLabel->setText(workbox.image);
    m_homeLabel->setText(volumeName(workbox.id, homeVolume));
    // The color itself, its value only in the tooltip.
    const qreal pixelRatio = devicePixelRatioF();
    const int colorSize = fontMetrics().height();
    QPixmap color(QSize(colorSize, colorSize) * pixelRatio);
    color.setDevicePixelRatio(pixelRatio);
    color.fill(Qt::transparent);
    QPainter painter(&color);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(workbox.color);
    painter.drawRoundedRect(QRectF(0, 0, colorSize, colorSize), colorSize * 0.25, colorSize * 0.25);
    painter.end();
    m_colorLabel->setPixmap(color);
    m_colorLabel->setToolTip(workbox.color.name());
    m_colorLabel->setAccessibleName(workbox.color.name());
    m_sudoLabel->setText(yesOrNo(workbox.hasSudo));
    m_waylandLabel->setText(yesOrNo(workbox.hasWayland));
    QStringList volumes;
    for (const Volume &volume : workbox.volumes) {
        volumes.append(volumeDescription(volume));
    }
    m_volumesLabel->setText(volumes.isEmpty() ? i18nc("@info a workbox's volumes", "None") : volumes.join(QLatin1Char('\n')));
}

void MainWindow::select(const QString &id)
{
    m_selectedId = id;
    m_shellError.clear();
    const QModelIndex shown = m_filter->mapFromSource(selectedIndex());
    if (shown.isValid()) {
        m_list->setCurrentIndex(shown);
    }
    showSelected();
}

void MainWindow::openTerminal(const QString &id)
{
    select(id);
    m_tabs->setCurrentWidget(m_terminalStack);
    if (!selectedIndex().data(WorkboxModel::RunningRole).toBool()) {
        startSelected();
    } else if (WorkboxTerminals *terminals = m_terminals.value(id)) {
        terminals->openShell();
    }
    // Its terminals, the first time, with a shell already.
    showSelected();
}

void MainWindow::startSelected()
{
    m_shouldOpenShell.insert(m_selectedId);
    m_model.start(m_selectedId);
}

void MainWindow::openSelectedInKonsole()
{
    const Workbox workbox = selectedIndex().data(WorkboxModel::WorkboxRole).value<Workbox>();
    const QStringList commandLine = commandLineWithColors(workbox.color, m_model.shellCommandLine(workbox.id));
    if (!QProcess::startDetached(QStringLiteral("konsole"), QStringList{QStringLiteral("-e")} + commandLine)) {
        showError(i18n("Konsole couldn't be started."));
    }
}

void MainWindow::enableCreating()
{
    for (QPushButton *button : std::as_const(m_createButtons)) {
        button->setEnabled(m_model.problem().isEmpty());
    }
}

void MainWindow::openCreateDialog()
{
    auto *dialog = new CreateWorkboxDialog(m_model, this);
    dialog->setObjectName(QStringLiteral("createDialog"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &CreateWorkboxDialog::created, this, &MainWindow::select);
    connect(dialog, &CreateWorkboxDialog::terminalRequested, this, &MainWindow::openTerminal);
    dialog->open();
}

void MainWindow::confirmRemoval(const Workbox &workbox)
{
    auto *dialog = new QDialog(this);
    dialog->setObjectName(QStringLiteral("removeDialog"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(i18nc("@title:window", "Remove Workbox"));
    auto *layout = new QVBoxLayout(dialog);

    auto *question = new QLabel(i18nc("@title", "Remove “%1”?", workbox.name));
    question->setTextFormat(Qt::PlainText);
    question->setFont(scaledFont(question->font(), 1.2, QFont::DemiBold));
    layout->addWidget(question);
    auto *explanation = new QLabel(workbox.isRunning
                                       ? i18n("Its container, its home and its volumes will be removed, once it's stopped. This can't be undone.")
                                       : i18n("Its container, its home and its volumes will be removed. This can't be undone."));
    explanation->setWordWrap(true);
    layout->addWidget(explanation);

    // Ids are [a-z0-9-]: nothing in one is markup.
    auto *confirmationLabel = new QLabel(i18n("Type <b>%1</b> to confirm:", workbox.id));
    layout->addWidget(confirmationLabel);
    auto *confirmationEdit = new QLineEdit;
    confirmationEdit->setObjectName(QStringLiteral("confirmationEdit"));
    confirmationLabel->setBuddy(confirmationEdit);
    layout->addWidget(confirmationEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    QPushButton *removeButton = buttons->addButton(i18nc("@action:button a workbox", "Remove"), QDialogButtonBox::AcceptRole);
    removeButton->setObjectName(QStringLiteral("confirmRemoveButton"));
    removeButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    removeButton->setEnabled(false);
    connect(confirmationEdit, &QLineEdit::textChanged, removeButton, [removeButton, id = workbox.id](const QString &text) {
        removeButton->setEnabled(text == id);
    });
    connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    layout->addWidget(buttons);

    connect(dialog, &QDialog::accepted, this, [this, id = workbox.id] {
        m_model.remove(id);
    });
    dialog->open();
}

void MainWindow::showError(const QString &errorText)
{
    m_shellError = errorText;
    showSelected();
}
