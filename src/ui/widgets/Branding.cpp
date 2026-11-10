#include "ui/widgets/Branding.h"
#include "BinaryData.h"
#include "ui/theme/Fonts.h"
#include "ui/theme/Tokens.h"

namespace lithium::branding
{

using namespace tokens;

namespace
{
    juce::Font familyFont (float h = textSize::lg)    { return fonts::captionBold (h).withExtraKerningFactor (0.14f); }
    juce::Font productFont (float h = textSize::lg)   { return fonts::caption (h).withExtraKerningFactor (0.14f); }

    const juce::String familyText ("LITHIUM"), productText (" STUDIO");
    constexpr float logoGap = 8.0f;

    float stringWidth (const juce::Font& f, const juce::String& s)   { return juce::GlyphArrangement::getStringWidth (f, s); }
}

juce::Image getLogo()
{
    // ImageCache decodes the embedded PNG once and keeps it, so asking for it
    // again costs nothing.
    return juce::ImageCache::getFromMemory (BinaryData::io_github_thecalamityjoe87_LithiumStudio_png,
                                            BinaryData::io_github_thecalamityjoe87_LithiumStudio_pngSize);
}

int getWidth()
{
    return (int) std::ceil ((float) size::logoSize + logoGap
                            + stringWidth (familyFont(), familyText) + stringWidth (productFont(), productText));
}

void paintLogo (juce::Graphics& g, juce::Rectangle<float> tile)
{
    const auto logo = getLogo();

    if (! logo.isValid())
        return;

    // In the SVG's 256 units (Lithium Drums' icon proportions): the tile is
    // 236 square at 10 in from the edge, with corners of 51, and a 3-unit
    // border inside its edge. Draw what's inside the border, clipped to its
    // rounded shape.
    constexpr float margin = 10.0f, border = 3.0f, tileSize = 236.0f, cornerRadius = 51.0f;
    const auto scale = (float) logo.getWidth() / 256.0f;
    const auto source = logo.getBounds().toFloat().reduced ((margin + border) * scale);
    const auto innerRadius = tile.getWidth() * (cornerRadius - border) / (tileSize - 2.0f * border);

    juce::Graphics::ScopedSaveState saved (g);
    juce::Path shape;
    shape.addRoundedRectangle (tile, innerRadius);
    g.reduceClipRegion (shape);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (logo, tile.getX(), tile.getY(), tile.getWidth(), tile.getHeight(),
                 juce::roundToInt (source.getX()), juce::roundToInt (source.getY()),
                 juce::roundToInt (source.getWidth()), juce::roundToInt (source.getHeight()));
}

void paint (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float logoSize = (float) size::logoSize;
    paintLogo (g, area.removeFromLeft (logoSize).withSizeKeepingCentre (logoSize, logoSize));
    area.removeFromLeft (logoGap);
    paintWordmark (g, area, textSize::lg, juce::Justification::centredLeft);
}

void paintWordmark (juce::Graphics& g, juce::Rectangle<float> area, float fontHeight, juce::Justification justification)
{
    const auto family = familyFont (fontHeight), product = productFont (fontHeight);
    const auto familyWidth = stringWidth (family, familyText);
    const auto width = juce::jmin (area.getWidth(), familyWidth + stringWidth (product, productText));
    auto row = justification.appliedToRectangle (juce::Rectangle<float> (width, area.getHeight()), area);

    g.setColour (theme().text);
    g.setFont (family);
    g.drawText (familyText, row.removeFromLeft (familyWidth), juce::Justification::centredLeft, true);
    g.setColour (theme().textSecondary);
    g.setFont (product);
    g.drawText (productText, row, juce::Justification::centredLeft, true);
}

} // namespace lithium::branding
