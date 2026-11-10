#pragma once

#include <juce_core/juce_core.h>

namespace lithium::licenses
{

/** An SPDX id as people write it: "CC-BY-SA-3.0" -> "CC BY-SA 3.0", "CC0-1.0" -> "CC0 1.0".
    Anything else is returned as it is. */
inline juce::String displayName (const juce::String& spdx)
{
    if (! spdx.startsWith ("CC"))
        return spdx;

    auto version = spdx.fromLastOccurrenceOf ("-", false, false);

    if (! version.containsChar ('.'))
        return spdx.replaceCharacter ('-', ' ');

    auto rest = spdx.upToLastOccurrenceOf ("-", false, false);   // "CC-BY-SA" or "CC0"
    return rest.startsWith ("CC-") ? "CC " + rest.substring (3) + " " + version : rest + " " + version;
}

/** Where to read the license: its page on spdx.org. */
inline juce::URL url (const juce::String& spdx)
{
    return juce::URL ("https://spdx.org/licenses/" + spdx + ".html");
}

} // namespace lithium::licenses
