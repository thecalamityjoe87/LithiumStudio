#pragma once

#include <juce_core/juce_core.h>

namespace lithium
{

/*  Ikaros Studio was renamed Lithium Studio. The first time Lithium Studio
    starts, this moves what the old name left behind to the new name's places:

        settings     <app data>/<oldSettingsName> -> <app data>/<newSettingsName>
                     (and IkarosScannedPlugins.xml in it -> LithiumScannedPlugins.xml)
        local data   platform::getPreviousLocalDataFolder() -> getLocalDataFolder()
                     (working copies of open sessions, previews, caches)
        documents    Documents/Ikaros Studio -> Documents/Lithium Studio
                     (saved sessions and downloaded sound packs)

    then rewrites the paths the settings remember (recent sessions, folders
    chosen in Preferences, session views) to point into the moved folders.

    A folder only moves when there's no folder under the new name yet, so this
    does its work once and nothing afterwards. A folder that can't be renamed
    is left where it was (only the small settings folder is copied instead).
    Call it before the engine opens its settings. Tests never call it.
*/
void moveFilesFromIkarosStudio (const juce::String& oldSettingsName, const juce::String& newSettingsName);

} // namespace lithium
