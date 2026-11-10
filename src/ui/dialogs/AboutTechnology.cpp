#include "ui/dialogs/AboutTechnology.h"
#include "BinaryData.h"
#include "ui/theme/Fonts.h"
#include "ui/theme/Theme.h"
#include "ui/theme/Tokens.h"

namespace lithium
{

using namespace tokens;

namespace
{
    constexpr int cardHeight = 78, cardGap = 10, cardPadding = 14, logoHeight = 40, logoMaxWidth = 110;
    constexpr int rowHeight = 26, tableGap = 14;
    constexpr int footerHeight = 34;

    juce::Image loadImage (const void* data, int size)
    {
        return juce::ImageFileFormat::loadFrom (data, (size_t) size);
    }
}

//==============================================================================
// One project: a card with its logo, or a table row. A button, so it can be
// clicked, focused and read out by a screen reader.
class AboutTechnology::Entry  : public juce::Button
{
public:
    Entry (juce::String nameIn, juce::String whatIn, juce::String licenseIn, juce::String urlIn, juce::Image logoIn = {})
        : juce::Button (nameIn), name (nameIn), what (whatIn), license (licenseIn), url (urlIn), logo (logoIn)
    {
        setTitle (name);
        setDescription (what + ". " + license + ".");
        setTooltip (url);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        onClick = [this] { juce::URL (url).launchInDefaultBrowser(); };
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        if (logo.isValid())
            paintCard (g, over, down);
        else
            paintRow (g, over, down);
    }

private:
    void paintCard (juce::Graphics& g, bool over, bool down)
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? theme().rowPressed : over ? theme().rowHover : theme().cardBg);
        g.fillRoundedRectangle (bounds, size::radiusCard);
        g.setColour (theme().border);
        g.drawRoundedRectangle (bounds, size::radiusCard, 1.0f);

        auto r = getLocalBounds().reduced (cardPadding, 0);
        const auto logoWidth = juce::jmin (logoMaxWidth, juce::roundToInt ((float) logo.getWidth() * (float) logoHeight / (float) logo.getHeight()));

        // drawImage draws at the current colour's opacity.
        g.setOpacity (1.0f);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (logo, r.removeFromLeft (logoWidth).withSizeKeepingCentre (logoWidth, logoHeight).toFloat(),
                     juce::RectanglePlacement::centred);
        r.removeFromLeft (14);

        auto lines = r.withSizeKeepingCentre (r.getWidth(), 52);
        g.setColour (theme().text);
        g.setFont (fonts::uiSemiBold (textSize::lg));
        g.drawText (name, lines.removeFromTop (18), juce::Justification::centredLeft, true);

        g.setColour (theme().textSecondary);
        g.setFont (fonts::ui (textSize::sm));
        g.drawFittedText (what, lines.removeFromTop (18), juce::Justification::centredLeft, 1, 0.9f);

        g.setColour (theme().accent);
        g.setFont (fonts::mono (textSize::xs));
        g.drawText (license, lines, juce::Justification::centredLeft, true);
    }

    void paintRow (juce::Graphics& g, bool over, bool down)
    {
        if (over || down)
        {
            g.setColour (down ? theme().rowPressed : theme().rowHover);
            g.fillRoundedRectangle (getLocalBounds().toFloat(), size::radiusSmall);
        }

        // Name on the left half, license (in mono) on the right half.
        auto r = getLocalBounds().reduced (4, 0);
        auto left = r.removeFromLeft (r.getWidth() / 2);
        g.setColour (theme().text);
        g.setFont (fonts::ui (textSize::lg));
        g.drawText (name, left, juce::Justification::centredLeft, true);

        g.setColour (theme().textSecondary);
        g.setFont (fonts::mono (textSize::smPlus));
        g.drawText (license, r, juce::Justification::centredLeft, true);
    }

    juce::String name, what, license, url;
    juce::Image logo;
};

//==============================================================================
AboutTechnology::AboutTechnology()
{
    featured.add (new Entry ("Tracktion Engine", "Playback, recording, editing, plug-ins and mixing",
                             "GPLv3", "https://github.com/Tracktion/tracktion_engine",
                             loadImage (BinaryData::tracktion_engine_powered_png, BinaryData::tracktion_engine_powered_pngSize)));
    featured.add (new Entry ("JUCE", "Windows, graphics, and audio and MIDI devices",
                             "AGPLv3", "https://juce.com",
                             loadImage (BinaryData::juce_icon_png, BinaryData::juce_icon_pngSize)));

    others.add (new Entry ("sfizz", "Plays the sampled instruments", "BSD 2-Clause", "https://sfz.tools/sfizz"));
    others.add (new Entry ("Rubber Band Library", "Stretches loops and recordings to the song's tempo and key",
                           "GPLv2 or later", "https://breakfastquay.com/rubberband"));
    others.add (new Entry ("Guitarix and GxPlugins", "The sound of the Amp and most of the pedalboard's pedals",
                           "GPL", "https://github.com/brummer10/guitarix"));
    others.add (new Entry ("KlonCentaur and chowdsp_utils", "The sound of the Golden Boost pedal",
                           "BSD 3-Clause, GPLv3", "https://github.com/jatinchowdhury18/KlonCentaur"));
    others.add (new Entry ("fat1 and zita-at1", "The pitch correction behind the Tune effect",
                           "GPLv2 or later", "https://github.com/x42/fat1.lv2"));
    others.add (new Entry ("pitch-detection", "Finds the notes in the audio editor's Pitch mode",
                           "MIT", "https://github.com/sevagh/pitch-detection"));
    others.add (new Entry ("LAME", "Encodes MP3 exports", "LGPL", "https://lame.sourceforge.io"));
    others.add (new Entry ("miniz", "Reads and writes .lisession files", "MIT", "https://github.com/richgel999/miniz"));
    others.add (new Entry ("Lucide icons", "Some of the instrument icons", "ISC", "https://lucide.dev"));
    others.add (new Entry ("Barlow, IBM Plex Mono and Figtree fonts", "The app's typefaces",
                           "SIL Open Font License", "https://openfontlicense.org"));

    for (auto* e : featured) addAndMakeVisible (e);
    for (auto* e : others)   addAndMakeVisible (e);
}

AboutTechnology::~AboutTechnology() = default;

int AboutTechnology::getHeightForWidth (int) const
{
    return cardHeight + tableGap + others.size() * rowHeight + footerHeight;
}

void AboutTechnology::resized()
{
    auto r = getLocalBounds();

    auto cards = r.removeFromTop (cardHeight);
    const auto cardWidth = (cards.getWidth() - cardGap * (featured.size() - 1)) / featured.size();

    for (auto* e : featured)
    {
        e->setBounds (cards.removeFromLeft (cardWidth));
        cards.removeFromLeft (cardGap);
    }

    r.removeFromTop (tableGap);

    for (auto* e : others)
        e->setBounds (r.removeFromTop (rowHeight));

    footer = r.removeFromTop (footerHeight);
}

void AboutTechnology::paint (juce::Graphics& g)
{
    g.setColour (theme().textSecondary);
    g.setFont (fonts::ui (textSize::sm));
    g.drawFittedText ("Click a project to visit its website. Their full notices are in the CREDITS file that comes with Lithium Studio.",
                      footer.withTrimmedLeft (4), juce::Justification::bottomLeft, 2, 1.0f);
}

} // namespace lithium
