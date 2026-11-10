#pragma once

#include "engine/AudioEngine.h"
#include "ui/widgets/IconButton.h"

namespace lithium
{

//==============================================================================
/**
    The inside of the Manage Sounds window, laid out like Lithium Drums' kit
    browser:

        SOUNDS            [search]                [All] [Installed] [Available]
        Categories        pack cards, under         the selected pack: what it
        (with counts)     category headings         is, its sizes, credits,
        Starter Sounds                              license, and its button
        <space used>                                                    [Close]

    Every pack the index offers and every installed pack is a card. Installed
    packs are solid cards; packs that can be downloaded are dashed and dimmed,
    and show their download's progress. The details pane has the actions:
    Download, Update, Cancel while it downloads, and Remove (not for packs
    that come with the app).

    It reads everything from the engine's PackStore and redraws whenever the
    store says something changed (download progress included). Without the
    internet it says so and still lists the installed packs.
*/
class SoundsBrowser  : public juce::Component,
                       private juce::ChangeListener
{
public:
    /** With a pack id, that pack is selected and starts downloading as soon
        as the index lists it (the Sound tab's Download button). */
    SoundsBrowser (AudioEngine&, const juce::String& downloadPackId);
    ~SoundsBrowser() override;

    /** Called when the Close button is clicked. */
    std::function<void()> onClose;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    class CategoryList;
    class StarterCard;
    class Grid;
    class PackCard;
    class Details;

    enum class Show { all, installed, available };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void refresh();
    void layoutGrid();
    void setShow (Show);
    void setCategory (const juce::String&);
    void select (const juce::String& packId, bool scrollIntoView);
    void moveSelection (int delta);
    void runMainAction (const juce::String& packId);
    void confirmRemove (const juce::String& packId, const juce::String& name);
    bool isListed (const StorePack&) const;

    AudioEngine& engine;
    juce::String pendingDownload;            // the pack to download once the index has arrived
    bool requestedPackUnavailable = false;   // ...and it turned out not to be in the index

    std::vector<StorePack> packs;            // from the store, in its order (by name)
    juce::String selectedId;
    juce::String category;                   // a library category, "Other", or empty for all
    Show show = Show::all;
    juce::String notice;                     // offline, still loading, nothing to download...

    juce::TextEditor searchField;
    IconButton allButton { "Show all packs", std::nullopt, "ALL" };
    IconButton installedButton { "Show installed packs", std::nullopt, "INSTALLED" };
    IconButton availableButton { "Show packs to download", std::nullopt, "AVAILABLE" };
    IconButton retryButton { "Try again", std::nullopt, "Try Again" };
    IconButton closeButton { "Close", std::nullopt, "Close" };

    std::unique_ptr<CategoryList> categoryList;
    std::unique_ptr<StarterCard> starterCard;
    juce::Viewport gridView;
    std::unique_ptr<Grid> grid;
    std::vector<std::unique_ptr<PackCard>> cards;   // the listed packs, in the order shown
    std::unique_ptr<Details> details;

    juce::Rectangle<int> titleArea, noticeArea, storageArea;
    juce::String storageText;
    int columns = 3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundsBrowser)
};

} // namespace lithium
