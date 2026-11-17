// Hexa's window decoration: a title bar in the color scheme's title bar
// colors, the buttons on each side as the user arranges them (Hexa's Global
// Themes put the app's icon on the left and minimize, maximize and close on
// the right) and the title centered on the window. KWin rounds the window's
// corners with it and draws a line around the window with the focus in a
// darker accent color, the same in light and dark color schemes. Every
// window, with the focus or not, shares one soft shadow below it.

#include "button.h"

#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/Decoration>
#include <KDecoration3/DecorationButtonGroup>
#include <KDecoration3/DecorationSettings>
#include <KDecoration3/DecorationShadow>
#include <KPluginFactory>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <memory>

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
constexpr qreal WINDOW_BORDER_WIDTH = 2;
// QColor::darker()'s factor: 130 makes it 30% darker.
constexpr int WINDOW_BORDER_DARKER_FACTOR = 130;
// The shadow, black in light and dark color schemes: a CSS box-shadow's
// y offset and blur (twice the gaussian's standard deviation), in px.
constexpr qreal SHADOW_OFFSET_Y = 4;
constexpr qreal SHADOW_BLUR = 16;
constexpr qreal SHADOW_OPACITY = 0.5;

// How much of the span from start to end, blurred by a gaussian, covers
// position.
qreal blurredSpanCoverage(qreal position, qreal start, qreal end, qreal sigma)
{
    const qreal scale = 1 / (sigma * std::sqrt(2.0));
    return 0.5 * (std::erf((end - position) * scale) - std::erf((start - position) * scale));
}

// KWin draws the image's corners as they are and stretches its middle row and
// column along the window's sides, so the window in the image only needs to
// be big enough that its blurred edges don't reach its middle.
std::shared_ptr<KDecoration3::DecorationShadow> createShadow()
{
    const qreal sigma = SHADOW_BLUR / 2;
    // Past three standard deviations the shadow is invisible.
    const int reach = int(std::ceil(3 * sigma));
    const int windowSide = 2 * reach;
    const int imageSide = 2 * reach + windowSide;
    const QRectF window(reach, reach - SHADOW_OFFSET_Y, windowSide, windowSide);
    const QRectF shadowBox = window.translated(0, SHADOW_OFFSET_Y);

    QImage image(imageSide, imageSide, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < imageSide; ++y) {
        const qreal coverageY = blurredSpanCoverage(y + 0.5, shadowBox.top(), shadowBox.bottom(), sigma);
        auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < imageSide; ++x) {
            const qreal coverageX = blurredSpanCoverage(x + 0.5, shadowBox.left(), shadowBox.right(), sigma);
            line[x] = qRgba(0, 0, 0, qRound(255 * SHADOW_OPACITY * coverageX * coverageY));
        }
    }

    // Nothing under the window: it shows through its rounded corners.
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawRoundedRect(window, WINDOW_CORNER_RADIUS, WINDOW_CORNER_RADIUS);
    painter.end();

    auto shadow = std::make_shared<KDecoration3::DecorationShadow>();
    shadow->setShadow(image);
    shadow->setPadding(QMarginsF(window.left(), window.top(), imageSide - window.right(), imageSide - window.bottom()));
    shadow->setInnerShadowRect(QRectF(window.center(), QSizeF(1, 1)));
    return shadow;
}

// Created once and shared by every window while one has the decoration.
std::shared_ptr<KDecoration3::DecorationShadow> sharedShadow()
{
    static std::weak_ptr<KDecoration3::DecorationShadow> cachedShadow;
    auto shadow = cachedShadow.lock();
    if (!shadow) {
        shadow = createShadow();
        cachedShadow = shadow;
    }
    return shadow;
}
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
            updateBorder();
            update();
        });
        connect(decorated, &KDecoration3::DecoratedWindow::captionChanged, this, [this] {
            update();
        });
        connect(decorated, &KDecoration3::DecoratedWindow::paletteChanged, this, [this] {
            updateBorder();
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
        setShadow(sharedShadow());
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
        updateBorder();
        positionButtons();
    }

    // Only around the window with the focus, and not when maximized.
    void updateBorder()
    {
        if (!window()->isActive() || window()->isMaximized()) {
            setBorderOutline(KDecoration3::BorderOutline());
            return;
        }
        const QColor accent = window()->palette().color(QPalette::Active, QPalette::Highlight);
        setBorderOutline(KDecoration3::BorderOutline(WINDOW_BORDER_WIDTH,
                                                     accent.darker(WINDOW_BORDER_DARKER_FACTOR),
                                                     KDecoration3::BorderRadius(cornerRadius())));
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
