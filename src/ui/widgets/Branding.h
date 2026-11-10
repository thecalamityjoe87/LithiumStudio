#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace lithium::branding
{

/*  The app's branding, wherever it appears (the title bar, the session
    chooser): the logo's rounded tile, then the Lithium family wordmark,
    LITHIUM in bold and STUDIO muted, condensed and widely spaced.

        [logo] LITHIUM STUDIO
*/

/** The logo, decoded once from the embedded PNG. */
juce::Image getLogo();

/** Just the logo's tile, filling the area, without the icon's border: the
    border is for the icon on the desktop; inside the app it's left off. */
void paintLogo (juce::Graphics&, juce::Rectangle<float> area);

/** The width the branding needs. */
int getWidth();

/** Paints the branding at the left of an area, centred vertically. */
void paint (juce::Graphics&, juce::Rectangle<float> area);

/** Just the wordmark, at any size (the splash and the About window show it
    large under the full icon), placed in the area by the justification. */
void paintWordmark (juce::Graphics&, juce::Rectangle<float> area, float fontHeight, juce::Justification);

} // namespace lithium::branding
