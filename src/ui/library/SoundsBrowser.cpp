#include "ui/library/SoundsBrowser.h"
#include "engine/PackStore.h"
#include "ui/dialogs/AlertDialog.h"
#include "engine/SoundLibrary.h"
#include "ui/theme/Fonts.h"
#include "ui/theme/Theme.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/Icons.h"
#include "ui/widgets/Licenses.h"

namespace lithium
{

using namespace tokens;

namespace
{
    constexpr int categoryWidth = 200, detailsWidth = 280, columnGap = 18;
    constexpr int headerHeight = 36, footerHeight = 38;
    constexpr int cardHeight = 108, cardGap = 10, minCardWidth = 168, sectionHeight = 30, sectionGap = 12;

    // The groups packs are filed under: the library's categories, in the
    // sidebar's order, with their short labels. Packs with any other
    // category go under Other.
    struct PackGroup { const char* category; const char* label; };

    const PackGroup packGroups[] {
        { "Keyboards", "Keys" },
        { "Drums",     "Drums" },
        { "Bass",      "Bass" },
        { "Synths",    "Synths" },
        { "Guitar",    "Guitar" },
        { "Strings",   "Strings" },
        { "Woodwinds", "Woodwinds" },
        { "Brass",     "Brass" },
        { "Loops",     "Loops" },
        { "Other",     "Other" },
    };

    juce::String groupOf (const StorePack& p)
    {
        for (auto& g : packGroups)
            if (p.category == g.category)
                return g.category;

        return "Other";
    }

    juce::String labelOf (const juce::String& group)
    {
        for (auto& g : packGroups)
            if (group == g.category)
                return g.label;

        return group;
    }

    juce::String sizeText (juce::int64 bytes)
    {
        return bytes > 0 ? juce::File::descriptionOfSizeInBytes (bytes) : juce::String ("-");
    }

    juce::String dot()            { return juce::String::fromUTF8 (" \xc2\xb7 "); }   // " · "

    bool isInstalled (const StorePack& p)   { return p.installedVersion.isNotEmpty(); }
    bool isBusy (const StorePack& p)        { return p.state == StorePack::State::downloading || p.state == StorePack::State::installing; }

    // The few letters on a card's badge.
    juce::String licenseBadge (const juce::String& spdx)
    {
        if (spdx.startsWith ("CC0"))         return "CC0";
        if (spdx.startsWith ("CC-BY-NC"))    return "BY-NC";
        if (spdx.startsWith ("CC-BY-SA"))    return "BY-SA";
        if (spdx.startsWith ("CC-BY-ND"))    return "BY-ND";
        if (spdx.startsWith ("CC-BY"))       return "CC BY";
        if (spdx.containsIgnoreCase ("GPL")) return "GPL";
        return spdx.upToFirstOccurrenceOf ("-", false, false).toUpperCase().substring (0, 8);
    }

    juce::Colour badgeColour (const StorePack& p)
    {
        using F = Family;

        if (p.bundled)                       return theme().textSecondary;
        if (p.license.startsWith ("CC0"))    return theme().ink (F::bass);     // green: free for anything
        if (p.license.startsWith ("CC-BY-SA") || p.license.startsWith ("CC-BY-NC") || p.license.startsWith ("CC-BY-ND"))
            return theme().ink (F::guitar);                                    // yellow: conditions beyond credit
        if (p.license.startsWith ("CC-BY"))  return theme().ink (F::keys);     // blue: credit the author
        return theme().textSecondary;
    }

    // Uppercase caption text with a little letter-spacing, like Lithium Drums' labels.
    juce::Font captionFont (float size, float kerning = 0.1f)
    {
        return fonts::caption (size).withExtraKerningFactor (kerning);
    }

    float widthOf (const juce::Font& font, const juce::String& text)
    {
        return juce::GlyphArrangement::getStringWidth (font, text);
    }

    // A small outlined tag drawn right to left from `right`. Returns where the next one should end.
    float drawBadge (juce::Graphics& g, const juce::String& text, float right, float centreY, juce::Colour colour)
    {
        const auto font = captionFont (textSize::xs, 0.08f);
        const juce::Rectangle<float> r (right - widthOf (font, text) - 12.0f, centreY - 8.0f, widthOf (font, text) + 12.0f, 16.0f);

        g.setColour (colour.withAlpha (0.12f));
        g.fillRoundedRectangle (r, size::radiusTiny);
        g.setColour (colour.withAlpha (0.7f));
        g.drawRoundedRectangle (r.reduced (0.5f), size::radiusTiny, 1.0f);
        g.setColour (colour);
        g.setFont (font);
        g.drawText (text, r, juce::Justification::centred, false);
        return r.getX() - 5.0f;
    }

    void drawProgressBar (juce::Graphics& g, juce::Rectangle<float> r, float progress)
    {
        g.setColour (theme().controlFill);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (theme().accent);
        g.fillRoundedRectangle (r.withWidth (r.getWidth() * juce::jlimit (0.0f, 1.0f, progress)), r.getHeight() * 0.5f);
    }

    float progressOf (const StorePack& p)
    {
        return p.state == StorePack::State::installing ? 1.0f : p.progress;
    }

    juce::String progressText (const StorePack& p)
    {
        return p.state == StorePack::State::installing ? juce::String ("Installing...")
                                                       : "Downloading " + juce::String (juce::roundToInt (p.progress * 100.0f)) + "%";
    }

    // What the outlined segment buttons (All, Installed, Available) look like:
    // bordered when off, the accent when on.
    IconButton::Colours segmentButtonColours()
    {
        auto c = IconButton::borderedColours();
        c.background = theme().cardBg;
        c.foreground = theme().textSecondary;
        c.backgroundOn = theme().accent;
        c.foregroundOn = theme().textOnAccent;
        c.borderOn = theme().accent;
        return c;
    }
}

//==============================================================================
/** The categories down the left, with "All sounds" on top. */
class SoundsBrowser::CategoryList  : public juce::Component
{
public:
    struct Row
    {
        juce::String group;   // empty for All sounds
        juce::String label;
        int count = 0;
    };

    std::function<void (const juce::String&)> onSelect;

    CategoryList()
    {
        setTitle ("Categories");
    }

    void setRows (std::vector<Row> newRows, const juce::String& selectedGroup)
    {
        rows = std::move (newRows);
        selected = selectedGroup;
        repaint();
    }

    int getPreferredHeight() const      { return top + (int) rows.size() * rowHeight; }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme().textSecondary);
        g.setFont (captionFont (textSize::sm, 0.14f));
        g.drawText ("CATEGORIES", juce::Rectangle<int> (12, 0, getWidth() - 12, 20), juce::Justification::centredLeft, false);

        for (size_t i = 0; i < rows.size(); ++i)
        {
            const auto& row = rows[i];
            const auto r = rowBounds ((int) i).toFloat();
            const bool isSelected = row.group == selected;

            if (isSelected || (int) i == hoverRow)
            {
                g.setColour (isSelected ? theme().selectedRow : theme().rowHover);
                g.fillRoundedRectangle (r, size::radius);
            }

            if (isSelected)
            {
                g.setColour (theme().accent);
                g.fillRoundedRectangle (r.withWidth (3.0f).reduced (0.0f, 8.0f), 1.5f);
            }

            auto text = r.reduced (12.0f, 0.0f);
            g.setColour (theme().textSecondary);
            g.setFont (fonts::mono (textSize::sm));
            g.drawText (juce::String (row.count), text.removeFromRight (36.0f), juce::Justification::centredRight, false);

            g.setColour (isSelected ? theme().text : (row.count > 0 ? theme().text.withAlpha (0.85f) : theme().textSecondary));
            g.setFont (isSelected ? fonts::uiSemiBold (textSize::lg) : fonts::ui (textSize::lg));
            g.drawText (row.label, text, juce::Justification::centredLeft, true);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override    { setHover (rowAt (e.y)); }
    void mouseExit (const juce::MouseEvent&) override      { setHover (-1); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (const auto i = rowAt (e.y); i >= 0 && onSelect)
            onSelect (rows[(size_t) i].group);
    }

private:
    static constexpr int top = 28, rowHeight = 36;

    juce::Rectangle<int> rowBounds (int i) const    { return { 0, top + i * rowHeight, getWidth(), rowHeight - 2 }; }
    int rowAt (int y) const                         { const auto i = (y - top) / rowHeight; return y >= top && i < (int) rows.size() ? i : -1; }

    void setHover (int i)
    {
        if (i != hoverRow)
        {
            hoverRow = i;
            repaint();
        }
    }

    std::vector<Row> rows;
    juce::String selected;
    int hoverRow = -1;
};

//==============================================================================
/** The starter sounds, under the categories, while some are left to download. */
class SoundsBrowser::StarterCard  : public juce::Component
{
public:
    std::function<void()> onDownload;

    StarterCard()
    {
        button.setColours (IconButton::borderedColours);
        button.setFont (captionFont (textSize::md));
        button.onClick = [this] { if (onDownload) onDownload(); };
        addAndMakeVisible (button);
    }

    void setCollection (const StoreCollection& c, const std::vector<StorePack>& packs)
    {
        title = c.name;
        const auto total = (int) c.packIds.size();
        downloading = c.packsDownloading > 0;

        if (downloading)
        {
            // Installed packs count as done, downloading ones as far as they've got.
            float done = 0.0f;

            for (auto& id : c.packIds)
                for (auto& p : packs)
                    if (p.id == id)
                        done += p.state == StorePack::State::installed || p.state == StorePack::State::installing ? 1.0f
                              : p.state == StorePack::State::downloading ? p.progress : 0.0f;

            progress = total > 0 ? done / (float) total : 0.0f;
            text = "Downloading " + juce::String (total - c.packsToInstall - c.packsDownloading + 1) + " of " + juce::String (total) + ".";
        }
        else
        {
            text = c.description.isNotEmpty() ? c.description + " " : juce::String();
            text << c.packsToInstall << " of " << total << " left to download, " << sizeText (c.downloadSize) << ".";
        }

        button.setVisible (! downloading);
        button.setText ("DOWNLOAD ALL");
        button.setTitle ("Download all of " + title);
        setTitle (title);
        resized();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (theme().cardBg);
        g.fillRoundedRectangle (bounds, size::radiusLarge);
        g.setColour (theme().border);
        g.drawRoundedRectangle (bounds, size::radiusLarge, 1.0f);

        auto r = getLocalBounds().reduced (12);
        g.setColour (theme().text);
        g.setFont (fonts::uiSemiBold (textSize::lg));
        g.drawText (title, r.removeFromTop (20), juce::Justification::centredLeft, true);

        g.setColour (theme().textSecondary);
        g.setFont (fonts::ui (textSize::sm));
        g.drawFittedText (text, r.removeFromTop (48), juce::Justification::topLeft, 3, 1.0f);

        if (downloading)
            drawProgressBar (g, r.removeFromTop (4).toFloat(), progress);
    }

    void resized() override
    {
        button.setBounds (getLocalBounds().reduced (12).removeFromBottom (30));
    }

    static constexpr int preferredHeight = 124;

private:
    IconButton button { "Download all", std::nullopt, "DOWNLOAD ALL" };
    juce::String title, text;
    bool downloading = false;
    float progress = 0.0f;
};

//==============================================================================
/** The cards' container, which also draws a heading above each category's cards. */
class SoundsBrowser::Grid  : public juce::Component
{
public:
    struct Heading
    {
        juce::Rectangle<int> area;
        juce::String text, count;
    };

    std::vector<Heading> headings;
    juce::String emptyText;

    void paint (juce::Graphics& g) override
    {
        for (const auto& h : headings)
        {
            auto r = h.area.toFloat();
            const auto font = captionFont (textSize::md, 0.12f);
            const auto title = h.text.toUpperCase();
            g.setColour (theme().textSecondary);
            g.setFont (font);
            g.drawText (title, r, juce::Justification::centredLeft, false);

            const auto w = widthOf (font, title);
            g.setColour (theme().textSecondary.withAlpha (0.75f));
            g.setFont (fonts::ui (textSize::sm));
            g.drawText (h.count, r.withTrimmedLeft (w + 10.0f), juce::Justification::centredLeft, false);

            const auto lineX = w + 20.0f + widthOf (fonts::ui (textSize::sm), h.count);
            g.setColour (theme().border);
            g.fillRect (r.withTrimmedLeft (lineX).withHeight (1.0f).withY (r.getCentreY()));
        }

        if (emptyText.isNotEmpty())
        {
            g.setColour (theme().textSecondary);
            g.setFont (fonts::ui (textSize::lg));
            g.drawFittedText (emptyText, getLocalBounds().withHeight (60), juce::Justification::centred, 2, 1.0f);
        }
    }
};

//==============================================================================
/** One pack. */
class SoundsBrowser::PackCard  : public juce::Component
{
public:
    std::function<void (const juce::String&)> onClick;

    explicit PackCard (const juce::String& packId) : id (packId)
    {
        setRepaintsOnMouseActivity (true);
        setWantsKeyboardFocus (false);
    }

    const juce::String& getId() const noexcept    { return id; }

    void setPack (const StorePack& p, bool isSelected)
    {
        pack = p;
        selected = isSelected;
        setTitle (p.name);
        setDescription (p.description);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        const bool installed = isInstalled (pack);
        const auto edge = selected ? theme().accent : (isMouseOver() ? theme().borderStrong : theme().border);

        if (installed)
        {
            g.setColour (selected ? theme().selectedRow : theme().cardBg);
            g.fillRoundedRectangle (bounds, size::radiusLarge);

            if (isMouseOver() && ! selected)
            {
                g.setColour (theme().rowHover);
                g.fillRoundedRectangle (bounds, size::radiusLarge);
            }

            g.setColour (edge);
            g.drawRoundedRectangle (bounds, size::radiusLarge, selected ? 1.5f : 1.0f);
        }
        else
        {
            // Not on this computer: a dashed outline, like a placeholder.
            if (selected)
            {
                g.setColour (theme().cardBg);
                g.fillRoundedRectangle (bounds, size::radiusLarge);
            }

            juce::Path outline;
            outline.addRoundedRectangle (bounds, size::radiusLarge);
            const float dashes[] { 4.0f, 3.0f };
            juce::PathStrokeType (selected ? 1.5f : 1.0f).createDashedStroke (outline, outline, dashes, 2);
            g.setColour (edge);
            g.fillPath (outline);
        }

        auto area = getLocalBounds().toFloat().reduced (14.0f, 12.0f);

        // The license badge, top right.
        auto top = area.removeFromTop (18.0f);
        const auto badge = pack.bundled ? juce::String ("BUILT-IN") : licenseBadge (pack.license);
        auto right = top.getRight();

        if (badge.isNotEmpty())
            right = drawBadge (g, badge, right, top.getCentreY(), badgeColour (pack));

        // The name may run up under the badge's row (it's on the next line).
        area.removeFromTop (2.0f);
        g.setColour (installed ? theme().text : theme().text.withAlpha (0.6f));
        g.setFont (fonts::uiSemiBold (textSize::xxl));
        g.drawText (pack.name, area.removeFromTop (22.0f), juce::Justification::centredLeft, true);

        g.setColour (theme().textSecondary);
        g.setFont (fonts::ui (textSize::smPlus));
        g.drawText (pack.author.isNotEmpty() ? labelOf (groupOf (pack)) + dot() + pack.author : labelOf (groupOf (pack)),
                    area.removeFromTop (18.0f), juce::Justification::centredLeft, true);

        // The bottom row: its state on the left, its size on the right.
        auto bottom = area.removeFromBottom (16.0f);

        if (isBusy (pack))
        {
            g.setColour (theme().accent);
            g.setFont (fonts::mono (textSize::xs));
            g.drawText (juce::String (juce::roundToInt (progressOf (pack) * 100.0f)) + "%", bottom.removeFromRight (36.0f),
                        juce::Justification::centredRight, false);
            drawProgressBar (g, bottom.withSizeKeepingCentre (bottom.getWidth() - 6.0f, 4.0f).withX (bottom.getX()), progressOf (pack));
            return;
        }

        g.setColour (theme().text.withAlpha (0.75f));
        g.setFont (fonts::mono (textSize::sm));
        g.drawText (sizeText (installed ? pack.installedSize : pack.downloadSize), bottom, juce::Justification::centredRight, false);
        bottom.removeFromRight (70.0f);

        juce::String state;
        juce::Colour colour;

        if (pack.error.isNotEmpty())                                  { state = "Download failed";    colour = theme().record; }
        else if (pack.state == StorePack::State::updateAvailable)    { state = "Update available";   colour = theme().warning; }
        else if (pack.bundled)                                       { state = "Comes with the app"; colour = theme().textSecondary; }
        else if (installed)                                          { state = juce::String::fromUTF8 ("\xe2\x9c\x93 Installed"); colour = theme().accent; }
        else                                                         { state = juce::String::fromUTF8 ("\xe2\x86\x93 Download"); colour = theme().textSecondary; }

        g.setColour (colour);
        g.setFont (fonts::uiSemiBold (textSize::sm));
        g.drawText (state, bottom, juce::Justification::centredLeft, true);
    }

    void mouseDown (const juce::MouseEvent&) override    { if (onClick) onClick (id); }

private:
    const juce::String id;
    StorePack pack;
    bool selected = false;
};

//==============================================================================
/** The selected pack: what it is, its sizes, credits and license, and its actions. */
class SoundsBrowser::Details  : public juce::Component
{
public:
    std::function<void()> onMain, onRemove;

    Details()
    {
        removeButton.setColours ([] {
            auto c = IconButton::borderedColours();
            c.background = juce::Colours::transparentBlack;
            c.foreground = c.foregroundOn = theme().record;
            c.border = theme().record.withAlpha (0.4f);
            return c;
        });
        removeButton.setFont (captionFont (textSize::sm));
        removeButton.onClick = [this] { if (onRemove) onRemove(); };
        addChildComponent (removeButton);

        mainButton.setFont (captionFont (textSize::lg));
        mainButton.onClick = [this] { if (onMain) onMain(); };
        addChildComponent (mainButton);

        for (auto* link : { &licenseLink, &sourceLink })
        {
            link->setFont (fonts::ui (textSize::md), false, juce::Justification::centredLeft);
            addChildComponent (*link);
        }
    }

    void show (const StorePack* p, int soundCount, const SoundPackInfo* installedInfo)
    {
        has = p != nullptr;

        if (has)
            pack = *p;

        sounds = soundCount;
        credits.clear();
        juce::String sourceUrl;

        if (installedInfo != nullptr)
        {
            for (auto& c : installedInfo->credits)
            {
                juce::StringArray parts { c.what, c.who };
                parts.removeEmptyStrings();
                credits.add (parts.joinIntoString (dot()));

                if (sourceUrl.isEmpty())
                    sourceUrl = c.url;
            }
        }

        if (credits.isEmpty() && has && pack.author.isNotEmpty())
            credits.add ("By " + pack.author);

        const bool installed = has && isInstalled (pack);
        removeButton.setVisible (installed && ! pack.bundled && ! isBusy (pack));
        removeButton.setTitle ("Remove " + pack.name);

        // The main button: Download, Update or Cancel. Installed packs say so
        // instead (painted, not a button).
        juce::String action;

        if (has && pack.state == StorePack::State::downloading)        action = "CANCEL";
        else if (has && pack.state == StorePack::State::notInstalled)  action = "DOWNLOAD  " + sizeText (pack.downloadSize);
        else if (has && pack.state == StorePack::State::updateAvailable) action = "UPDATE  " + sizeText (pack.downloadSize);

        mainButton.setVisible (action.isNotEmpty());
        mainButton.setText (action);
        mainButton.setTitle (action.upToFirstOccurrenceOf (" ", false, false) + " " + pack.name);
        mainButton.setColours (action == "CANCEL" ? IconButton::ColourScheme (IconButton::borderedColours)
                                                  : IconButton::ColourScheme (IconButton::accentColours));

        licenseLink.setVisible (has && pack.license.isNotEmpty());
        licenseLink.setButtonText (licenses::displayName (pack.license));
        licenseLink.setURL (licenses::url (pack.license));
        licenseLink.setTooltip (licenses::url (pack.license).toString (false));
        licenseLink.setColour (juce::HyperlinkButton::textColourId, theme().accent);

        sourceLink.setVisible (has && sourceUrl.isNotEmpty());
        sourceLink.setButtonText ("Source");
        sourceLink.setURL (juce::URL (sourceUrl));
        sourceLink.setTooltip (sourceUrl);
        sourceLink.setColour (juce::HyperlinkButton::textColourId, theme().accent);

        resized();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (theme().cardBg);
        g.fillRoundedRectangle (bounds, size::radiusCard + 2.0f);
        g.setColour (theme().border);
        g.drawRoundedRectangle (bounds, size::radiusCard + 2.0f, 1.0f);

        if (! has)
        {
            g.setColour (theme().textSecondary);
            g.setFont (fonts::ui (textSize::lg));
            g.drawFittedText ("Choose a pack to see what's in it.", getLocalBounds().reduced (18), juce::Justification::centred, 2, 1.0f);
            return;
        }

        const bool installed = isInstalled (pack);
        auto area = getLocalBounds().reduced (18, 16);

        auto nameRow = area.removeFromTop (30);

        if (removeButton.isVisible())
            nameRow.removeFromRight (removeWidth + 8);

        g.setColour (theme().text);
        g.setFont (fonts::uiSemiBold (textSize::hero - 4.0f));
        g.drawFittedText (pack.name, nameRow, juce::Justification::centredLeft, 1, 0.8f);

        juce::StringArray sub { labelOf (groupOf (pack)) };

        // Registry packs give a commit id as their version: only show real version numbers.
        const auto version = pack.installedVersion.isNotEmpty() ? pack.installedVersion : pack.availableVersion;

        if (version.containsChar ('.'))
            sub.add ("version " + version);

        g.setColour (theme().textSecondary);
        g.setFont (fonts::ui (textSize::md));
        g.drawText (sub.joinIntoString (dot()), area.removeFromTop (20), juce::Justification::centredLeft, true);

        // Readouts: what it takes, its version, how many sounds.
        area.removeFromTop (14);
        auto readouts = area.removeFromTop (38);
        const std::pair<juce::String, juce::String> values[] {
            { installed ? "ON DISK" : "DOWNLOAD", sizeText (installed ? pack.installedSize : pack.downloadSize) },
            { "SOUNDS", installed && sounds > 0 ? juce::String (sounds) : juce::String ("-") },
            { "LICENSE", pack.bundled ? juce::String ("Built-in") : licenseBadge (pack.license) },
        };
        const int w = readouts.getWidth() / 3;

        for (auto& [label, value] : values)
        {
            auto cell = readouts.removeFromLeft (w);
            g.setColour (theme().textSecondary);
            g.setFont (captionFont (textSize::sm, 0.14f));
            g.drawText (label, cell.removeFromTop (16), juce::Justification::centredLeft, false);
            g.setColour (theme().text);
            g.setFont (fonts::mono (textSize::xl));
            g.drawText (value.isNotEmpty() ? value : juce::String ("-"), cell, juce::Justification::centredLeft, true);
        }

        // What it is, or why the last download failed.
        area.removeFromTop (14);
        const auto bottom = getLocalBounds().reduced (18, 16).getBottom() - buttonHeight - 12 - linkHeight - 10;
        auto text = area.withBottom (bottom);

        if (pack.error.isNotEmpty())
        {
            g.setColour (theme().record);
            g.setFont (fonts::ui (textSize::md));
            g.drawFittedText (pack.error, text.removeFromTop (36), juce::Justification::topLeft, 2, 1.0f);
            text.removeFromTop (8);
        }

        if (pack.description.isNotEmpty())
        {
            juce::AttributedString s;
            s.append (pack.description, fonts::ui (textSize::lg), theme().text.withAlpha (0.85f));
            s.setLineSpacing (3.0f);
            juce::TextLayout layout;
            layout.createLayout (s, (float) text.getWidth());
            const auto h = juce::jmin ((int) std::ceil (layout.getHeight()), juce::jmax (0, text.getHeight() - 60));
            layout.draw (g, text.removeFromTop (h).toFloat());
            text.removeFromTop (16);
        }

        if (! credits.isEmpty() && text.getHeight() > 40)
        {
            g.setColour (theme().textSecondary);
            g.setFont (captionFont (textSize::sm, 0.14f));
            g.drawText ("CREDITS", text.removeFromTop (18), juce::Justification::centredLeft, false);
            g.setColour (theme().textSecondary);
            g.setFont (fonts::ui (textSize::md));
            g.drawFittedText (credits.joinIntoString ("\n"), text, juce::Justification::topLeft,
                              juce::jmax (1, text.getHeight() / 17), 1.0f);
        }

        if (! dotArea.isEmpty())
        {
            g.setColour (theme().accent);
            g.setFont (fonts::ui (textSize::md));
            g.drawText (juce::String::fromUTF8 ("\xe2\x80\xa2"), dotArea, juce::Justification::centred, false);
        }

        // Installed: the state in place of the button.
        if (! mainButton.isVisible())
        {
            auto r = buttonArea().toFloat();

            if (isBusy (pack))
            {
                g.setColour (theme().textSecondary);
                g.setFont (fonts::ui (textSize::md));
                g.drawText (progressText (pack), r.removeFromTop (18.0f), juce::Justification::centredLeft, false);
                drawProgressBar (g, r.withSizeKeepingCentre (r.getWidth(), 6.0f), progressOf (pack));
                return;
            }

            const auto state = pack.bundled ? juce::String ("COMES WITH THE APP") : juce::String ("INSTALLED");
            g.setColour (theme().accent.withAlpha (0.16f));
            g.fillRoundedRectangle (r, size::radius);
            g.setColour (theme().accent.withAlpha (0.35f));
            g.drawRoundedRectangle (r.reduced (0.5f), size::radius, 1.0f);
            g.setColour (theme().accent);
            g.setFont (captionFont (textSize::lg));
            g.drawText (state, r, juce::Justification::centred, false);
        }
        else if (isBusy (pack))
        {
            // Downloading: the progress above the Cancel button.
            auto r = buttonArea().translated (0, -24).withHeight (18).toFloat();
            g.setColour (theme().textSecondary);
            g.setFont (fonts::ui (textSize::md));
            g.drawText (progressText (pack), r, juce::Justification::centredLeft, false);
            drawProgressBar (g, r.translated (0.0f, 14.0f).withHeight (4.0f), progressOf (pack));
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (18, 16);
        removeButton.setBounds (area.getRight() - removeWidth, area.getY() + 2, removeWidth, 26);
        mainButton.setBounds (buttonArea());

        auto links = buttonArea().translated (0, -(linkHeight + 12 + (isBusy (pack) && mainButton.isVisible() ? 28 : 0))).withHeight (linkHeight);
        const auto linkFont = fonts::ui (textSize::md);

        if (licenseLink.isVisible())
            licenseLink.setBounds (links.removeFromLeft (juce::roundToInt (widthOf (linkFont, licenseLink.getButtonText())) + 4));

        // A dot between the two links, as in Lithium Drums: "CC BY 4.0 • Source".
        dotArea = licenseLink.isVisible() && sourceLink.isVisible() ? links.removeFromLeft (16) : juce::Rectangle<int>();

        if (sourceLink.isVisible())
            sourceLink.setBounds (links.removeFromLeft (juce::roundToInt (widthOf (linkFont, "Source")) + 4));
    }

private:
    static constexpr int removeWidth = 86, buttonHeight = 40, linkHeight = 20;

    juce::Rectangle<int> buttonArea() const    { return getLocalBounds().reduced (18, 16).removeFromBottom (buttonHeight); }

    bool has = false;
    StorePack pack;
    int sounds = 0;
    juce::StringArray credits;
    IconButton removeButton { "Remove", std::nullopt, "REMOVE" };
    IconButton mainButton { "Download", std::nullopt, "DOWNLOAD" };
    juce::HyperlinkButton licenseLink, sourceLink;
    juce::Rectangle<int> dotArea;   // between the two links
};

//==============================================================================
SoundsBrowser::SoundsBrowser (AudioEngine& e, const juce::String& downloadPackId)
    : engine (e), pendingDownload (downloadPackId)
{
    setTitle ("Manage Sounds");
    setWantsKeyboardFocus (true);

    searchField.setTextToShowWhenEmpty ("Search packs, instruments, authors...", theme().textSecondary);
    searchField.setFont (fonts::ui (textSize::lg));
    searchField.setTitle ("Search sound packs");
    searchField.setIndents (34, 9);   // room for the magnifier (see paint)
    searchField.onTextChange = [this] { refresh(); gridView.setViewPosition (0, 0); };
    searchField.onEscapeKey = [this]
    {
        if (searchField.isEmpty())
        {
            if (onClose) onClose();
        }
        else
        {
            searchField.clear();
            refresh();
        }
    };
    addAndMakeVisible (searchField);

    for (auto [button, which] : { std::pair { &allButton, Show::all }, std::pair { &installedButton, Show::installed },
                                  std::pair { &availableButton, Show::available } })
    {
        button->setColours (segmentButtonColours);
        button->setFont (captionFont (textSize::md, 0.12f));
        button->setClickingTogglesState (false);
        button->onClick = [this, which = which] { setShow (which); };
        addAndMakeVisible (*button);
    }

    categoryList = std::make_unique<CategoryList>();
    categoryList->onSelect = [this] (const juce::String& g) { setCategory (g); };
    addAndMakeVisible (*categoryList);

    starterCard = std::make_unique<StarterCard>();
    starterCard->onDownload = [this] { engine.getPackStore().installCollection (PackStore::starterCollectionId); };
    addChildComponent (*starterCard);

    grid = std::make_unique<Grid>();
    gridView.setViewedComponent (grid.get(), false);   // false: we own the grid
    gridView.setScrollBarsShown (true, false);
    gridView.setScrollBarThickness (size::scrollbarWidth);
    addAndMakeVisible (gridView);

    details = std::make_unique<Details>();
    details->onMain = [this] { runMainAction (selectedId); };
    details->onRemove = [this]
    {
        if (auto p = engine.getPackStore().findPack (selectedId))
            confirmRemove (p->id, p->name);
    };
    addAndMakeVisible (*details);

    retryButton.setColours (IconButton::borderedColours);
    retryButton.onClick = [this] { engine.getPackStore().refreshIndex(); };
    addChildComponent (retryButton);

    closeButton.setColours (IconButton::borderedColours);
    closeButton.setFont (captionFont (textSize::lg, 0.12f));
    closeButton.setText ("CLOSE");
    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeButton);

    selectedId = pendingDownload;

    auto& store = engine.getPackStore();
    store.addChangeListener (this);

    // Fetch the list each time, so it's never out of date.
    if (store.getIndexStatus() != PackStore::IndexStatus::loading)
        store.refreshIndex();

    refresh();
}

SoundsBrowser::~SoundsBrowser()
{
    engine.getPackStore().removeChangeListener (this);
}

void SoundsBrowser::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();
}

bool SoundsBrowser::isListed (const StorePack& p) const
{
    if (show == Show::installed && ! isInstalled (p))   return false;
    if (show == Show::available && isInstalled (p))     return false;

    const auto query = searchField.getText().trim();

    return query.isEmpty()
        || p.name.containsIgnoreCase (query) || p.author.containsIgnoreCase (query)
        || p.description.containsIgnoreCase (query) || p.category.containsIgnoreCase (query)
        || labelOf (groupOf (p)).containsIgnoreCase (query);
}

void SoundsBrowser::setShow (Show s)
{
    show = s;
    refresh();
    gridView.setViewPosition (0, 0);
}

void SoundsBrowser::setCategory (const juce::String& group)
{
    category = group;
    refresh();
    gridView.setViewPosition (0, 0);
}

void SoundsBrowser::refresh()
{
    auto& store = engine.getPackStore();
    const auto status = store.getIndexStatus();
    packs = store.getPacks();

    // The Sound tab's Download button: start it as soon as the index lists it.
    if (pendingDownload.isNotEmpty() && status == PackStore::IndexStatus::loaded)
    {
        if (auto p = store.findPack (pendingDownload); p.has_value() && p->availableVersion.isNotEmpty())
        {
            store.install (pendingDownload);
            selectedId = pendingDownload;
        }
        else
        {
            requestedPackUnavailable = true;
        }

        pendingDownload.clear();
        packs = store.getPacks();
    }

    const bool offline = status == PackStore::IndexStatus::unavailable;
    const bool nothingToDownload = status == PackStore::IndexStatus::loaded
        && std::none_of (packs.begin(), packs.end(), [] (const StorePack& p) { return p.availableVersion.isNotEmpty(); });

    notice = status == PackStore::IndexStatus::loading ? "Looking for sound packs..."
           : offline ? "Couldn't reach the sound library. Check your internet connection and try again. "
                       "Your installed packs are below."
           : requestedPackUnavailable ? "The sound pack that track needs isn't available to download. "
                                        "Ask whoever shared the session with you for it."
           : nothingToDownload ? "There are no sound packs to download yet. Your installed packs are below."
           : juce::String();
    retryButton.setVisible (offline);

    // The segment buttons.
    allButton.setToggleState (show == Show::all, juce::dontSendNotification);
    installedButton.setToggleState (show == Show::installed, juce::dontSendNotification);
    availableButton.setToggleState (show == Show::available, juce::dontSendNotification);

    // Categories: every group that has packs, counted under the current
    // All/Installed/Available choice. A chosen group with no packs left (its
    // last pack was removed) goes back to All sounds.
    auto inGroup = [] (const StorePack& p, const juce::String& group) { return group.isEmpty() || groupOf (p) == group; };
    auto countIn = [&] (const juce::String& group)
    {
        return (int) std::count_if (packs.begin(), packs.end(), [&] (const StorePack& p) { return inGroup (p, group) && isListed (p); });
    };

    if (category.isNotEmpty() && std::none_of (packs.begin(), packs.end(), [&] (const StorePack& p) { return groupOf (p) == category; }))
        category.clear();

    std::vector<CategoryList::Row> rows { { {}, "All sounds", countIn ({}) } };

    for (auto& g : packGroups)
        if (std::any_of (packs.begin(), packs.end(), [&] (const StorePack& p) { return groupOf (p) == g.category; }))
            rows.push_back ({ g.category, g.label, countIn (g.category) });

    categoryList->setRows (std::move (rows), category);

    // The starter sounds, while some are left to get.
    if (auto starter = store.findCollection (PackStore::starterCollectionId);
        starter.has_value() && (starter->packsToInstall > 0 || starter->packsDownloading > 0))
    {
        starterCard->setCollection (*starter, packs);
        starterCard->setVisible (true);
    }
    else
    {
        starterCard->setVisible (false);
    }

    // The cards, in category order. They're kept (not rebuilt) while the same
    // packs are listed, so a card being clicked isn't deleted under the mouse.
    std::vector<const StorePack*> listed;

    for (auto& g : packGroups)
        for (auto& p : packs)
            if (groupOf (p) == g.category && inGroup (p, category) && isListed (p))
                listed.push_back (&p);

    bool same = listed.size() == cards.size();

    for (size_t i = 0; same && i < listed.size(); ++i)
        same = cards[i]->getId() == listed[i]->id;

    if (! same)
    {
        cards.clear();

        for (auto* p : listed)
        {
            auto& card = cards.emplace_back (std::make_unique<PackCard> (p->id));
            card->onClick = [this] (const juce::String& id) { select (id, false); };
            grid->addAndMakeVisible (*card);
        }
    }

    // Keep the selection on a listed pack.
    if (std::none_of (listed.begin(), listed.end(), [this] (const StorePack* p) { return p->id == selectedId; }))
        selectedId = listed.empty() ? juce::String() : listed.front()->id;

    for (size_t i = 0; i < listed.size(); ++i)
        cards[i]->setPack (*listed[i], listed[i]->id == selectedId);

    grid->emptyText = ! listed.empty() || status == PackStore::IndexStatus::loading ? juce::String()
                    : searchField.getText().trim().isNotEmpty() ? "No packs match \"" + searchField.getText().trim() + "\"."
                    : show == Show::installed ? juce::String ("No packs installed here yet.")
                    : show == Show::available ? juce::String ("Every pack is installed.")
                                              : juce::String ("No sound packs yet.");

    // Space used by what's installed.
    juce::int64 installedBytes = 0;

    for (auto& p : packs)
        if (isInstalled (p))
            installedBytes += p.installedSize;

    storageText = installedBytes > 0 ? sizeText (installedBytes) : juce::String();

    select (selectedId, false);
    resized();
    repaint();
}

void SoundsBrowser::select (const juce::String& packId, bool scrollIntoView)
{
    selectedId = packId;
    const StorePack* pack = nullptr;

    for (auto& p : packs)
        if (p.id == packId)
            pack = &p;

    for (auto& card : cards)
        if (pack != nullptr && card->getId() == packId)
            card->setPack (*pack, true);
        else
            for (auto& p : packs)
                if (p.id == card->getId())
                    card->setPack (p, false);

    // The installed pack's own record has its credits, and its sounds say how many it has.
    const SoundPackInfo* info = nullptr;
    int soundCount = 0;
    auto& library = engine.getSounds();

    if (pack != nullptr && isInstalled (*pack))
    {
        for (auto& p : library.getPacks())
            if (p.id == packId)
                info = &p;

        soundCount = (int) std::count_if (library.getSounds().begin(), library.getSounds().end(),
                                          [&] (const SoundInfo& s) { return s.packId == packId; });

        if (info != nullptr && info->folder.isDirectory())
            soundCount += (int) std::count_if (library.getLoops().begin(), library.getLoops().end(),
                                               [&] (const LibraryFile& l) { return ! l.isFolder && l.file.isAChildOf (info->folder); });
    }

    details->show (pack, soundCount, info);

    if (scrollIntoView)
        for (auto& card : cards)
            if (card->getId() == packId)
            {
                const auto b = card->getBounds();
                const auto view = gridView.getViewArea();

                if (b.getY() < view.getY())
                    gridView.setViewPosition (0, juce::jmax (0, b.getY() - sectionHeight));
                else if (b.getBottom() > view.getBottom())
                    gridView.setViewPosition (0, b.getBottom() - view.getHeight());
            }
}

void SoundsBrowser::moveSelection (int delta)
{
    if (cards.empty())
        return;

    int index = 0;

    for (size_t i = 0; i < cards.size(); ++i)
        if (cards[i]->getId() == selectedId)
            index = (int) i;

    select (cards[(size_t) juce::jlimit (0, (int) cards.size() - 1, index + delta)]->getId(), true);
}

void SoundsBrowser::runMainAction (const juce::String& packId)
{
    auto& store = engine.getPackStore();
    auto p = store.findPack (packId);

    if (! p.has_value())
        return;

    if (p->state == StorePack::State::downloading)
        store.cancel (packId);
    else if (p->state == StorePack::State::notInstalled || p->state == StorePack::State::updateAvailable)
        store.install (packId);
}

void SoundsBrowser::confirmRemove (const juce::String& packId, const juce::String& name)
{
    auto options = juce::MessageBoxOptions()
                       .withTitle ("Remove \"" + name + "\"?")
                       .withMessage ("Its sounds and loops are deleted from this computer. Tracks that use them stay in "
                                     "your sessions and play again once you download the pack again.")
                       .withButton ("Remove", 1)
                       .withButton ("Cancel", 0)
                       .withAssociatedComponent (this);

    // The answer arrives later (no modal loops), so check we still exist.
    AlertDialog::show (options, [safeThis = SafePointer<SoundsBrowser> (this), packId] (int result)
    {
        if (safeThis == nullptr || result != 1)
            return;

        if (! safeThis->engine.getPackStore().remove (packId))
            AlertDialog::show (juce::MessageBoxOptions()
                               .withTitle ("Couldn't remove the pack")
                               .withMessage ("Wait for downloads to finish, or check that its folder isn't open elsewhere.")
                               .withButton ("OK", 0)
                               .withAssociatedComponent (safeThis.getComponent()));
    });
}

bool SoundsBrowser::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::leftKey)    { moveSelection (-1); return true; }
    if (key == juce::KeyPress::rightKey)   { moveSelection (1); return true; }
    if (key == juce::KeyPress::upKey)      { moveSelection (-columns); return true; }
    if (key == juce::KeyPress::downKey)    { moveSelection (columns); return true; }

    return false;
}

void SoundsBrowser::paint (juce::Graphics& g)
{
    g.setColour (theme().text);
    g.setFont (fonts::captionBold (textSize::hero - 4.0f).withExtraKerningFactor (0.14f));
    g.drawText ("SOUNDS", titleArea, juce::Justification::centredLeft, false);

    if (! notice.isEmpty())
    {
        g.setColour (theme().text);
        g.setFont (fonts::ui (textSize::md));
        g.drawFittedText (notice, noticeArea.withTrimmedRight (retryButton.isVisible() ? 112 : 0),
                          juce::Justification::centredLeft, 2, 1.0f);
    }

    if (storageText.isNotEmpty())
    {
        juce::AttributedString s;
        s.append (storageText, fonts::mono (textSize::md), theme().text);
        s.append (" of sounds on this computer", fonts::ui (textSize::md), theme().textSecondary);
        s.setJustification (juce::Justification::centredLeft);
        s.draw (g, storageArea.toFloat());
    }
}

void SoundsBrowser::paintOverChildren (juce::Graphics& g)
{
    // The magnifier inside the search field (its left indent leaves room).
    // Drawn over the children: the TextEditor fills its own background.
    icons::draw (g, Icon::search, searchField.getBounds().removeFromLeft (32).toFloat().translated (2.0f, 0.0f),
                 theme().textSecondary, 15.0f, 1.8f);
}

void SoundsBrowser::resized()
{
    auto r = getLocalBounds();

    // Top row: the title over the categories, the search field and the
    // All/Installed/Available buttons over the cards and details.
    auto top = r.removeFromTop (headerHeight);
    titleArea = top.removeFromLeft (categoryWidth);
    top.removeFromLeft (columnGap);

    const auto segmentFont = captionFont (textSize::md, 0.12f);

    for (auto [b, label] : { std::pair { &availableButton, "AVAILABLE" }, std::pair { &installedButton, "INSTALLED" },
                             std::pair { &allButton, "ALL" } })
    {
        const int w = juce::jmax (96, juce::roundToInt (widthOf (segmentFont, label)) + 32);
        b->setBounds (top.removeFromRight (w));
        top.removeFromRight (8);
    }

    top.removeFromRight (columnGap - 8);
    searchField.setBounds (top.removeFromRight (juce::jmin (360, top.getWidth())));
    r.removeFromTop (18);

    // Bottom row: space used, and Close.
    auto bottom = r.removeFromBottom (footerHeight);
    closeButton.setBounds (bottom.removeFromRight (130));
    storageArea = bottom.withTrimmedRight (16);
    r.removeFromBottom (16);

    // Left: the categories, then the starter sounds.
    auto left = r.removeFromLeft (categoryWidth);
    r.removeFromLeft (columnGap);
    categoryList->setBounds (left.removeFromTop (categoryList->getPreferredHeight()));

    if (starterCard->isVisible())
    {
        left.removeFromTop (14);
        starterCard->setBounds (left.removeFromTop (StarterCard::preferredHeight).reduced (4, 0));
    }

    // Right: the details.
    details->setBounds (r.removeFromRight (detailsWidth));
    r.removeFromRight (columnGap);

    // Middle: any notice, then the cards.
    if (notice.isNotEmpty())
    {
        noticeArea = r.removeFromTop (44);

        if (retryButton.isVisible())
            retryButton.setBounds (noticeArea.withLeft (noticeArea.getRight() - 104).withSizeKeepingCentre (104, 30));

        r.removeFromTop (8);
    }
    else
    {
        noticeArea = {};
    }

    gridView.setBounds (r);
    layoutGrid();
}

void SoundsBrowser::layoutGrid()
{
    const int width = gridView.getWidth() - gridView.getScrollBarThickness() - 6;
    columns = juce::jlimit (1, 4, (width + cardGap) / (minCardWidth + cardGap));
    const int cardWidth = (width - (columns - 1) * cardGap) / juce::jmax (1, columns);

    grid->headings.clear();
    int y = 0;
    juce::String currentGroup;
    int column = 0;

    for (size_t i = 0; i < cards.size(); ++i)
    {
        const StorePack* pack = nullptr;

        for (auto& p : packs)
            if (p.id == cards[i]->getId())
                pack = &p;

        const auto group = pack != nullptr ? groupOf (*pack) : juce::String ("Other");

        if (group != currentGroup)
        {
            if (column > 0)
                y += cardHeight + cardGap;   // finish the last row

            if (! grid->headings.empty())
                y += sectionGap;

            const auto count = std::count_if (cards.begin(), cards.end(), [&] (const auto& c)
            {
                return std::any_of (packs.begin(), packs.end(), [&] (const StorePack& p) { return p.id == c->getId() && groupOf (p) == group; });
            });

            grid->headings.push_back ({ { 0, y, width, sectionHeight }, labelOf (group),
                                        juce::String ((int) count) + (count == 1 ? " pack" : " packs") });
            y += sectionHeight + 6;
            currentGroup = group;
            column = 0;
        }

        cards[i]->setBounds (column * (cardWidth + cardGap), y, cardWidth, cardHeight);

        if (++column == columns)
        {
            column = 0;
            y += cardHeight + cardGap;
        }
    }

    if (column > 0)
        y += cardHeight;

    grid->setSize (juce::jmax (0, width), juce::jmax (y + 4, gridView.getHeight()));
    grid->repaint();
}

} // namespace lithium
