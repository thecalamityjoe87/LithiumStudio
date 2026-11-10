#include "ui/dialogs/AboutDialog.h"
#include "BinaryData.h"
#include "engine/SoundLibrary.h"
#include "ui/dialogs/AboutTechnology.h"
#include "ui/theme/Fonts.h"
#include "ui/theme/Theme.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/Branding.h"
#include "ui/widgets/Licenses.h"

namespace lithium
{

using namespace tokens;

namespace
{
    constexpr int brandHeight = 80, iconSize = 76, tabsHeight = 34;
    constexpr int topButtonWidth = 110, topButtonHeight = 32;

    const juce::String projectUrl ("https://github.com/thecalamityjoe87/LithiumStudio");

    juce::String versionText()
    {
        return "Version " + juce::JUCEApplication::getInstance()->getApplicationVersion() + " (" LITHIUM_STAGE ")";
    }

    // Uppercase caption text with a little letter-spacing, like Lithium Drums' labels.
    juce::Font captionFont (float size, float kerning = 0.14f)
    {
        return fonts::caption (size).withExtraKerningFactor (kerning);
    }

    float widthOf (const juce::Font& font, const juce::String& text)
    {
        return juce::GlyphArrangement::getStringWidth (font, text);
    }

    // A tab: plain text, white when chosen (the dialog draws the underline).
    // Hovering only brightens the word, like Lithium Drums' tabs.
    IconButton::Colours tabColours()
    {
        IconButton::Colours c { juce::Colours::transparentBlack, theme().textSecondary, juce::Colours::transparentBlack, theme().text };
        c.hoverBackground = juce::Colours::transparentBlack;
        c.hoverForeground = theme().text;
        return c;
    }
}

//==============================================================================
/** One tab's page: a column of paragraphs, section headings, bullet lists,
    credit rows and the Built With block, laid out top to bottom. */
class AboutDialog::Page  : public juce::Component
{
public:
    enum class Tone { body, secondary };

    struct Credit
    {
        juce::String title, by, detail, description;
        juce::String license;     // SPDX id
        juce::String sourceUrl;
    };

    void addParagraph (const juce::String& text, float size = textSize::lg, Tone tone = Tone::body)
    {
        blocks.push_back ({ Kind::paragraph, text, size, tone });
    }

    void addHeading (const juce::String& text)
    {
        blocks.push_back ({ Kind::heading, text.toUpperCase() });
    }

    void addBullets (const juce::StringArray& lines)
    {
        Block b { Kind::bullets };
        b.lines = lines;
        blocks.push_back (std::move (b));
    }

    void addCredit (const Credit& c)
    {
        Block b { Kind::credit };
        b.credit = c;

        b.licenseLink = std::make_unique<juce::HyperlinkButton> (licenses::displayName (c.license), licenses::url (c.license));
        b.licenseLink->setTooltip (licenses::url (c.license).toString (false));
        b.licenseLink->setVisible (c.license.isNotEmpty());

        b.sourceLink = std::make_unique<juce::HyperlinkButton> ("Source", juce::URL (c.sourceUrl));
        b.sourceLink->setTooltip (c.sourceUrl);
        b.sourceLink->setVisible (c.sourceUrl.isNotEmpty());

        for (auto* link : { b.licenseLink.get(), b.sourceLink.get() })
        {
            link->setFont (fonts::ui (textSize::md), false, juce::Justification::centredRight);
            addChildComponent (*link);
        }

        blocks.push_back (std::move (b));
        applyLinkColours();
    }

    void addTechnology()
    {
        Block b { Kind::technology };
        b.technology = std::make_unique<AboutTechnology>();
        addAndMakeVisible (*b.technology);
        blocks.push_back (std::move (b));
    }

    void applyLinkColours()
    {
        for (auto& b : blocks)
            for (auto* link : { b.licenseLink.get(), b.sourceLink.get() })
                if (link != nullptr)
                    link->setColour (juce::HyperlinkButton::textColourId, theme().accent);
    }

    /** Lays the blocks out at this width and sets the page's height to fit. */
    void layout (int width)
    {
        int y = 0;

        for (size_t i = 0; i < blocks.size(); ++i)
        {
            auto& b = blocks[i];

            switch (b.kind)
            {
                case Kind::paragraph:
                {
                    const auto h = textHeight (b.text, fonts::ui (b.size), width);
                    b.area = { 0, y, width, h };
                    y += h + 10;
                    break;
                }

                case Kind::heading:
                    y += i == 0 ? 0 : 12;
                    b.area = { 0, y, width, 28 };
                    y += 28 + 10;
                    break;

                case Kind::bullets:
                {
                    const auto top = y;

                    for (auto& line : b.lines)
                        y += textHeight (line, fonts::ui (textSize::lg), width - bulletIndent) + 6;

                    b.area = { 0, top, width, y - top };
                    y += 6;
                    break;
                }

                case Kind::credit:
                {
                    const auto& c = b.credit;
                    int h = 10 + 22 + (c.by.isNotEmpty() ? 19 : 0) + (c.detail.isNotEmpty() ? 18 : 0);

                    if (c.description.isNotEmpty())
                        h += 4 + textHeight (c.description, fonts::ui (textSize::md), width);

                    h += 12;
                    b.area = { 0, y, width, h };

                    // The links, right-aligned on the title's line: "CC BY 4.0 • Source".
                    auto line = juce::Rectangle<int> (0, y + 10, width, 22);
                    const auto linkFont = fonts::ui (textSize::md);

                    if (b.sourceLink->isVisible())
                        b.sourceLink->setBounds (line.removeFromRight (juce::roundToInt (widthOf (linkFont, "Source")) + 4));

                    b.dotArea = b.sourceLink->isVisible() && b.licenseLink->isVisible() ? line.removeFromRight (16) : juce::Rectangle<int>();

                    if (b.licenseLink->isVisible())
                        b.licenseLink->setBounds (line.removeFromRight (juce::roundToInt (widthOf (linkFont, b.licenseLink->getButtonText())) + 4));

                    b.titleRight = line.getRight() - 12;
                    y += h;
                    break;
                }

                case Kind::technology:
                {
                    const auto h = b.technology->getHeightForWidth (width);
                    b.technology->setBounds (0, y, width, h);
                    b.area = { 0, y, width, h };
                    y += h + 10;
                    break;
                }
            }
        }

        setSize (width, y + 8);
    }

    void paint (juce::Graphics& g) override
    {
        for (size_t i = 0; i < blocks.size(); ++i)
        {
            const auto& b = blocks[i];

            switch (b.kind)
            {
                case Kind::paragraph:
                    drawText (g, b.text, fonts::ui (b.size), colourOf (b.tone), b.area);
                    break;

                case Kind::heading:
                {
                    // "LICENSE" with a rule under it.
                    g.setColour (theme().textSecondary);
                    g.setFont (captionFont (textSize::md));
                    g.drawText (b.text, b.area.withTrimmedBottom (8), juce::Justification::bottomLeft, false);
                    g.setColour (theme().border);
                    g.fillRect (b.area.withTop (b.area.getBottom() - 1));
                    break;
                }

                case Kind::bullets:
                {
                    auto r = b.area;
                    const auto font = fonts::ui (textSize::lg);

                    for (auto& line : b.lines)
                    {
                        const auto h = textHeight (line, font, r.getWidth() - bulletIndent);
                        auto row = r.removeFromTop (h);
                        r.removeFromTop (6);
                        // A small dot, centred on the first line.
                        g.setColour (theme().accent);
                        const auto firstLineCentre = (float) row.getY() + font.getHeight() * 0.6f;
                        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ (float) row.getX() + 4.0f, firstLineCentre }));
                        row.removeFromLeft (bulletIndent);
                        drawText (g, line, font, colourOf (Tone::body), row);
                    }
                    break;
                }

                case Kind::credit:
                {
                    const auto& c = b.credit;
                    auto r = b.area.withTrimmedTop (10);

                    g.setColour (theme().text);
                    g.setFont (fonts::uiSemiBold (textSize::xl));
                    g.drawText (c.title, r.removeFromTop (22).withRight (b.titleRight), juce::Justification::centredLeft, true);

                    if (! b.dotArea.isEmpty())
                    {
                        g.setColour (theme().accent);
                        g.setFont (fonts::ui (textSize::md));
                        g.drawText (juce::String::fromUTF8 ("\xe2\x80\xa2"), b.dotArea, juce::Justification::centred, false);
                    }

                    if (c.by.isNotEmpty())
                    {
                        g.setColour (theme().textSecondary);
                        g.setFont (fonts::ui (textSize::md));
                        g.drawText (c.by, r.removeFromTop (19), juce::Justification::centredLeft, true);
                    }

                    if (c.detail.isNotEmpty())
                    {
                        g.setColour (theme().textSecondary);
                        g.setFont (fonts::ui (textSize::sm));
                        g.drawText (c.detail, r.removeFromTop (18), juce::Justification::centredLeft, true);
                    }

                    if (c.description.isNotEmpty())
                    {
                        r.removeFromTop (4);
                        drawText (g, c.description, fonts::ui (textSize::md), colourOf (Tone::body), r);
                    }

                    // A line between rows, not after the last one in its section.
                    const bool lastInSection = i + 1 == blocks.size() || blocks[i + 1].kind != Kind::credit;

                    if (! lastInSection)
                    {
                        g.setColour (theme().border);
                        g.fillRect (b.area.withTop (b.area.getBottom() - 1));
                    }
                    break;
                }

                case Kind::technology:
                    break;
            }
        }
    }

private:
    enum class Kind { paragraph, heading, bullets, credit, technology };

    struct Block
    {
        Kind kind;
        juce::String text;
        float size = textSize::lg;
        Tone tone = Tone::body;
        juce::StringArray lines;
        Credit credit;
        std::unique_ptr<juce::HyperlinkButton> licenseLink, sourceLink;
        std::unique_ptr<AboutTechnology> technology;
        juce::Rectangle<int> area, dotArea;
        int titleRight = 0;
    };

    static constexpr int bulletIndent = 18;

    static juce::Colour colourOf (Tone tone)
    {
        return tone == Tone::body ? theme().text.withAlpha (0.85f) : theme().textSecondary;
    }

    static juce::AttributedString attributed (const juce::String& text, const juce::Font& font, juce::Colour colour)
    {
        juce::AttributedString s;
        s.append (text, font, colour);
        s.setLineSpacing (3.0f);
        return s;
    }

    static int textHeight (const juce::String& text, const juce::Font& font, int width)
    {
        juce::TextLayout layout;
        layout.createLayout (attributed (text, font, juce::Colours::white), (float) width);
        return (int) std::ceil (layout.getHeight());
    }

    static void drawText (juce::Graphics& g, const juce::String& text, const juce::Font& font, juce::Colour colour,
                          juce::Rectangle<int> area)
    {
        juce::TextLayout layout;
        layout.createLayout (attributed (text, font, colour), (float) area.getWidth());
        layout.draw (g, area.toFloat());
    }

    std::vector<Block> blocks;
};

//==============================================================================
void AboutDialog::show (juce::Component& parent, AudioEngine& engine)
{
    // Owned by its parent from here on; it deletes itself in close().
    auto* dialog = new AboutDialog (engine);
    parent.addAndMakeVisible (dialog);
    dialog->setBounds (parent.getLocalBounds());
    dialog->grabKeyboardFocus();
}

AboutDialog::AboutDialog (AudioEngine& e)
    : engine (e)
{
    setTitle ("About Lithium Studio");
    setWantsKeyboardFocus (true);
    setAlwaysOnTop (true);

    icon = juce::ImageFileFormat::loadFrom (BinaryData::io_github_thecalamityjoe87_LithiumStudio_png,
                                            (size_t) BinaryData::io_github_thecalamityjoe87_LithiumStudio_pngSize);

    aboutPage = std::make_unique<Page>();
    creditsPage = std::make_unique<Page>();
    releaseNotesPage = std::make_unique<Page>();
    fillAbout (*aboutPage);
    fillCredits (*creditsPage);
    fillReleaseNotes (*releaseNotesPage);

    pageView.setScrollBarsShown (true, false);
    pageView.setScrollBarThickness (size::scrollbarWidth);
    addAndMakeVisible (pageView);

    // The GPL's full text, in place of the page.
    licenseText.setMultiLine (true);
    licenseText.setReadOnly (true);
    licenseText.setCaretVisible (false);
    licenseText.setScrollbarsShown (true);
    licenseText.setBorder ({ 12, 14, 12, 14 });
    licenseText.setTitle ("License");
    applyLicenseColours();
    addChildComponent (licenseText);

    int index = 0;

    for (auto* b : { &aboutTab, &creditsTab, &releaseNotesTab })
    {
        b->setColours (tabColours);
        b->setFont (captionFont (textSize::md + 1.0f));
        b->setRadioGroupId (1);   // radio group: turning one on turns the others off
        b->onClick = [this, index] { showTab ((Tab) index); };
        addAndMakeVisible (b);
        ++index;
    }

    for (auto* b : { &githubButton, &licenseButton, &closeButton })
    {
        b->setColours (IconButton::borderedColours);
        b->setFont (captionFont (textSize::md, 0.12f));
        addAndMakeVisible (b);
    }

    githubButton.onClick = [] { juce::URL (projectUrl).launchInDefaultBrowser(); };
    githubButton.setTooltip (projectUrl);
    licenseButton.onClick = [this] { showTab (Tab::license); };
    closeButton.onClick = [this] { close(); };

    showTab (Tab::about);
}

AboutDialog::~AboutDialog() = default;

void AboutDialog::applyLicenseColours()
{
    licenseText.setColour (juce::TextEditor::focusedOutlineColourId, theme().borderSoft);
    licenseText.setColour (juce::TextEditor::textColourId, theme().textSecondary);
}

void AboutDialog::lookAndFeelChanged()
{
    // JUCE calls lookAndFeelChanged() on every component when the theme
    // changes. A TextEditor keeps the colour its text was added with, so set
    // the colours again and refill it. The pages paint with the theme's
    // colours, except their links.
    applyLicenseColours();

    for (auto* p : { aboutPage.get(), creditsPage.get(), releaseNotesPage.get() })
        if (p != nullptr)
            p->applyLinkColours();

    if (tab == Tab::license)
        showTab (Tab::license);
}

void AboutDialog::showTab (Tab newTab)
{
    tab = newTab;

    aboutTab.setToggleState (tab == Tab::about || tab == Tab::license, juce::dontSendNotification);
    creditsTab.setToggleState (tab == Tab::credits, juce::dontSendNotification);
    releaseNotesTab.setToggleState (tab == Tab::releaseNotes, juce::dontSendNotification);

    licenseText.setVisible (tab == Tab::license);
    pageView.setVisible (tab != Tab::license);

    if (tab == Tab::license)
    {
        // Monospaced, as the text is laid out for: its indents and centred headings.
        licenseText.clear();
        licenseText.setFont (fonts::mono (textSize::sm));
        licenseText.setColour (juce::TextEditor::textColourId, theme().textSecondary);
        licenseText.insertTextAtCaret (juce::String::fromUTF8 (BinaryData::LICENSE, BinaryData::LICENSESize));
        licenseText.setCaretPosition (0);   // show the top
    }
    else
    {
        auto* page = tab == Tab::credits ? creditsPage.get() : tab == Tab::releaseNotes ? releaseNotesPage.get() : aboutPage.get();
        pageView.setViewedComponent (page, false);   // false: we own the pages
        pageView.setViewPosition (0, 0);
        resized();
    }

    repaint();
}

//==============================================================================
void AboutDialog::fillAbout (Page& p)
{
    p.addParagraph ("A digital audio workstation for Linux, Windows, and macOS. Record instruments and vocals, "
                    "play virtual instruments, arrange tracks, add effects, mix, and export your music. "
                    "Everything you need to make a song, without the unnecessary complexity.");
    p.addParagraph ("This is an alpha release: expect rough edges, and keep a copy of any session you care about.",
                    textSize::md, Page::Tone::secondary);

    // The GPL's notices: copyright, the license, no warranty. The License
    // button shows the full text.
    p.addHeading ("License");
    p.addParagraph (juce::String::fromUTF8 ("\xc2\xa9 2026 Isaac Joseph. Lithium Studio is free software: you can redistribute "
                                            "and change it under the terms of the GNU General Public License, version 3 or "
                                            "later. It comes with no warranty."),
                    textSize::md, Page::Tone::secondary);

    p.addHeading ("Built with");
    p.addTechnology();
}

void AboutDialog::fillCredits (Page& p)
{
    p.addParagraph ("These sounds and grooves are their authors' work, released under open licenses. Credit them as "
                    "shown when you publish music made with them.");

    p.addHeading ("Sound packs");

    for (auto& pack : engine.getSounds().getPacks())
    {
        Page::Credit c;
        c.title = pack.name;
        c.license = pack.license;
        c.description = pack.description;

        // Who made it and what they made, from the pack's credits.
        juce::StringArray who, what;

        for (auto& credit : pack.credits)
        {
            if (credit.who.isNotEmpty())   who.addIfNotAlreadyThere (credit.who);
            if (credit.what.isNotEmpty() && ! credit.what.equalsIgnoreCase (pack.name))
                what.addIfNotAlreadyThere (credit.what);
            if (c.license.isEmpty())       c.license = credit.license;
            if (c.sourceUrl.isEmpty())     c.sourceUrl = credit.url;
        }

        if (who.isEmpty() && pack.author.isNotEmpty())
            who.add (pack.author);

        // The packs that come with the app were made for it (their credit
        // says how, not who).
        if (pack.bundled)
        {
            who = { "Isaac Joseph" };
            what.clear();
        }

        c.by = who.isEmpty() ? juce::String() : "by " + who.joinIntoString (", ");
        c.detail = what.joinIntoString (juce::String::fromUTF8 (" \xc2\xb7 "));

        if (pack.bundled)
            c.detail = c.detail.isEmpty() ? juce::String ("Comes with the app") : c.detail + juce::String::fromUTF8 (" \xc2\xb7 Comes with the app");

        p.addCredit (c);
    }

    p.addHeading ("Drummer");
    p.addCredit ({ "Groove MIDI Dataset", "by Google LLC (Magenta)", {},
                   "Drumming by ten drummers, recorded by Google's Magenta team. The Drummer's grooves come from it.",
                   "CC-BY-4.0", "https://magenta.tensorflow.org/datasets/groove" });
    p.addCredit ({ "Red Zeppelin Kit", "by Glen MacArthur (AVL Drumkits)", {},
                   "The Drummer's drum kit, a separate download.",
                   "CC-BY-SA-3.0", "https://www.bandshed.net/avldrumkits/" });
}

void AboutDialog::fillReleaseNotes (Page& p)
{
    p.addHeading (juce::JUCEApplication::getInstance()->getApplicationVersion() + " (" LITHIUM_STAGE ")");
    p.addParagraph ("The first release of Lithium Studio. It's alpha software: ready to try, but expect rough edges, "
                    "and keep a copy of any session you care about.");

    p.addHeading ("What's new");
    p.addBullets ({
        "Sessions saved as one .lisession file that holds every recording and loop it uses",
        "Instrument tracks with a built-in synth and sampled instruments, played from the on-screen keyboard or a MIDI keyboard",
        "Audio tracks: record from an input, and keep several takes of the same part",
        "Drummer tracks that play along in the style you pick",
        "Regions on a timeline, a piano roll editor, loops and automation",
        "Sound knobs for every instrument, a per-track EQ, effects and a mixer with sends",
        "A pedalboard and amp for guitar and bass, and pitch correction for vocals",
        "Plug-ins: VST3 everywhere, LV2 on Linux and Windows, Audio Units on macOS",
        "Export your song as audio, including MP3",
        "Autosave, and recovery of your work after a crash",
        "Lithium Basics, a sound pack that comes with the app, and more packs to download from Manage Sounds",
    });
}

//==============================================================================
void AboutDialog::close()
{
    // Remove ourselves on the next message-loop turn: we may be inside our
    // own button's callback right now.
    setVisible (false);
    juce::Component::unfocusAllComponents();

    juce::MessageManager::callAsync ([safeThis = SafePointer<AboutDialog> (this)]
    {
        if (safeThis != nullptr)
            delete safeThis.getComponent();
    });
}

bool AboutDialog::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey || key == juce::KeyPress::returnKey)
    {
        close();
        return true;
    }

    return false;
}

void AboutDialog::mouseDown (const juce::MouseEvent& e)
{
    if (! panel.contains (e.getPosition()))
        close();
}

void AboutDialog::paint (juce::Graphics& g)
{
    g.fillAll (theme().scrim);

    theme().dialogShadow.drawForRectangle (g, panel);
    g.setColour (theme().dialogBg);
    g.fillRoundedRectangle (panel.toFloat(), size::radiusPill + 4.0f);
    g.setColour (theme().border);
    g.drawRoundedRectangle (panel.toFloat().reduced (0.5f), size::radiusPill + 4.0f, 1.0f);

    // The brand block: the app icon (it has its own rounded square), then the
    // wordmark, version and byline.
    auto r = brandArea;

    // drawImage draws at the current colour's opacity.
    g.setOpacity (1.0f);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (icon, r.removeFromLeft (iconSize).withSizeKeepingCentre (iconSize, iconSize).toFloat(),
                 juce::RectanglePlacement::centred);

    r.removeFromLeft (22);
    auto lines = r.withSizeKeepingCentre (r.getWidth(), 70);
    branding::paintWordmark (g, lines.removeFromTop (34).toFloat(), 30.0f, juce::Justification::centredLeft);

    g.setColour (theme().textSecondary);
    g.setFont (fonts::mono (textSize::md));
    g.drawText (versionText(), lines.removeFromTop (18), juce::Justification::centredLeft, false);

    juce::AttributedString byline;
    byline.append ("by Isaac Joseph", fonts::ui (textSize::lg), theme().text);
    byline.append (juce::String::fromUTF8 (" \xc2\xb7 Calamity Audio"), fonts::ui (textSize::lg), theme().textSecondary);
    byline.setJustification (juce::Justification::centredLeft);
    byline.draw (g, lines.toFloat());

    // The tabs' rule, and the chosen tab's underline.
    g.setColour (theme().border);
    g.fillRect (tabsArea.withTop (tabsArea.getBottom() - 1));

    for (auto* b : { &aboutTab, &creditsTab, &releaseNotesTab })
        if (b->getToggleState())
        {
            g.setColour (theme().accent);
            g.fillRect (b->getBounds().reduced (6, 0).withTop (tabsArea.getBottom() - 2).withHeight (2));
        }
}

void AboutDialog::resized()
{
    panel = getLocalBounds().withSizeKeepingCentre (juce::jmin (panelWidth, getWidth() - 32),
                                                    juce::jmin (panelHeight, getHeight() - 32));
    auto r = panel.reduced (30, 28);

    // Brand block, with GitHub and License on the right.
    brandArea = r.removeFromTop (brandHeight);
    auto buttons = brandArea.withSizeKeepingCentre (brandArea.getWidth(), topButtonHeight);
    licenseButton.setBounds (buttons.removeFromRight (topButtonWidth));
    buttons.removeFromRight (10);
    githubButton.setBounds (buttons.removeFromRight (topButtonWidth));
    brandArea.setRight (githubButton.getX() - 12);

    // Tabs: each as wide as its label, 26px apart.
    r.removeFromTop (22);
    tabsArea = r.removeFromTop (tabsHeight);
    auto t = tabsArea;
    const auto tabFont = captionFont (textSize::md + 1.0f);

    for (auto [b, label] : { std::pair { &aboutTab, "ABOUT" }, std::pair { &creditsTab, "CREDITS" },
                             std::pair { &releaseNotesTab, "RELEASE NOTES" } })
    {
        // IconButton insets its label 8px each side: the button is that much
        // wider than the word, and starts 8px left of the column.
        if (b == &aboutTab)
            t.removeFromLeft (-8);

        b->setBounds (t.removeFromLeft (juce::roundToInt (widthOf (tabFont, label)) + 20).withTrimmedBottom (2));
        t.removeFromLeft (10);
    }

    // Close, bottom right.
    closeButton.setBounds (r.removeFromBottom (34).removeFromRight (110));
    r.removeFromBottom (14);
    r.removeFromTop (14);

    contentArea = r;
    licenseText.setBounds (contentArea);
    pageView.setBounds (contentArea);

    // Room on the right for the scrollbar, so it doesn't cover the text.
    const auto pageWidth = contentArea.getWidth() - pageView.getScrollBarThickness() - 8;

    for (auto* p : { aboutPage.get(), creditsPage.get(), releaseNotesPage.get() })
        if (p != nullptr)
            p->layout (pageWidth);
}

} // namespace lithium
