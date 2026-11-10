#pragma once

#include <juce_graphics/juce_graphics.h>

namespace lithium::fonts
{

// The bundled typefaces (compiled into the binary via juce_add_binary_data).
// Each function returns a juce::Font at the given pixel height. They're the
// Lithium family's fonts: Barlow for text, Barlow Condensed for uppercase
// captions, IBM Plex Mono for numbers.

juce::Font ui (float height);            // Barlow Regular
juce::Font uiMedium (float height);      // Barlow Medium
juce::Font uiSemiBold (float height);    // Barlow SemiBold
juce::Font uiBold (float height);        // Barlow SemiBold (Lithium's heaviest text weight)
juce::Font uiExtraBold (float height);   // Figtree ExtraBold (pedal and amp names: part of the pedals' own artwork)
juce::Font caption (float height);       // Barlow Condensed SemiBold: uppercase section captions (TRACKS, EFFECTS...)
juce::Font captionBold (float height);   // Barlow Condensed Bold: the wordmark
juce::Font mono (float height);          // IBM Plex Mono Regular
juce::Font monoBold (float height);      // IBM Plex Mono Medium

/** The default UI typeface, handed to the LookAndFeel so plain JUCE widgets
    (menus, dialogs, text editors) also use Barlow. */
juce::Typeface::Ptr defaultTypeface();

} // namespace lithium::fonts
