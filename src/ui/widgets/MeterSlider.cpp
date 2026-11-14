#include "ui/widgets/MeterSlider.h"
#include "ui/widgets/PaintProbe.h"
#include "ui/theme/LithiumLookAndFeel.h"
#include "ui/theme/Tokens.h"

namespace lithium
{

using namespace tokens;

MeterSlider::MeterSlider()
    : juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox)
{
    setRange (0.0, 1.0);
}

void MeterSlider::setLevel (StereoLevel level)
{
    auto newLeft = LevelMeter::fallTowards (left, LevelMeter::dbToProportion (level.leftDb));
    auto newRight = LevelMeter::fallTowards (right, LevelMeter::dbToProportion (level.rightDb));

    if (std::abs (newLeft - left) > 0.002f || std::abs (newRight - right) > 0.002f)
    {
        left = newLeft;
        right = newRight;
        repaint();
    }
}

void MeterSlider::paint (juce::Graphics& g)
{
    LITHIUM_PERF_PAINT ("MeterSlider");
    // juce::Slider normally asks the LookAndFeel to draw it. This draws it
    // itself: the meter bars across the handle's travel, then the handle.
    // getPositionOfValue gives the handle's x for a value, so the bars line
    // up with where the handle can go.
    const auto start = (float) getPositionOfValue (getMinimum());
    const auto end = (float) getPositionOfValue (getMaximum());
    const auto centreY = (float) getHeight() * 0.5f;

    const float barHeight = 4.0f, barGap = 2.0f;
    auto bars = juce::Rectangle<float> (start, centreY - barHeight - barGap * 0.5f, end - start, barHeight * 2.0f + barGap);
    const auto unlit = style == Style::trackHeader ? theme().volumeSliderTrack : theme().sliderTrack;
    LevelMeter::paintBar (g, bars.removeFromTop (barHeight), left, LevelMeter::Orientation::horizontal, unlit);
    bars.removeFromTop (barGap);
    LevelMeter::paintBar (g, bars.removeFromTop (barHeight), right, LevelMeter::Orientation::horizontal, unlit);

    const auto x = (float) getPositionOfValue (getValue());

    // The handle is the console fader's cap, turned on its side: on a track
    // header its line is in the track's colour, on the master's the neutral
    // faderCapLine. It is as wide as JUCE keeps free at each end of the
    // travel, so it stays inside the slider.
    const auto capWidth = 2.0f * (float) getLookAndFeel().getSliderThumbRadius (*this);
    const auto cap = juce::Rectangle<float> (capWidth, juce::jmin (20.0f, (float) getHeight())).withCentre ({ x, centreY });
    LithiumLookAndFeel::drawConsoleCap (g, cap, false, *this);

    if (hasKeyboardFocus (false))
        LithiumLookAndFeel::drawFocusRing (g, getLocalBounds().toFloat().expanded (2.0f), size::radiusXs);
}

} // namespace lithium
