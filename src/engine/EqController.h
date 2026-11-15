#pragma once

#include "engine/EngineTypes.h"

namespace lithium
{

class AudioEngine;

//==============================================================================
/**
    The "EQ" tab: an 8-band EQ on a track or on the master channel (pass
    masterTrackId). The bands are Low Cut, a low shelf, four bells, a high
    shelf and High Cut (see EqPlugin).

    Under the hood it's our own EqPlugin, kept off the effects list (it
    isn't one of the user's effects). It's only added to a
    track the first time a band is changed, so untouched tracks and older
    sessions don't change. A track's EQ sits after its effects; the
    master's after the master effects.

    Every method runs on the message thread. Changes record into the
    session's UndoManager.
*/
class EqController
{
public:
    explicit EqController (AudioEngine&);

    /** The EQ's settings: flat (every gain 0 dB) until it's been changed. */
    EqSettings getEq (TrackId) const;

    /** Whether the EQ has been added to the track yet (the first time a
        band, or its on/off, was changed). It stays after a Reset. */
    bool hasEq (TrackId) const;

    /** Sets one band. No undo step of its own: the UI starts one when a drag
        starts, so a whole drag undoes in one go. Values are clamped. A cut
        has no gain: gainDb is ignored for it. */
    void setBand (TrackId, int band, float frequencyHz, float gainDb, float q);

    /** Turns Low Cut or High Cut on or off (the other bands are always on).
        Like setBand, it goes into the current undo step. */
    void setBandOn (TrackId, int band, bool on);

    /** Turns the EQ on or off ("Turn EQ On/Off" undo step). */
    void setEnabled (TrackId, bool enabled);

    /** Back to flat, with every band at its starting frequency and the cuts
        off ("Reset EQ"). */
    void reset (TrackId);

    /** How loud the EQ makes each of these frequencies, in dB, for drawing
        its curve. (Worked out from the settings, so it's flat until the EQ
        is changed, and 0 everywhere while it's off.) */
    std::vector<float> getResponse (TrackId, const std::vector<float>& frequenciesHz) const;

    static constexpr float minFrequency = 20.0f, maxFrequency = 20000.0f;
    static constexpr float minGain = -20.0f, maxGain = 20.0f;
    static constexpr float minQ = 0.1f, maxQ = 4.0f;

    /** The settings of a flat EQ: where each band starts. */
    static EqSettings getDefaults();

private:
    AudioEngine& owner;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqController)
};

} // namespace lithium
