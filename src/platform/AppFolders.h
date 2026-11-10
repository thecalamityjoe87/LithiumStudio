#pragma once

#include <juce_core/juce_core.h>

namespace lithium::platform
{

/*  Where the app keeps large files that belong to this computer only (the
    unpacked copies of open sessions, see AudioEngine):

        Linux    $XDG_DATA_HOME/lithium-studio, or ~/.local/share/lithium-studio
        Windows  %LOCALAPPDATA%\Lithium Studio (not the roaming profile)
        macOS    ~/Library/Application Support/Lithium Studio

    Unlike the temp folder, it isn't cleared on restart (so a crashed
    session's work can be found again), and unlike the settings folder it's
    a sensible place for gigabytes of audio. The folder may not exist yet.
*/
juce::File getLocalDataFolder();

/** The same folder under the app's earlier name (Ikaros Studio). Only
    app/RenameMigration reads it, to move its contents across once. */
juce::File getPreviousLocalDataFolder();

} // namespace lithium::platform
