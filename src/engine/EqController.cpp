#include "engine/EqController.h"
#include "engine/AudioEngine.h"
#include "engine/EqPlugin.h"
#include "engine/TrackHelpers.h"

namespace te = tracktion;

namespace lithium
{

using namespace detail;

namespace
{
    // The EQ plugin on a track (or the master), if it has one yet.
    EqPlugin* findEq (te::PluginList& list)
    {
        for (auto* p : list)
            if (isEqPlugin (*p))
                if (auto* eq = dynamic_cast<EqPlugin*> (p))
                    return eq;

        return nullptr;
    }

    // Where a new EQ goes: after a track's effects, just before its volume
    // plugin (Tracktion keeps volume and meter at the end); at the end of
    // the master's effects.
    int insertIndex (te::PluginList& list)
    {
        for (int i = 0; i < list.size(); ++i)
            if (dynamic_cast<te::VolumeAndPanPlugin*> (list[i]) != nullptr)
                return i;

        return list.size();
    }

    // The EQ, added (as part of the current undo step) if it isn't there yet.
    // It starts flat: its settings' defaults are where the bands start, so
    // nothing needs writing into its state.
    EqPlugin* findOrAddEq (te::Edit& edit, te::PluginList& list)
    {
        if (auto* eq = findEq (list))
            return eq;

        auto state = te::createValueTree (te::IDs::PLUGIN, te::IDs::type, EqPlugin::xmlTypeName);
        state.setProperty (eqProp, true, nullptr);

        auto plugin = edit.getPluginCache().createNewPlugin (state);
        list.insertPlugin (plugin, insertIndex (list), nullptr);
        return findEq (list);
    }

    // The settings a plugin holds (or the defaults without one).
    EqSettings readSettings (EqPlugin* eq)
    {
        auto settings = EqController::getDefaults();

        if (eq == nullptr)
            return settings;

        for (int b = 0; b < EqSettings::numBands; ++b)
        {
            auto params = eq->getBand (b);
            auto& band = settings.bands[(size_t) b];
            band.frequencyHz = params.frequency->getCurrentValue();
            band.gainDb = params.gain != nullptr ? params.gain->getCurrentValue() : 0.0f;
            band.q = params.q->getCurrentValue();
            band.on = params.on == nullptr || params.on->getCurrentValue() >= 0.5f;
        }

        settings.enabled = eq->isEnabled();
        return settings;
    }
}

//==============================================================================
EqController::EqController (AudioEngine& o) : owner (o)
{
}

EqSettings EqController::getDefaults()
{
    return EqPlugin::getDefaults();
}

EqSettings EqController::getEq (TrackId id) const
{
    auto* list = findPluginList (owner.getEdit(), id);
    return readSettings (list != nullptr ? findEq (*list) : nullptr);
}

bool EqController::hasEq (TrackId id) const
{
    auto* list = findPluginList (owner.getEdit(), id);
    return list != nullptr && findEq (*list) != nullptr;
}

void EqController::setBand (TrackId id, int band, float frequencyHz, float gainDb, float q)
{
    auto* edit = owner.getEdit();
    auto* list = findPluginList (edit, id);

    if (list == nullptr || ! juce::isPositiveAndBelow (band, EqSettings::numBands))
        return;

    auto* eq = findOrAddEq (*edit, *list);

    if (eq == nullptr)
        return;

    auto params = eq->getBand (band);
    setParameterUndoably (*eq, *params.frequency, params.frequencyId, juce::jlimit (minFrequency, maxFrequency, frequencyHz));

    if (params.gain != nullptr)
        setParameterUndoably (*eq, *params.gain, params.gainId, juce::jlimit (minGain, maxGain, gainDb));

    setParameterUndoably (*eq, *params.q, params.qId, juce::jlimit (minQ, maxQ, q));
}

void EqController::setBandOn (TrackId id, int band, bool on)
{
    auto* edit = owner.getEdit();
    auto* list = findPluginList (edit, id);

    if (list == nullptr || ! juce::isPositiveAndBelow (band, EqSettings::numBands)
         || ! getDefaults().bands[(size_t) band].isCut() || getEq (id).bands[(size_t) band].on == on)
        return;

    if (auto* eq = findOrAddEq (*edit, *list))
    {
        auto params = eq->getBand (band);
        setParameterUndoably (*eq, *params.on, params.onId, on ? 1.0f : 0.0f);
    }
}

void EqController::setEnabled (TrackId id, bool enabled)
{
    auto* edit = owner.getEdit();
    auto* list = findPluginList (edit, id);

    if (list == nullptr || getEq (id).enabled == enabled)
        return;

    owner.beginUndoStep (enabled ? "Turn EQ On" : "Turn EQ Off");

    if (auto* eq = findOrAddEq (*edit, *list))
        eq->setEnabled (enabled);
}

void EqController::reset (TrackId id)
{
    auto* list = findPluginList (owner.getEdit(), id);
    auto* eq = list != nullptr ? findEq (*list) : nullptr;

    if (eq == nullptr)
        return;   // never changed: already flat

    owner.beginUndoStep ("Reset EQ");
    const auto defaults = getDefaults();

    for (int b = 0; b < EqSettings::numBands; ++b)
    {
        const auto& band = defaults.bands[(size_t) b];
        setBand (id, b, band.frequencyHz, band.gainDb, band.q);

        if (band.isCut())
            setBandOn (id, b, band.on);
    }
}

std::vector<float> EqController::getResponse (TrackId id, const std::vector<float>& frequencies) const
{
    std::vector<float> result (frequencies.size(), 0.0f);
    const auto settings = getEq (id);

    if (! settings.enabled)
        return result;

    // The plugin's own filters. Each one's effect, in dB, adds up. The
    // sample rate only matters near the top of the range, so a typical one
    // does.
    constexpr double sampleRate = 48000.0;

    for (const auto& band : settings.bands)
    {
        if (! EqPlugin::isActive (band))
            continue;   // a flat band (or a cut that's off) changes nothing

        const auto coefficients = EqPlugin::design (band, sampleRate);

        for (size_t i = 0; i < frequencies.size(); ++i)
            result[i] += (float) EqPlugin::responseDb (coefficients, frequencies[i], sampleRate);
    }

    return result;
}

} // namespace lithium
