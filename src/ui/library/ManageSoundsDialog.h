#pragma once

#include "engine/AudioEngine.h"
#include "ui/widgets/IconButton.h"

namespace lithium
{

class SoundsBrowser;

//==============================================================================
/**
    "Manage Sounds": the sound packs on this computer and the ones the user
    can download, like GarageBand's Sound Library. The window is a
    SoundsBrowser (categories, pack cards and a details pane, laid out like
    Lithium Drums' kit browser) in a large panel over the main window.

    On first run it opens in its welcome form instead (showWelcome): a small
    panel offering the starter sounds, with "Not Now". Without the internet
    it says so, with Try Again.

    Shown inside the main window over a dimmed background, like the New
    Track dialog. Deletes itself when closed.
*/
class ManageSoundsDialog  : public juce::Component,
                          private juce::ChangeListener
{
public:
    /** Opens the dialog over `parent`. With a pack id, that pack starts
        downloading straight away (the Sound tab's Download button). */
    static void show (juce::Component& parent, AudioEngine&, const juce::String& downloadPackId = {});

    /** The first-run welcome: offers the starter sounds, with "Not Now".
        Calls onClosed when it closes. */
    static void showWelcome (juce::Component& parent, AudioEngine&, std::function<void()> onClosed);

    /** True until the user has answered the first-run offer (downloaded the
        starter sounds or said Not Now). */
    static bool shouldOfferStarterSounds (AudioEngine&);

    ~ManageSoundsDialog() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;   // click outside the panel = close

private:
    class Row;

    ManageSoundsDialog (AudioEngine&, bool welcome, const juce::String& downloadPackId, std::function<void()> onClosed);

    AudioEngine& engine;
    const bool welcome;                // the first-run form
    juce::String pendingDownload;      // the pack to download once the index has arrived
    bool requestedPackUnavailable = false;   // ...and it turned out not to be in the index
    bool starterRequested = false;     // welcome: the user chose to download the starter sounds
    std::function<void()> onClosed;

    std::unique_ptr<SoundsBrowser> browser;   // Manage Sounds (not the welcome)

    // The welcome form.
    juce::Viewport listViewport;
    juce::Component listHolder;
    juce::OwnedArray<Row> rows;
    IconButton retryButton { "Try again", std::nullopt, "Try Again" };
    IconButton notNowButton { "Not now", std::nullopt, "Not Now" };
    IconButton doneButton { "Done", std::nullopt, "Done" };
    juce::String doneText { "Done" };  // what doneButton says (for its width)
    juce::Rectangle<int> panel, statusArea;

    juce::String statusText;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void refresh();
    void markStarterOffered();
    void close();

    static constexpr int panelWidth = 680, panelHeight = 600;            // the welcome
    static constexpr int browserMaxWidth = 1180, browserMaxHeight = 760;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ManageSoundsDialog)
};

} // namespace lithium
