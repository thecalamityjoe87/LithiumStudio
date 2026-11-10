#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace lithium
{

//==============================================================================
/**
    The About page's BUILT WITH section: the projects Lithium Studio is made
    from. Tracktion Engine and JUCE get cards with their logos, side by side;
    the rest are rows of a two-column table (name, license), like Lithium
    Drums' About panel. Clicking a card or row opens the project's website.

    The full license texts are in the CREDITS file.
*/
class AboutTechnology  : public juce::Component
{
public:
    AboutTechnology();
    ~AboutTechnology() override;   // Entry is defined in the .cpp

    int getHeightForWidth (int width) const;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Entry;

    juce::OwnedArray<Entry> featured, others;
    juce::Rectangle<int> footer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutTechnology)
};

} // namespace lithium
