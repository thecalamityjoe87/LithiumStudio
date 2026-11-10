#pragma once

#include <juce_core/juce_core.h>

#include <functional>
#include <vector>

// What the Linux installer installs, where, and how it removes it again, as
// Lithium Drums' installer does. No UI here: InstallerMain.cpp drives it from
// a window, or from the command line (--install / --remove).
//
// Everything goes into the user's home folder, so no password is needed:
//   the app          ~/.local/lib/lithium-studio/Lithium Studio, with its
//                    bundled sounds (Sounds/) beside it, where it looks for them
//   a command        ~/.local/bin/lithium-studio (a link to the app)
//   the menu entry   ~/.local/share/applications/<app id>.desktop
//   the icons        ~/.local/share/icons/hicolor/<size>/apps/<app id>.png (and .svg)
//   the file types   ~/.local/share/mime/packages/<app id>.xml (.lisession, .iksession)
//   starter sounds   ~/Documents/Lithium Studio/Sounds (optional, downloaded:
//                    the same packs, place and checks as the app's Manage Sounds)
// (~/.local/share is $XDG_DATA_HOME when that's set.)
//
// Not ~/.local/share/lithium-studio: that's the app's own data folder (open
// sessions' working copies, previews), which the app creates itself.
//
// Every path it creates is listed in ~/.local/lib/lithium-studio/install.json,
// and Remove deletes exactly those, so the user's sessions, sound packs and
// settings are never touched. The starter sounds aren't listed: once
// installed they're the user's packs, managed in the app like any other.

namespace lithium::installer
{

struct Part
{
    juce::String id, name, description;
    juce::String where;            // shown to the user, e.g. "~/.local"
    bool selectedByDefault = true;
    bool required = false;         // always installed: shown ticked, can't be unticked
};

/** The rows shown, in order. */
const std::vector<Part>& components();

/** What's installed now, read from the manifest. */
struct Existing
{
    bool installed = false;
    juce::String version;
    juce::StringArray components;
};

class Installation
{
public:
    /** `payload` is the folder of files to install (next to the installer:
        app/, icons/, the file types and the licence files). `version` is the
        version being installed. */
    Installation (juce::File payload, juce::String version);

    /** Checks the payload holds everything; returns an error message, or
        an empty string. */
    juce::String checkPayload() const;

    Existing existing() const;

    /** Status lines and progress (0 to 1) as the work goes. Called on the
        thread that runs install() or remove(). */
    using Progress = std::function<void (const juce::String& status, float progress)>;

    /** Removes any earlier install, then installs `componentIds`. Returns an
        error message, or an empty string. */
    juce::String install (const juce::StringArray& componentIds, const Progress&);

    /** Removes everything the manifest lists. Returns an error message, or
        an empty string. */
    juce::String remove (const Progress&);

    /** The installed app, if it is installed. */
    juce::File installedApp() const;

    /** After a successful install: anything that didn't work but didn't stop
        the install (the starter sounds couldn't be downloaded), or empty. */
    juce::String getWarning() const   { return warning; }

    /** Where the app keeps the user's sound packs. */
    static juce::File soundsFolder();

    static juce::File dataHome();     // $XDG_DATA_HOME or ~/.local/share
    static juce::File programFolder(); // ~/.local/lib/lithium-studio
    static juce::File manifestFile();

private:
    juce::File payload;
    juce::String version, warning;

    /** Downloads and installs the Starter Sounds packs. Returns an error, or empty. */
    juce::String installStarterSounds (const Progress&);
};

}
