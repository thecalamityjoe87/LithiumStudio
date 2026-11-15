#pragma once

#include "engine/BuiltInEffects.h"
#include "engine/EngineTypes.h"
#include <array>

namespace lithium
{

//==============================================================================
/**
    The EQ tab's equaliser: eight bands, low to high:

        Low Cut   high-pass, 12 dB/octave (off until it's used)
        Low       low shelf
        Low Mid   bell
        Mid       bell
        High Mid  bell
        Presence  bell
        High      high shelf
        High Cut  low-pass, 12 dB/octave (off until it's used)

    Each band's frequency, gain and Q (a cut has an on switch instead of a
    gain) is a setting backed by a property on the plugin's state, so it's
    saved with the session and undo restores it (see BuiltInEffectPlugin).
    EqController is what the app talks to.

    The shelves and bells use the same filters as Tracktion's 4-band
    equaliser, which the EQ tab used before. upgradeLegacyEqs turns an
    older session's 4-band EQ into this one, keeping every setting, so it
    sounds exactly the same.

    Threading: applyToBuffer (the audio thread) reads the settings through
    getCurrentValue() and works out a band's filter again only when its
    settings change. Nothing in it allocates.
*/
class EqPlugin  : public BuiltInEffectPlugin
{
public:
    explicit EqPlugin (tracktion::PluginCreationInfo);
    ~EqPlugin() override;

    static const char* xmlTypeName;      // "ikarosChannelEq"
    static const char* getPluginName()   { return "EQ"; }

    juce::String getName() const override                   { return getPluginName(); }
    juce::String getPluginType() override                   { return xmlTypeName; }
    juce::String getShortName (int) override                { return "EQ"; }
    bool noTail() override                                  { return false; }
    double getTailLength() const override                   { return 0.2; }

    void initialise (const tracktion::PluginInitialisationInfo&) override;
    void reset() override;
    void applyToBuffer (const tracktion::PluginRenderContext&) override;

    //==============================================================================
    /** Where each band starts: every gain 0 dB, the cuts off. */
    static EqSettings getDefaults();

    /** One band's settings, and the state properties they're stored in.
        gain is null for a cut, on is null for the other bands. */
    struct Band
    {
        tracktion::AutomatableParameter* frequency = nullptr;
        tracktion::AutomatableParameter* gain = nullptr;
        tracktion::AutomatableParameter* q = nullptr;
        tracktion::AutomatableParameter* on = nullptr;
        juce::Identifier frequencyId, gainId, qId, onId;
    };

    Band getBand (int band) const;

    /** Whether a band changes the sound: a cut that's on, or another band
        with some gain. */
    static bool isActive (const EqBand&);

    /** A band's filter as normalised biquad coefficients (b0, b1, b2, a1, a2). */
    using Coefficients = std::array<double, 5>;
    static Coefficients design (const EqBand&, double sampleRate);

    /** How much a filter changes a frequency, in dB. */
    static double responseDb (const Coefficients&, double frequencyHz, double sampleRate);

    /** Turns every 4-band EQ that older versions put on the EQ tab (in a
        session's state, before it's loaded) into this plugin, with the
        same settings: Low, Low Mid, High Mid and High keep theirs. Returns
        how many it changed. */
    static int upgradeLegacyEqs (juce::ValueTree sessionState);

private:
    struct Filter
    {
        tracktion::AutomatableParameter::Ptr frequency, gain, q, on;
        EqBand settings;              // what the coefficients were worked out for
        Coefficients coefficients {};
        bool designed = false, active = false;
        std::array<std::array<double, 2>, 2> z {};   // per channel
    };

    std::array<Filter, EqSettings::numBands> filters;
    double rate = 44100.0;

    EqBand readBand (int band) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqPlugin)
};

} // namespace lithium
