// Guitarix's amp models, tone stacks and cabinet IRs, compiled without the
// LV2 wrapper (see GuitarixAmp.h).
//
// One file for all of them, as Guitarix's own gx_amp.cc does: the models
// share valve.h, whose tube tables are plain global arrays (defining them in
// two files would clash). valve.h is included once, below, and the models'
// own `#include "valve.h"` then finds it already done.

#include "GuitarixAmp.h"

// This Faust code calls std::min/std::max itself, so it mustn't get the old
// code's min/max macros (see GxFaustSupport.h, shared with the pedals).
#define LITHIUM_GX_STD_MINMAX
#include "GxFaustSupport.h"

namespace lithium::gx::amp
{
    // The LV2 ports, numbered as in trunk/src/LV2/gx_amp.lv2/gxamp.h. The
    // DSPs' connect() switches on these names.
    enum PortIndex
    {
        GAIN1 = 0,
        PREGAIN,
        WET_DRY,
        DRIVE,
        MIDDLE,
        BASS,
        TREBLE,
        CLevel,
        ALevel,
        MODEL,
        T_MODEL,
        C_MODEL,
        AMP_CONTROL,
        AMP_NOTIFY,
        SCHEDULE,
        AMP_OUTPUT,
        AMP_INPUT,
        BYPASS,
        HIGHGAIN,
        TRIM
    };

    static_assert (GAIN1 == gain1 && PREGAIN == pregain && WET_DRY == wetDry && DRIVE == drive && MIDDLE == middle
                     && BASS == bass && TREBLE == treble && HIGHGAIN == highGain,
                   "GuitarixAmp.h's port numbers are out of date");

    // Powers the newer Faust code calls by name. Guitarix defines them in its
    // LV2 wrappers' gx_common.h (which we don't use); these are the same.
    template <class T> inline T mydsp_faustpower2_f (T x)    { return x * x; }
    template <class T> inline T mydsp_faustpower3_f (T x)    { return x * x * x; }
    template <class T> inline T mydsp_faustpower4_f (T x)    { return x * x * x * x; }
    template <class T> inline T mydsp_faustpower5_f (T x)    { return x * x * x * x * x; }

    // The tube tables, and the functions the models look them up with.
    #include "valve.h"

    // The amp models. Each is in a namespace of its own (gxamp, gxamp5...).
    #include "gxamp.cc"
    #include "gxamp5.cc"
    #include "gxamp7.cc"
    #include "gxamp8.cc"
    #include "gxamp9.cc"
    #include "gxamp16.cc"

    // The tone stacks.
    #include "tonestack_fender_default.cc"
    #include "tonestack_bassman.cc"
    #include "tonestack_ac30.cc"
    #include "tonestack_jcm800.cc"
    #include "tonestack_engl.cc"
    #include "tonestack_ampeg.cc"

    // The cabinet impulse responses (and the presence IR, which isn't used:
    // Lithium Studio's presence is a filter, see src/engine/AmpPlugin.cpp).
    #include "cab_data.cc"

    PluginLV2* createModel (Model m)
    {
        switch (m)
        {
            case Model::gxamp:      return gxamp::plugin();
            case Model::gxamp5:     return gxamp5::plugin();
            case Model::gxamp7:     return gxamp7::plugin();
            case Model::gxamp8:     return gxamp8::plugin();
            case Model::gxamp9:     return gxamp9::plugin();
            case Model::gxamp16:    return gxamp16::plugin();
        }

        return gxamp::plugin();
    }

    PluginLV2* createToneStack (ToneStack t)
    {
        switch (t)
        {
            case ToneStack::fenderDefault:  return tonestack_fender_default::plugin();
            case ToneStack::bassman:        return tonestack_bassman::plugin();
            case ToneStack::ac30:           return tonestack_ac30::plugin();
            case ToneStack::jcm800:         return tonestack_jcm800::plugin();
            case ToneStack::engl:           return tonestack_engl::plugin();
            case ToneStack::ampeg:          return tonestack_ampeg::plugin();
        }

        return tonestack_fender_default::plugin();
    }

    CabinetIR getCabinet (int index)
    {
        // In Guitarix's order (its DSP/cab_data_table.cc).
        #define LITHIUM_CAB(name, data)    { name, data.ir_count, data.ir_sr, data.ir_data }

        static const CabinetIR cabinets[numCabinets] {
            LITHIUM_CAB ("4x12",      cab_data_4x12),
            LITHIUM_CAB ("2x12",      cab_data_2x12),
            LITHIUM_CAB ("1x12",      cab_data_1x12),
            LITHIUM_CAB ("4x10",      cab_data_4x10),
            LITHIUM_CAB ("2x10",      cab_data_2x10),
            LITHIUM_CAB ("HighGain",  cab_data_HighGain),
            LITHIUM_CAB ("Twin",      cab_data_Twin),
            LITHIUM_CAB ("Bassman",   cab_data_Bassman),
            LITHIUM_CAB ("Marshall",  cab_data_Marshall),
            LITHIUM_CAB ("AC30",      cab_data_AC30),
            LITHIUM_CAB ("Princeton", cab_data_Princeton),
            LITHIUM_CAB ("A2",        cab_data_A2),
            LITHIUM_CAB ("1x15",      cab_data_1x15),
            LITHIUM_CAB ("mesa",      cab_data_mesa),
            LITHIUM_CAB ("briliant",  cab_data_briliant),
            LITHIUM_CAB ("vitalize",  cab_data_vitalize),
            LITHIUM_CAB ("charisma",  cab_data_charisma),
            LITHIUM_CAB ("1x8",       cab_data_1x8),
        };

        #undef LITHIUM_CAB

        return cabinets[index >= 0 && index < numCabinets ? index : 0];
    }
}
