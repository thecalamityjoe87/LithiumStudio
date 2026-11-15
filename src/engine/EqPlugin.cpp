#include "engine/EqPlugin.h"
#include "engine/TrackHelpers.h"
#include <complex>

namespace te = tracktion;

namespace lithium
{

namespace
{
    using Kind = EqBand::Kind;

    // Each band: the start of its property names (never rename one, or
    // older sessions lose that setting), its name, and where it starts.
    struct BandInfo
    {
        const char* key;
        const char* name;
        Kind kind;
        float frequencyHz, q;
    };

    // Gentle, musical starting points: a shelf below the bass and one above
    // the vocals' presence range, four wide bells spread evenly between them
    // (on the log scale), and the cuts out at the edges, ready to pull in.
    constexpr std::array<BandInfo, EqSettings::numBands> bandInfo {{
        { "lowCut",   "Low Cut",  Kind::lowCut,    30.0f,    0.71f },
        { "low",      "Low",      Kind::lowShelf,  100.0f,   0.7f  },
        { "lowMid",   "Low Mid",  Kind::bell,      250.0f,   0.7f  },
        { "mid",      "Mid",      Kind::bell,      630.0f,   0.7f  },
        { "highMid",  "High Mid", Kind::bell,      1600.0f,  0.7f  },
        { "presence", "Presence", Kind::bell,      4000.0f,  0.7f  },
        { "high",     "High",     Kind::highShelf, 10000.0f, 0.7f  },
        { "highCut",  "High Cut", Kind::highCut,   18000.0f, 0.71f },
    }};

    bool isCut (Kind k)   { return k == Kind::lowCut || k == Kind::highCut; }

    juce::Identifier propertyId (int band, const char* field)
    {
        return juce::String (bandInfo[(size_t) band].key) + field;
    }

    bool sameSettings (const EqBand& a, const EqBand& b)
    {
        return a.kind == b.kind && a.frequencyHz == b.frequencyHz && a.gainDb == b.gainDb
                && a.q == b.q && a.on == b.on;
    }
}

//==============================================================================
const char* EqPlugin::xmlTypeName = "ikarosChannelEq";

EqPlugin::EqPlugin (te::PluginCreationInfo info) : BuiltInEffectPlugin (info)
{
    // The same limits as EqController's. The frequency knob's middle is
    // 632 Hz, the middle of 20 Hz..20 kHz on a log scale.
    juce::NormalisableRange<float> frequencyRange { 20.0f, 20000.0f };
    frequencyRange.setSkewForCentre (632.0f);
    const auto qText = [] (float q) { return "Q " + juce::String (q, 2); };

    for (int b = 0; b < EqSettings::numBands; ++b)
    {
        const auto& i = bandInfo[(size_t) b];
        const juce::String name (i.name);
        auto& f = filters[(size_t) b];

        f.frequency = addSetting (propertyId (b, "Freq"), name + " Frequency", frequencyRange, i.frequencyHz, hertz);

        if (isCut (i.kind))
            f.on = addSetting (propertyId (b, "On"), name + " On", { 0.0f, 1.0f, 1.0f }, 0.0f,
                               [] (float v) { return juce::String (v >= 0.5f ? "On" : "Off"); });
        else
            f.gain = addSetting (propertyId (b, "Gain"), name + " Gain", { -20.0f, 20.0f }, 0.0f, decibels);

        f.q = addSetting (propertyId (b, "Q"), name + " Q", { 0.1f, 4.0f }, i.q, qText);
    }
}

EqPlugin::~EqPlugin()
{
    notifyListenersOfDeletion();
}

EqSettings EqPlugin::getDefaults()
{
    EqSettings s;

    for (size_t b = 0; b < s.bands.size(); ++b)
    {
        const auto& i = bandInfo[b];
        s.bands[b] = { i.name, i.kind, i.frequencyHz, 0.0f, i.q, ! isCut (i.kind) };
    }

    return s;
}

EqPlugin::Band EqPlugin::getBand (int band) const
{
    const auto& f = filters[(size_t) band];
    Band result;
    result.frequency = f.frequency.get();
    result.gain = f.gain.get();
    result.q = f.q.get();
    result.on = f.on.get();
    result.frequencyId = propertyId (band, "Freq");
    result.gainId = propertyId (band, "Gain");
    result.qId = propertyId (band, "Q");
    result.onId = propertyId (band, "On");
    return result;
}

EqBand EqPlugin::readBand (int band) const
{
    // Runs on the audio thread too: the name stays empty, so nothing allocates.
    const auto& f = filters[(size_t) band];
    EqBand b;
    b.kind = bandInfo[(size_t) band].kind;
    b.frequencyHz = f.frequency->getCurrentValue();
    b.gainDb = f.gain != nullptr ? f.gain->getCurrentValue() : 0.0f;
    b.q = f.q->getCurrentValue();
    b.on = f.on == nullptr || f.on->getCurrentValue() >= 0.5f;
    return b;
}

bool EqPlugin::isActive (const EqBand& b)
{
    return b.isCut() ? b.on : std::abs (b.gainDb) >= 0.01f;
}

//==============================================================================
// The filters
EqPlugin::Coefficients EqPlugin::design (const EqBand& band, double sampleRate)
{
    // JUCE's (the RBJ cookbook's) filters; the shelves and bells are the
    // ones Tracktion's 4-band equaliser used. Frequencies too close to the
    // top of what the sample rate can hold are pulled down.
    using Array = juce::dsp::IIR::ArrayCoefficients<double>;
    const auto f = juce::jlimit (10.0, sampleRate * 0.45, (double) band.frequencyHz);
    const auto q = juce::jmax (0.05, (double) band.q);
    const auto gain = juce::Decibels::decibelsToGain ((double) band.gainDb);

    std::array<double, 6> c;

    switch (band.kind)
    {
        case Kind::lowCut:    c = Array::makeHighPass (sampleRate, f, q); break;
        case Kind::highCut:   c = Array::makeLowPass (sampleRate, f, q); break;
        case Kind::lowShelf:  c = Array::makeLowShelf (sampleRate, f, q, gain); break;
        case Kind::highShelf: c = Array::makeHighShelf (sampleRate, f, q, gain); break;
        case Kind::bell:
        default:              c = Array::makePeakFilter (sampleRate, f, q, gain); break;
    }

    // (b0, b1, b2, a0, a1, a2) -> divided through by a0.
    return { c[0] / c[3], c[1] / c[3], c[2] / c[3], c[4] / c[3], c[5] / c[3] };
}

double EqPlugin::responseDb (const Coefficients& c, double frequencyHz, double sampleRate)
{
    // The filter's transfer function at z = e^(jw).
    const auto w = juce::MathConstants<double>::twoPi * frequencyHz / sampleRate;
    const auto z1 = std::polar (1.0, -w), z2 = z1 * z1;
    const auto h = (c[0] + c[1] * z1 + c[2] * z2) / (1.0 + c[3] * z1 + c[4] * z2);
    return juce::Decibels::gainToDecibels (std::abs (h), -120.0);
}

//==============================================================================
void EqPlugin::initialise (const te::PluginInitialisationInfo& info)
{
    rate = info.sampleRate;
    silence.prepare (rate, getTailLength());

    for (auto& f : filters)
        f.designed = false;   // the sample rate may have changed

    reset();
}

void EqPlugin::reset()
{
    for (auto& f : filters)
        f.z = {};
}

void EqPlugin::applyToBuffer (const te::PluginRenderContext& fc)
{
    if (fc.destBuffer == nullptr)
        return;

    if (! silence.shouldProcess (*fc.destBuffer, fc.bufferStartSample, fc.bufferNumSamples))
        return;

    SCOPED_REALTIME_CHECK

    auto& buffer = *fc.destBuffer;
    const int start = fc.bufferStartSample, num = fc.bufferNumSamples;
    const int channels = juce::jmin (2, buffer.getNumChannels());

    for (int b = 0; b < EqSettings::numBands; ++b)
    {
        auto& f = filters[(size_t) b];
        const auto settings = readBand (b);

        // A band that changes nothing is skipped. When it comes back, it
        // starts from silence rather than whatever it held before.
        if (! isActive (settings))
        {
            f.active = false;
            continue;
        }

        if (! f.active)
        {
            f.z = {};
            f.active = true;
        }

        if (! f.designed || ! sameSettings (settings, f.settings))
        {
            f.coefficients = design (settings, rate);
            f.settings = settings;
            f.designed = true;
        }

        const auto [b0, b1, b2, a1, a2] = f.coefficients;

        // Transposed direct form II, in doubles (the low bands' poles sit
        // very close to 1, where floats get noisy).
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* d = buffer.getWritePointer (ch, start);
            auto& z = f.z[(size_t) ch];

            for (int i = 0; i < num; ++i)
            {
                const double x = d[i];
                const double y = b0 * x + z[0];
                z[0] = b1 * x - a1 * y + z[1];
                z[1] = b2 * x - a2 * y;
                d[i] = (float) y;
            }
        }
    }
}

//==============================================================================
int EqPlugin::upgradeLegacyEqs (juce::ValueTree tree)
{
    int count = 0;

    if (tree.hasType (te::IDs::PLUGIN) && (bool) tree.getProperty (detail::eqProp, false)
         && tree[te::IDs::type].toString() == te::EqualiserPlugin::xmlTypeName)
    {
        // Its four bands are our Low, Low Mid, High Mid and High (bands 1,
        // 2, 4 and 6), with the same filters; the cuts stay off.
        struct Move { juce::Identifier from; int band; const char* field; };
        const Move moves[] = {
            { te::IDs::loFreq,   1, "Freq" }, { te::IDs::loGain,   1, "Gain" }, { te::IDs::loQ,   1, "Q" },
            { te::IDs::midFreq1, 2, "Freq" }, { te::IDs::midGain1, 2, "Gain" }, { te::IDs::midQ1, 2, "Q" },
            { te::IDs::midFreq2, 4, "Freq" }, { te::IDs::midGain2, 4, "Gain" }, { te::IDs::midQ2, 4, "Q" },
            { te::IDs::hiFreq,   6, "Freq" }, { te::IDs::hiGain,   6, "Gain" }, { te::IDs::hiQ,   6, "Q" },
        };

        tree.setProperty (te::IDs::type, xmlTypeName, nullptr);

        for (const auto& m : moves)
        {
            if (tree.hasProperty (m.from))
            {
                tree.setProperty (propertyId (m.band, m.field), tree.getProperty (m.from), nullptr);
                tree.removeProperty (m.from, nullptr);
            }
        }

        ++count;
    }

    for (auto child : tree)
        count += upgradeLegacyEqs (child);

    return count;
}

} // namespace lithium
