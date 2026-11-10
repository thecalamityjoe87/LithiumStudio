#include "platform/AppFolders.h"

namespace lithium::platform
{

namespace
{
    // linuxName is the XDG-style folder name ("lithium-studio"); the other
    // systems use the app's display name ("Lithium Studio").
    juce::File localDataFolderCalled (const juce::String& linuxName, const juce::String& displayName)
    {
       #if JUCE_LINUX || JUCE_BSD
        juce::ignoreUnused (displayName);
        // The XDG base directory spec: $XDG_DATA_HOME, defaulting to ~/.local/share.
        const auto xdg = juce::SystemStats::getEnvironmentVariable ("XDG_DATA_HOME", {});
        const auto base = xdg.isNotEmpty() && juce::File::isAbsolutePath (xdg)
                            ? juce::File (xdg)
                            : juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile (".local/share");
        return base.getChildFile (linuxName);
       #elif JUCE_WINDOWS
        juce::ignoreUnused (linuxName);
        return juce::File::getSpecialLocation (juce::File::windowsLocalAppData).getChildFile (displayName);
       #else
        juce::ignoreUnused (linuxName);
        // macOS: userApplicationDataDirectory is ~/Library; apps keep their data
        // in Application Support inside it.
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Application Support").getChildFile (displayName);
       #endif
    }
}

juce::File getLocalDataFolder()
{
    return localDataFolderCalled ("lithium-studio", "Lithium Studio");
}

juce::File getPreviousLocalDataFolder()
{
    return localDataFolderCalled ("ikaros-studio", "Ikaros Studio");
}

} // namespace lithium::platform
