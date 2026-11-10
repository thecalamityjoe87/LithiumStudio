#include "ui/widgets/LevelMeter.h"
#include "ui/widgets/PaintProbe.h"
#include "ui/theme/Tokens.h"

namespace lithium
{

using namespace tokens;

LevelMeter::LevelMeter (Orientation o) : orientation (o)
{
    setInterceptsMouseClicks (false, false);
    setAccessible (false);   // decorative; the numbers aren't useful to a screen reader at 30 Hz
}

float LevelMeter::dbToProportion (float db)
{
    // Map -60 dB .. +6 dB onto 0..1, with a mild curve so normal levels sit
    // in the green part of the meter.
    const float minDb = -60.0f, maxDb = 6.0f;
    auto p = (juce::jlimit (minDb, maxDb, db) - minDb) / (maxDb - minDb);
    return std::pow (p, 1.6f);
}

float LevelMeter::fallTowards (float current, float target)
{
    // Instant attack, gradual release.
    return target > current ? target : current * 0.86f + target * 0.14f;
}

void LevelMeter::setLevel (StereoLevel level)
{
    auto newLeft = fallTowards (left, dbToProportion (level.leftDb));
    auto newRight = fallTowards (right, dbToProportion (level.rightDb));

    if (std::abs (newLeft - left) > 0.002f || std::abs (newRight - right) > 0.002f)
    {
        left = newLeft;
        right = newRight;
        repaint();
    }
}

void LevelMeter::reset()
{
    left = right = 0.0f;
    repaint();
}

void LevelMeter::paintBar (juce::Graphics& g, juce::Rectangle<float> r, float value, Orientation orientation, juce::Colour unlit)
{
    // As Lithium Drums' meters: the accent up to -6 dB, yellow up to 0 dB,
    // red above, in solid zones, with a line marking 0 dB.
    const bool horiz = orientation == Orientation::horizontal;
    g.setColour (unlit);
    g.fillRoundedRectangle (r, 2.0f);

    // The part of the bar from proportion a to b (0 = silent end).
    auto span = [&] (float a, float b)
    {
        return horiz ? juce::Rectangle<float> (r.getX() + r.getWidth() * a, r.getY(), r.getWidth() * (b - a), r.getHeight())
                     : juce::Rectangle<float> (r.getX(), r.getBottom() - r.getHeight() * b, r.getWidth(), r.getHeight() * (b - a));
    };

    const auto p6 = dbToProportion (-6.0f), p0 = dbToProportion (0.0f);

    if (value > 0.001f)
    {
        g.setColour (theme().accent.withMultipliedAlpha (0.85f));
        g.fillRect (span (0.0f, juce::jmin (value, p6)));

        if (value > p6)
        {
            g.setColour (theme().meterYellow);
            g.fillRect (span (p6, juce::jmin (value, p0)));
        }

        if (value > p0)
        {
            g.setColour (theme().meterRed);
            g.fillRect (span (p0, value));
        }
    }

    g.setColour (theme().borderSoft.withMultipliedAlpha (0.8f));
    g.fillRect (horiz ? juce::Rectangle<float> (r.getX() + r.getWidth() * p0, r.getY(), 1.0f, r.getHeight())
                      : juce::Rectangle<float> (r.getX(), r.getBottom() - r.getHeight() * p0, r.getWidth(), 1.0f));
}

void LevelMeter::paint (juce::Graphics& g)
{
    LITHIUM_PERF_PAINT ("LevelMeter");
    auto r = getLocalBounds().toFloat();

    if (orientation == Orientation::horizontal)
    {
        auto h = juce::jmin (5.0f, (r.getHeight() - 3.0f) / 2.0f);
        paintBar (g, r.removeFromTop (h), left, orientation, theme().controlFill);
        r.removeFromTop (3.0f);
        paintBar (g, r.removeFromTop (h), right, orientation, theme().controlFill);
    }
    else
    {
        // Lithium Drums' channel meter: two 6px bars, 2px apart, on the well colour.
        constexpr float w = 6.0f, gap = 2.0f;
        auto bars = r.withSizeKeepingCentre (w * 2.0f + gap, r.getHeight());
        paintBar (g, bars.removeFromLeft (w), left, orientation, theme().wellBg);
        bars.removeFromLeft (gap);
        paintBar (g, bars.removeFromLeft (w), right, orientation, theme().wellBg);
    }
}

} // namespace lithium
