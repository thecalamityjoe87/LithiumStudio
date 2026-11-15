#include "ui/bottompanel/PanelTab.h"
#include "ui/widgets/PaintProbe.h"
#include "ui/widgets/Icons.h"
#include "ui/theme/Fonts.h"
#include "ui/theme/Theme.h"
#include "ui/theme/Tokens.h"

namespace lithium
{

using namespace tokens;

PanelTab::PanelTab() : juce::Button ("Show panel")
{
    setTooltip ("Show panel");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void PanelTab::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    LITHIUM_PERF_PAINT ("PanelTab");

    // Rounded top corners only: the bottom joins the panel. The border is
    // the outer shape showing around the inner one, which runs down to the
    // bottom edge and so covers the panel's top border under the tab.
    auto shape = [] (juce::Rectangle<float> r, float radius)
    {
        juce::Path p;
        p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radius, radius,
                               true, true, false, false);
        return p;
    };

    const auto bounds = getLocalBounds().toFloat();
    g.setColour (theme().bottomPanelBorder);
    g.fillPath (shape (bounds, size::radiusSmall));
    g.setColour (theme().bottomPanelBg);
    g.fillPath (shape (bounds.reduced (1.0f, 0.0f).withTrimmedTop (1.0f), size::radiusSmall - 1.0f));

    // A chevron and "Show panel", centered above the 1px that overlaps the panel.
    const auto colour = (isHighlighted || isDown) ? theme().text : theme().textSecondary;
    const auto font = fonts::ui (textSize::sm);
    const juce::String label ("Show panel");
    constexpr float iconSize = 12.0f, gap = 5.0f;
    const auto textWidth = juce::GlyphArrangement::getStringWidth (font, label);

    auto content = bounds.withTrimmedBottom (1.0f).withSizeKeepingCentre (iconSize + gap + textWidth + 2.0f,
                                                                          bounds.getHeight() - 1.0f);
    icons::draw (g, Icon::chevronUp, content.removeFromLeft (iconSize), colour, iconSize, 1.8f);
    content.removeFromLeft (gap);

    g.setColour (colour);
    g.setFont (font);
    g.drawText (label, content, juce::Justification::centredLeft, false);
}

} // namespace lithium
