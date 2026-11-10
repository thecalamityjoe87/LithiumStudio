#pragma once

#include "engine/AudioEngine.h"
#include "ui/widgets/IconButton.h"

namespace lithium
{

//==============================================================================
/**
    Help > About Lithium Studio, laid out like Lithium Drums' About panel: a
    brand block on top (the app icon, the wordmark, the version, who made it,
    GitHub and License buttons), then three tabs:

        About           what the app is, its license notice, and what it's
                        built with (Tracktion Engine and JUCE with their
                        logos, then a table of the other projects)
        Credits         the sounds and grooves, with their authors, licenses
                        and sources
        Release Notes   what's in this version

    The License button shows the GPL's full text in place of the page, with
    the About tab still chosen.

    The sound credits come from each installed pack's pack.json, so downloaded
    packs are credited as soon as they're installed (several of them require
    it: CC BY).

    Shown inside the main window over a dimmed background, like the other
    dialogs. Deletes itself when closed.
*/
class AboutDialog  : public juce::Component
{
public:
    static void show (juce::Component& parent, AudioEngine&);

    ~AboutDialog() override;   // Page is defined in the .cpp

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;   // click outside the panel = close
    void lookAndFeelChanged() override;                  // the theme changed: recolour the text

private:
    class Page;
    enum class Tab { about, credits, releaseNotes, license };

    explicit AboutDialog (AudioEngine&);

    void showTab (Tab);
    void fillAbout (Page&);
    void fillCredits (Page&);
    void fillReleaseNotes (Page&);
    void applyLicenseColours();
    void close();

    AudioEngine& engine;
    juce::Image icon;

    std::unique_ptr<Page> aboutPage, creditsPage, releaseNotesPage;
    juce::Viewport pageView;
    juce::TextEditor licenseText;    // the GPL's full text

    IconButton aboutTab        { "About", std::nullopt, "ABOUT" };
    IconButton creditsTab      { "Credits", std::nullopt, "CREDITS" };
    IconButton releaseNotesTab { "Release Notes", std::nullopt, "RELEASE NOTES" };
    IconButton githubButton    { "Open the project on GitHub", std::nullopt, "GITHUB" };
    IconButton licenseButton   { "View the license", std::nullopt, "LICENSE" };
    IconButton closeButton     { "Close", std::nullopt, "CLOSE" };

    Tab tab = Tab::about;
    juce::Rectangle<int> panel, brandArea, tabsArea, contentArea;

    static constexpr int panelWidth = 760, panelHeight = 720;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutDialog)
};

} // namespace lithium
