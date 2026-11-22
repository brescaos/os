#include "dialogs/formparts.h"

#include <KColorScheme>

#include <QDir>
#include <QFontDatabase>
#include <QLabel>
#include <QRadioButton>
#include <QStyle>
#include <QVBoxLayout>

QLabel *createSectionTitle(const QString &text)
{
    auto *title = new QLabel(text);
    QFont font = title->font();
    font.setWeight(QFont::DemiBold);
    title->setFont(font);
    return title;
}

QLabel *createExplanation(const QString &text)
{
    auto *label = new QLabel(text);
    label->setWordWrap(true);
    label->setForegroundRole(QPalette::PlaceholderText);
    return label;
}

void setMonospace(QWidget *widget)
{
    widget->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

void setNegative(QLabel *label, bool isNegative)
{
    if (isNegative) {
        QPalette palette = label->palette();
        palette.setBrush(QPalette::WindowText, KColorScheme(QPalette::Active, KColorScheme::View).foreground(KColorScheme::NegativeText));
        label->setPalette(palette);
    }
    label->setForegroundRole(isNegative ? QPalette::WindowText : QPalette::PlaceholderText);
}

QWidget *createOption(QAbstractButton *button, const QString &explanation)
{
    auto *option = new QWidget;
    auto *layout = new QVBoxLayout(option);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(button);
    QLabel *label = createExplanation(explanation);
    const QStyle *style = button->style();
    const bool isRadio = qobject_cast<QRadioButton *>(button);
    const int indent = isRadio ? style->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth) + style->pixelMetric(QStyle::PM_RadioButtonLabelSpacing)
                               : style->pixelMetric(QStyle::PM_IndicatorWidth) + style->pixelMetric(QStyle::PM_CheckBoxLabelSpacing);
    label->setContentsMargins(indent, 0, 0, 0);
    layout->addWidget(label);
    return option;
}

// The home's real path: the one volumes have (volumes.h).
QString homeRelativePath(const QString &path)
{
    const QString home = QDir(QDir::homePath()).canonicalPath();
    if (path == home) {
        return QStringLiteral("~");
    }
    return path.startsWith(home + QLatin1Char('/')) ? QLatin1Char('~') + path.mid(home.size()) : path;
}
