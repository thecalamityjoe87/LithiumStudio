#include "ui/theme/Fonts.h"
#include "BinaryData.h"

namespace lithium::fonts
{

namespace
{
    // Loads each typeface once, the first time it's asked for.
    // A function-local `static` is initialised exactly once (thread-safe since C++11).
    struct Typefaces
    {
        juce::Typeface::Ptr regular   = load (BinaryData::BarlowRegular_ttf,             BinaryData::BarlowRegular_ttfSize);
        juce::Typeface::Ptr medium    = load (BinaryData::BarlowMedium_ttf,              BinaryData::BarlowMedium_ttfSize);
        juce::Typeface::Ptr semiBold  = load (BinaryData::BarlowSemiBold_ttf,            BinaryData::BarlowSemiBold_ttfSize);
        juce::Typeface::Ptr condensed = load (BinaryData::BarlowCondensedSemiBold_ttf,   BinaryData::BarlowCondensedSemiBold_ttfSize);
        juce::Typeface::Ptr condensedBold = load (BinaryData::BarlowCondensedBold_ttf,   BinaryData::BarlowCondensedBold_ttfSize);
        juce::Typeface::Ptr extraBold = load (BinaryData::FigtreeExtraBold_ttf,          BinaryData::FigtreeExtraBold_ttfSize);
        juce::Typeface::Ptr mono      = load (BinaryData::IBMPlexMonoRegular_ttf,        BinaryData::IBMPlexMonoRegular_ttfSize);
        juce::Typeface::Ptr monoBold  = load (BinaryData::IBMPlexMonoMedium_ttf,         BinaryData::IBMPlexMonoMedium_ttfSize);

        static juce::Typeface::Ptr load (const void* data, int size)
        {
            return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
        }
    };

    Typefaces& get()
    {
        static Typefaces t;
        return t;
    }

    juce::Font make (const juce::Typeface::Ptr& tf, float height)
    {
        // FontOptions is JUCE 8's way to build a Font from a specific typeface.
        return juce::Font (juce::FontOptions (tf).withHeight (height));
    }
}

juce::Font ui (float h)          { return make (get().regular, h); }
juce::Font uiMedium (float h)    { return make (get().medium, h); }
juce::Font uiSemiBold (float h)  { return make (get().semiBold, h); }
juce::Font uiBold (float h)      { return make (get().semiBold, h); }
juce::Font uiExtraBold (float h) { return make (get().extraBold, h); }
juce::Font caption (float h)     { return make (get().condensed, h); }
juce::Font captionBold (float h) { return make (get().condensedBold, h); }
juce::Font mono (float h)        { return make (get().mono, h); }
juce::Font monoBold (float h)    { return make (get().monoBold, h); }

juce::Typeface::Ptr defaultTypeface()   { return get().regular; }

} // namespace lithium::fonts
