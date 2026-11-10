#include "app/RenameMigration.h"

#include "platform/AppFolders.h"

#include <utility>
#include <vector>

namespace lithium
{

namespace
{
    // Renames a folder, if there's one to move and nothing under the new name.
    // copyIfNeeded: if the rename fails (another drive), copy it instead.
    bool moveFolder (const juce::File& from, const juce::File& to, bool copyIfNeeded)
    {
        if (! from.isDirectory() || to.exists() || to.getParentDirectory().createDirectory().failed())
            return false;

        if (from.moveFileTo (to))
            return true;

        if (copyIfNeeded && from.copyDirectoryTo (to))
        {
            from.deleteRecursively();
            return true;
        }

        DBG ("Couldn't move " + from.getFullPathName() + " to " + to.getFullPathName());
        return false;
    }

    // Replaces the path `from` with `to` where it's the whole path or the start
    // of a longer one (".../Ikaros Studio/Sounds"), but not where it's only the
    // start of another name (".../Ikaros Studio 2").
    juce::String replacePath (const juce::String& text, const juce::String& from, const juce::String& to)
    {
        juce::String result;
        int start = 0;

        for (auto i = text.indexOf (from); i >= 0; i = text.indexOf (start, from))
        {
            const auto next = text[i + from.length()];
            const bool continuesName = juce::CharacterFunctions::isLetterOrDigit (next)
                                         || next == ' ' || next == '_' || next == '-' || next == '.';

            result << text.substring (start, i) << (continuesName ? from : to);
            start = i + from.length();
        }

        return result + text.substring (start);
    }

    // The settings folder holds Tracktion's Settings.xml and our own XML files
    // (Session Views.xml...). Rewrites the moved paths in each of them.
    void rewritePaths (const juce::File& settingsFolder, const std::vector<std::pair<juce::String, juce::String>>& moved)
    {
        for (const auto& file : settingsFolder.findChildFiles (juce::File::findFiles, false, "*.xml;*.settings"))
        {
            const auto original = file.loadFileAsString();
            auto text = original;

            for (const auto& [from, to] : moved)
                text = replacePath (text, from, to);

            if (text != original && ! file.replaceWithText (text))
                DBG ("Couldn't update the paths in " + file.getFullPathName());
        }
    }
}

void moveFilesFromIkarosStudio (const juce::String& oldSettingsName, const juce::String& newSettingsName)
{
    const auto appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    const auto documents = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    const auto settings = appData.getChildFile (newSettingsName);

    moveFolder (appData.getChildFile (oldSettingsName), settings, true);

    // The plug-in scanner's record (and its backup) had the old name too.
    for (const auto* suffix : { ".xml", ".xml.bak" })
    {
        const auto oldRecord = settings.getChildFile (juce::String ("IkarosScannedPlugins") + suffix);
        const auto newRecord = settings.getChildFile (juce::String ("LithiumScannedPlugins") + suffix);

        if (oldRecord.existsAsFile() && ! newRecord.exists())
            oldRecord.moveFileTo (newRecord);
    }

    std::vector<std::pair<juce::String, juce::String>> moved;

    for (const auto& [from, to] : { std::pair { platform::getPreviousLocalDataFolder(), platform::getLocalDataFolder() },
                                    std::pair { documents.getChildFile ("Ikaros Studio"), documents.getChildFile ("Lithium Studio") } })
    {
        if (moveFolder (from, to, false))
            moved.emplace_back (from.getFullPathName(), to.getFullPathName());
    }

    if (! moved.empty() && settings.isDirectory())
        rewritePaths (settings, moved);
}

} // namespace lithium
