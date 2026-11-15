#pragma once

#include "engine/AudioEngine.h"
#include "ui/widgets/IconButton.h"
#include "ui/widgets/Knob.h"
#include "ui/theme/Family.h"

namespace lithium
{

//==============================================================================
/**
    The "EQ" tab: an 8-band EQ for the selected track or for the master:
    Low Cut, a low shelf, four bells, a high shelf and High Cut.

        [⏻ On]                                              [Reset]
        ┌────────────────────────────────────────────────────────┐
        │ +12 ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ │  the curve: how
        │   0 ─①──②───③─╱‾④‾╲──⑤─────⑥─────────⑦────⑧───│  loud each
        │ -12           100 Hz        1 kHz          10 kHz      │  frequency gets
        └────────────────────────────────────────────────────────┘
          Low Cut 30 Hz Off   Low 100 Hz +0 dB   Low Mid ...

    Drag a band's point: sideways for its frequency, up and down to boost or
    cut. The mouse wheel over a point makes the band wider or narrower, and a
    double-click puts it back to 0 dB. Every drag is one undo step.

    The two cuts (① and ⑧) have no gain, so their points stay on the 0 dB
    line. They're off (a hollow point) until clicked or dragged; a
    double-click turns one off again.

    Beside the graph, a card with knobs for the selected band (click its
    point, or its readout underneath, to select it): Frequency, Gain and Q.
    A cut has just Frequency and Q, with an On/Off button beside its name;
    turning one of its knobs turns it on.
*/
class EqView  : public juce::Component
{
public:
    explicit EqView (AudioEngine&);

    /** Which EQ to show: a track's, or the master's (masterTrackId). 0 = none. */
    void setTarget (TrackId, tokens::Family);

    /** Re-reads the settings from the engine (model changed). */
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    AudioEngine& engine;
    TrackId target = 0;
    tokens::Family family = tokens::Family::master;   // the curve's colour, looked up when painting
    EqSettings settings;
    TrackId shownTarget = 0;                                // what refresh() last drew, so it can skip
    tokens::Family shownFamily = tokens::Family::master;    // redrawing when nothing changed
    bool shownOnce = false;
    std::vector<float> curve;      // dB at each column of the graph

    IconButton powerButton { "EQ on/off", Icon::power, "On" };
    IconButton resetButton { "Reset EQ", std::nullopt, "Reset" };

    // The selected band's knobs (see updateKnobs).
    Knob frequencyKnob { "Frequency" }, gainKnob { "Gain" }, qKnob { "Q" };
    IconButton cutButton { "Turn band on or off", Icon::power, "Off" };

    juce::Rectangle<int> graphArea, readoutArea, knobCard, bandTitleArea, knobRow;
    int dragBand = -1, hoverBand = -1;
    int selectedBand = 1;   // Low, to start with

    // The graph: frequency across (on a log scale, like your ears hear it),
    // dB up and down.
    float frequencyToX (float hz) const;
    float xToFrequency (float x) const;
    float gainToY (float db) const;
    float yToGain (float y) const;
    juce::Point<float> bandPoint (int band) const;
    int bandAt (juce::Point<float>) const;

    void setBand (int band, float frequencyHz, float gainDb, float q);
    void selectBand (int band);
    void updateKnobs();
    void layOutKnobs();
    void turnSelectedCutOn();
    int readoutColumnAt (juce::Point<float>) const;
    void updateCurve();
    static juce::String describeFrequency (float hz);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqView)
};

} // namespace lithium
