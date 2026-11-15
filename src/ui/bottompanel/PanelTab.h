#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace lithium
{

//==============================================================================
/**
    The tab that sticks up from the collapsed bottom panel's top edge: click
    it to show the panel again. It's drawn in the panel's colours and
    reaches 1px down into the panel, over its top border, so it reads as
    part of the panel. The owner (MainComponent) places it and shows it
    only while the panel is collapsed.
*/
class PanelTab  : public juce::Button
{
public:
    PanelTab();

    static constexpr int width = 104;
    static constexpr int height = 21;      // including the 1px that overlaps the panel
    static constexpr int rightMargin = 14; // from the window's right edge (the panel's padding)

    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelTab)
};

} // namespace lithium
