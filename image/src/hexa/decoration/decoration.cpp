// Hexa's window decoration: a title bar in the color scheme's title bar
// colors, the buttons on each side as the user arranges them (Hexa's Global
// Themes put the app's icon on the left and minimize, maximize and close on
// the right) and the title centered on the window. KWin rounds the window's
// corners with it.

#include "button.h"

#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/Decoration>
#include <KDecoration3/DecorationButtonGroup>
#include <KDecoration3/DecorationSettings>
#include <KPluginFactory>
#include <QPainter>

#include <algorithm>

namespace
{
// px. Breeze's is 5.
constexpr qreal WINDOW_CORNER_RADIUS = 8;
constexpr qreal TITLE_BAR_HEIGHT = 40;
// Between each button group and the window's edge.
constexpr qreal BUTTONS_MARGIN = 8;
constexpr qreal BUTTON_SPACING = 2;
// Between the title and the buttons.
constexpr qreal TITLE_MARGIN = 16;
constexpr qreal RESIZE_BORDER = 6;
}

class HexaDecoration : public KDecoration3::Decoration
{
    Q_OBJECT

public:
    // KPluginFactory's registerPlugin looks for this signature; it doesn't
    // see an inherited constructor.
    HexaDecoration(QObject *parent, const QVariantList &args)
        : Decoration(parent, args)
    {
    }

    bool init() override
    {
        const auto *decorated = window();
        m_leftButtons = new KDecoration3::DecorationButtonGroup(KDecoration3::DecorationButtonGroup::Position::Left, this, &HexaButton::create);
        m_rightButtons = new KDecoration3::DecorationButtonGroup(KDecoration3::DecorationButtonGroup::Position::Right, this, &HexaButton::create);
        m_leftButtons->setSpacing(BUTTON_SPACING);
        m_rightButtons->setSpacing(BUTTON_SPACING);

        // Their width changes with the buttons the user chooses.
        connect(m_leftButtons, &KDecoration3::DecorationButtonGroup::geometryChanged, this, &HexaDecoration::positionButtons);
        connect(m_rightButtons, &KDecoration3::DecorationButtonGroup::geometryChanged, this, &HexaDecoration::positionButtons);
        connect(decorated, &KDecoration3::DecoratedWindow::widthChanged, this, &HexaDecoration::updateLayout);
        connect(decorated, &KDecoration3::DecoratedWindow::maximizedChanged, this, &HexaDecoration::updateLayout);
        connect(decorated, &KDecoration3::DecoratedWindow::activeChanged, this, [this] {
            update();
        });
        connect(decorated, &KDecoration3::DecoratedWindow::captionChanged, this, [this] {
            update();
        });
        connect(decorated, &KDecoration3::DecoratedWindow::paletteChanged, this, [this] {
            update();
        });
        // The app's icon button: an app may set its icon after its window shows.
        connect(decorated, &KDecoration3::DecoratedWindow::iconChanged, this, [this] {
            update();
        });
        connect(settings().get(), &KDecoration3::DecorationSettings::fontChanged, this, [this] {
            update();
        });

        // The rounded corners show what's behind.
        setOpaque(false);
        updateLayout();
        return true;
    }

    void paint(QPainter *painter, const QRectF &repaintArea) override
    {
        const auto *decorated = window();
        const auto colorGroup = decorated->isActive() ? KDecoration3::ColorGroup::Active : KDecoration3::ColorGroup::Inactive;
        const QRectF titleBar(0, 0, size().width(), borderTop());

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(decorated->color(colorGroup, KDecoration3::ColorRole::TitleBar));
        // Only its top corners rounded: a rounded rectangle reaching below it,
        // cut at its bottom.
        const qreal radius = cornerRadius();
        painter->setClipRect(titleBar);
        painter->drawRoundedRect(titleBar.adjusted(0, 0, 0, radius), radius, radius);
        painter->setClipping(false);

        QFont titleFont = settings()->font();
        titleFont.setWeight(QFont::DemiBold);
        painter->setFont(titleFont);
        painter->setPen(decorated->color(colorGroup, KDecoration3::ColorRole::Foreground));
        const QRectF titleArea = centeredTitleArea(titleBar);
        const QString title = painter->fontMetrics().elidedText(decorated->caption(), Qt::ElideRight, titleArea.width());
        painter->drawText(titleArea, Qt::AlignCenter, title);
        painter->restore();

        m_leftButtons->paint(painter, repaintArea);
        m_rightButtons->paint(painter, repaintArea);
    }

private:
    // Square when maximized, as Breeze's.
    qreal cornerRadius() const
    {
        return window()->isMaximized() ? 0 : WINDOW_CORNER_RADIUS;
    }

    // As wide as it can be while staying centered on the window, clear of
    // the buttons on both sides.
    QRectF centeredTitleArea(const QRectF &titleBar) const
    {
        const qreal leftTaken = m_leftButtons->geometry().right();
        const qreal rightTaken = titleBar.right() - m_rightButtons->geometry().left();
        const qreal sideTaken = std::max(leftTaken, rightTaken) + TITLE_MARGIN;
        return titleBar.adjusted(sideTaken, 0, -sideTaken, 0);
    }

    void updateLayout()
    {
        const bool isMaximized = window()->isMaximized();
        setBorders(QMarginsF(0, TITLE_BAR_HEIGHT, 0, 0));
        setResizeOnlyBorders(isMaximized ? QMarginsF() : QMarginsF(RESIZE_BORDER, RESIZE_BORDER, RESIZE_BORDER, RESIZE_BORDER));
        setTitleBar(QRectF(0, 0, window()->width(), TITLE_BAR_HEIGHT));
        setBorderRadius(KDecoration3::BorderRadius(cornerRadius()));
        positionButtons();
    }

    void positionButtons()
    {
        m_leftButtons->setPos(QPointF(BUTTONS_MARGIN, (TITLE_BAR_HEIGHT - m_leftButtons->geometry().height()) / 2));
        m_rightButtons->setPos(QPointF(window()->width() - m_rightButtons->geometry().width() - BUTTONS_MARGIN,
                                       (TITLE_BAR_HEIGHT - m_rightButtons->geometry().height()) / 2));
        update();
    }

    KDecoration3::DecorationButtonGroup *m_leftButtons = nullptr;
    KDecoration3::DecorationButtonGroup *m_rightButtons = nullptr;
};

K_PLUGIN_FACTORY_WITH_JSON(HexaDecorationFactory, "com.brescaos.hexa.decoration.json", registerPlugin<HexaDecoration>();)

#include "decoration.moc"
