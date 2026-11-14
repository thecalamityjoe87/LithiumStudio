#include "ui/theme/Theme.h"

namespace lithium
{

namespace
{
    // juce::Colour (0xAARRGGBB): alpha first, then red, green, blue.
    juce::Colour hex (juce::uint32 argb)   { return juce::Colour (argb); }

    const juce::Colour none = juce::Colours::transparentBlack;

    //==========================================================================
    ThemePalette makeDark()
    {
        // The Lithium family's dark palette (shared with Lithium Drums):
        // blue-black surfaces, one cyan accent, the kick red for warnings.
        //   background #0e1318  panel #141b22  raised #18212a  raisedHigh #1d2833
        //   border #22303b  borderStrong #2a3946  text #e6edf2  muted #93a3b1
        //   accent #3cc4d6
        ThemePalette p;

        // Surfaces
        p.windowBg            = hex (0xff0e1318);
        p.titleBarBg          = hex (0xff0a0e12);
        p.titleBarBorder      = hex (0xff22303b);
        p.toolbarBg           = hex (0xff141b22);
        p.toolbarBorder       = hex (0xff22303b);
        p.transportWell       = hex (0xff0a0e12);
        p.transportWellBorder = hex (0xff22303b);
        p.librarySidebarBg    = hex (0xff141b22);
        p.tracksBarBg         = hex (0xff161e26);
        p.sectionsRowBg       = hex (0xff161e26);
        p.trackColumnBg       = hex (0xff161e26);
        p.trackRowLine        = hex (0xff22303b);
        p.rulerBg             = hex (0xff141b22);
        p.sectionsStripBg     = hex (0xff0e1318);
        p.bottomPanelBg       = hex (0xff141b22);
        p.bottomPanelBorder   = hex (0xff22303b);
        p.inspectorBg         = hex (0xff11171e);
        p.divider             = hex (0xff22303b);
        p.dialogBg            = hex (0xff141b22);
        p.dialogSidebarBg     = hex (0xff11171e);
        p.popupBg             = hex (0xff141b22);

        p.wellBg              = hex (0xff0a0e12);
        p.cardBg              = hex (0xff18212a);
        p.panelBoxBg          = hex (0xff18212a);
        p.raised              = hex (0xff18212a);
        p.controlFill         = hex (0xff1d2833);
        p.selectedRow         = hex (0xff1d2833);
        p.tileBg              = hex (0xff18212a);
        p.tileBorder          = hex (0xff22303b);
        p.rowHover            = hex (0xff1d2833).withAlpha (0.7f);
        p.rowPressed          = hex (0xff1d2833);

        p.neutralActive       = hex (0xff223040);
        p.neutralActiveBorder = hex (0xff2a3946);
        p.selectedBorder      = hex (0xff4a6072);
        p.selectedTileShadow  = none;
        p.highlight           = hex (0xffe6edf2);

        p.selectedBg          = hex (0xff223040);
        p.selectedOutline     = hex (0xffe6edf2);
        p.selectedInset       = hex (0xff3a4c5b);
        p.trackTileAlpha      = 0.16f;

        p.border              = hex (0xff2a3946);
        p.borderSoft          = hex (0xff22303b);
        p.borderStrong        = hex (0xff34475a);
        p.outline             = hex (0xff3a4c5b);

        p.buttonFill          = hex (0xff1d2833);
        p.buttonBorder        = hex (0xff2a3946);
        p.buttonIcon          = hex (0xff93a3b1);
        p.raisedBorder        = hex (0xff22303b);   // Lithium's buttons have a hairline border
        p.armBorder           = hex (0xff2a3946);

        p.segmentWell         = hex (0xff0f151b);
        p.segmentWellBorder   = hex (0xff2a3946);
        p.segmentSelected     = hex (0xff223040);
        p.segmentShadow       = none;

        p.inputBg             = hex (0xff0a0e12);
        p.inputBorder         = hex (0xff22303b);
        p.field               = hex (0xff18212a);

        p.lcdBg               = hex (0xff070a0d);
        p.lcdBorder           = hex (0xff18212a);
        p.lcdLabel            = hex (0xff93a3b1);
        p.lcdValue            = hex (0xff3cc4d6);   // the playhead's cyan
        p.lcdInTune           = hex (0xff8bd17c);

        // Text
        p.text                = hex (0xffe6edf2);
        p.textSecondary       = hex (0xff93a3b1);
        p.textSoft            = hex (0xffb9c6d1);
        p.textMenu            = hex (0xffb9c6d1);
        p.textOnAccent        = hex (0xff0e1318);
        p.textOnColour        = hex (0xffffffff);
        p.textDisabled        = hex (0xff5d6e7c);

        // Sliders and knobs
        p.sliderTrack         = hex (0xff1d2833);
        p.sliderFill          = hex (0xff93a3b1);
        p.sliderThumb         = hex (0xffe6edf2);
        p.thumbBorder         = none;
        p.thumbShadow         = none;
        p.volumeSliderTrack   = hex (0xff1d2833);
        // The console fader: a dark gunmetal cap.
        p.faderSlot           = hex (0xff05080b);
        p.faderCapLight       = hex (0xff56636f);
        p.faderCapHigh        = hex (0xff3b4752);
        p.faderCapMid         = hex (0xff2c3640);
        p.faderCapGroove      = hex (0xff1a2129);
        p.faderCapBottom      = hex (0xff242d36);
        p.faderCapBorder      = hex (0xff05080b);
        p.faderCapHighlight   = juce::Colours::white.withAlpha (0.22f);
        p.faderCapRibs        = juce::Colours::black.withAlpha (0.38f);
        p.faderCapShadow      = juce::Colours::black.withAlpha (0.55f);
        p.faderCapLine        = hex (0xffe6edf2);

        // The machined knob: dark turned metal.
        p.knobTrack           = hex (0xff0a0e12);
        p.knobTicks           = hex (0xff3a4c5b);
        p.knobShadow          = juce::Colours::black.withAlpha (0.5f);
        p.knobSkirtTop        = hex (0xff2e3c49);
        p.knobSkirtBottom     = hex (0xff0b1015);
        p.knobSkirtEdge       = hex (0xff06090c);
        p.knobCapLight        = hex (0xff7a8895);
        p.knobCapMid          = hex (0xff3e4a56);
        p.knobCapDark         = hex (0xff1b232b);
        p.knobCapEdge         = hex (0xff0a0e12);
        p.knobSheen           = juce::Colours::white.withAlpha (0.2f);
        p.knobRings           = juce::Colours::white.withAlpha (0.07f);
        p.knobGroove          = hex (0xff0b1015);
        p.knobGrooveHighlight = juce::Colours::white.withAlpha (0.22f);
        p.knobPointer         = hex (0xffe6edf2);
        p.knobGlow            = 0.3f;
        p.faderGlow           = 0.3f;

        p.smartCardBg         = hex (0xff18212a);
        p.smartCardBorder     = hex (0xff22303b);
        p.smartCardShadow     = none;

        // The on-screen keyboard
        p.keyboardTray        = hex (0xff070a0d);
        p.keyboardTrayBorder  = none;
        p.pianoWhite          = hex (0xffdfe7ed);
        p.pianoWhiteDown      = hex (0xff93a3b1);
        p.pianoKeyDivider     = hex (0xfff4f7f9);   // JUCE's white key background, showing between the keys
        p.pianoBlack          = hex (0xff141b22);
        p.pianoBlackDown      = hex (0xff5d6e7c);
        p.pianoBlackLip       = hex (0xff2a3946);
        p.pianoLabel          = hex (0xff4d5d6a);
        p.rollKeyWhite        = hex (0xffb9c6d1);
        p.rollKeyBlack        = hex (0xff0a0e12);

        p.previewFill         = hex (0xffe6edf2);
        p.previewIcon         = hex (0xff0e1318);

        // Accent and status
        p.accent              = hex (0xff3cc4d6);
        p.accentLight         = p.accent.brighter (0.15f);
        p.accentBorder        = none;
        p.record              = hex (0xfff07167);
        p.warning             = hex (0xffe7d27c);
        p.solo                = hex (0xffe7d27c);
        p.meterGreen          = hex (0xff8bd17c);
        p.meterYellow         = hex (0xffe7d27c);
        p.meterRed            = hex (0xfff07167);
        p.effectOn            = hex (0xff3cc4d6);
        p.realtimeBadgeText   = hex (0xff8bd17c);
        p.realtimeBadgeBg     = hex (0xff8bd17c).withAlpha (0.18f);

        p.tintLight           = hex (0xffffffff);
        p.tintDark            = hex (0xff000000);

        p.pedalboardFloor     = hex (0xff0a0e12);
        p.pedalboardCable     = hex (0xff3a4c5b);   // lighter than the floor, so it shows

        p.scrim               = juce::Colours::black.withAlpha (0.55f);
        p.scrimStrong         = juce::Colours::black.withAlpha (0.6f);    // session chooser
        p.scrimLight          = juce::Colours::black.withAlpha (0.45f);   // "Saving…" overlay

        // A juce::DropShadow is colour + blur radius + offset; drawForRectangle
        // or drawForPath paints it under a shape.
        p.dialogShadow        = { juce::Colours::black.withAlpha (0.6f), 30, { 0, 8 } };
        p.popoverShadow       = { juce::Colours::black.withAlpha (0.5f), 14, { 0, 4 } };

        // Instrument families, in the order of tokens::Family: the hues of
        // Lithium Drums' group and FX colours, a little more saturated than
        // Drums' pad colours so tracks and regions still stand out, but
        // softened so they sit with the blue-black surfaces. None is the
        // accent cyan.
        p.familyColours = { hex (0xffd968e0),    // drums: magenta
                            hex (0xfff98d4e),    // audio
                            hex (0xfff57cb3),    // strings
                            hex (0xff6fd672),    // bass
                            hex (0xff5f9bf0),    // keys
                            hex (0xffa47eff),    // synths
                            hex (0xfff2cf55),    // guitar
                            hex (0xffb3db5a),    // woodwinds
                            hex (0xffd9a24f),    // brass
                            hex (0xff42d4ae),    // loops
                            hex (0xfff65d63),    // plugins: red
                            hex (0xff93a3b1) };  // master: neutral grey
        p.inkColours = p.familyColours;          // dark mode: ink is the family colour
        p.knobArcColours = p.familyColours;

        using F = tokens::Family;
        p.eqBands = { p.family (F::keys), p.family (F::bass), p.family (F::guitar), p.family (F::synths) };

        // Automation curves: a shade lighter than the track colours, to stand
        // out over regions.
        p.automationVolume    = hex (0xff9cc0f0);   // blue
        p.automationPan       = hex (0xffa6e09a);   // green
        p.automationReverb    = hex (0xffcbbcff);   // purple
        p.automationDelay     = hex (0xfff5bc89);   // orange
        p.automationSound     = hex (0xfff0e2a0);   // yellow: the Sound tab's knobs
        p.automationPlugin    = hex (0xffefaccb);   // pink: plug-in parameters
        p.automated           = hex (0xff9cc0f0);

        // The arrangement
        auto& a = p.arrangement;
        a.laneBg              = hex (0xff0e1318);
        a.laneDivider         = hex (0xff18212a);
        a.gridBar             = hex (0xffe6edf2).withAlpha (0.05f);
        a.gridBeat            = hex (0xffe6edf2).withAlpha (0.025f);
        a.selectedLane        = hex (0xffe6edf2).withAlpha (0.045f);
        a.text                = hex (0xffe6edf2);
        a.textSecondary       = hex (0xff93a3b1);
        a.outline             = hex (0xff2a3946);
        a.highlight           = hex (0xffe6edf2);
        a.selectionOutline    = hex (0xffe6edf2);
        a.tintLight           = hex (0xffffffff);
        a.tintDark            = hex (0xff000000);
        a.bubble              = hex (0xff1d2833);
        a.tempoCurve          = hex (0xff93a3b1);
        a.chipFill            = hex (0xff1d2833);
        a.chipLitFill         = hex (0xff223040);
        a.chipBorder          = hex (0xff2a3946);
        a.playhead            = hex (0xff3cc4d6);
        a.cycle               = hex (0xff3cc4d6);
        a.record              = hex (0xfff07167);
        a.scrollbar           = hex (0xff2a3946);
        a.regionTextDark      = hex (0xff0e1318);
        a.regionTextLight     = hex (0xffffffff);
        a.strip               = hex (0xff11171e);

        return p;
    }

    //==========================================================================
    ThemePalette makeLight()
    {
        // Start from dark for what both themes share (the arrangement, the
        // family colours, the meters, the automation colours), then set every
        // chrome colour. Each area gets its own grey so the panels separate;
        // white is kept for what should stand out: buttons, the selected
        // track header and library category, the Smart Controls cards, keys.
        auto p = makeDark();
        p.isLight = true;

        // Surfaces
        p.windowBg            = hex (0xffe8ebef);
        p.titleBarBg          = hex (0xffd5d9e0);
        p.titleBarBorder      = hex (0xffc3c8d0);
        p.toolbarBg           = hex (0xffe1e4e9);
        p.toolbarBorder       = hex (0xffc9ced6);
        p.transportWell       = hex (0xffd3d7de);
        p.transportWellBorder = hex (0xffc3c8d0);
        p.librarySidebarBg    = hex (0xffe8ebef);
        p.tracksBarBg         = hex (0xffe1e4e9);
        p.sectionsRowBg       = hex (0xffe8ebef);
        p.trackColumnBg       = hex (0xffeef0f3);
        p.trackRowLine        = hex (0xffdde1e6);
        p.rulerBg             = hex (0xffe1e4e9);
        p.sectionsStripBg     = hex (0xffe8ebef);
        p.bottomPanelBg       = hex (0xffe1e4e9);
        p.bottomPanelBorder   = hex (0xffc3c8d0);
        p.inspectorBg         = hex (0xffe8ebef);
        p.divider             = hex (0xffcfd4db);
        p.dialogBg            = hex (0xfff3f4f7);
        p.dialogSidebarBg     = hex (0xffe8ebef);
        p.popupBg             = hex (0xffffffff);

        p.wellBg              = hex (0xffd3d7de);
        p.pedalboardFloor     = hex (0xffd3d8e0);
        p.pedalboardCable     = hex (0xff959ca7);
        p.cardBg              = hex (0xffffffff);
        p.panelBoxBg          = hex (0xffeceef2);
        p.raised              = hex (0xffffffff);
        p.controlFill         = hex (0xffe2e5ea);
        p.selectedRow         = hex (0xffe6e9ee);
        p.tileBg              = hex (0xfff3f4f7);
        p.tileBorder          = hex (0xffd5d9df);
        p.rowHover            = hex (0xffd9dde3);
        p.rowPressed          = hex (0xffd0d5dc);

        p.neutralActive       = hex (0xffe2e5ea);
        p.neutralActiveBorder = hex (0xffc6cbd3);
        p.selectedBorder      = hex (0xff6f7783);
        p.selectedTileShadow  = juce::Colours::black.withAlpha (0.1f);
        p.highlight           = hex (0xff1a1d22);

        p.selectedBg          = hex (0xffffffff);
        p.selectedOutline     = hex (0xff6f7783);
        p.selectedInset       = hex (0xffaeb5c0);
        p.trackTileAlpha      = 0.2f;

        p.border              = hex (0xffd5d9df);
        p.borderSoft          = hex (0xffdde1e6);
        p.borderStrong        = hex (0xffc3c8d0);
        p.outline             = hex (0xffaeb5c0);

        p.buttonFill          = hex (0xffffffff);
        p.buttonBorder        = hex (0xffd5d9df);
        p.buttonIcon          = hex (0xff1a1d22);
        p.raisedBorder        = hex (0xffd5d9df);
        p.armBorder           = hex (0xffc6cbd3);

        p.segmentWell         = hex (0xffd3d7de);
        p.segmentWellBorder   = hex (0xffc3c8d0);
        p.segmentSelected     = hex (0xffffffff);
        p.segmentShadow       = juce::Colours::black.withAlpha (0.1f);   // 0 1px 2px

        p.inputBg             = hex (0xffffffff);
        p.inputBorder         = hex (0xffd5d9df);
        p.field               = hex (0xfff0f2f5);

        p.lcdBg               = hex (0xffffffff);
        p.lcdBorder           = hex (0xffd5d9df);
        p.lcdLabel            = hex (0xff5f6672);
        p.lcdValue            = hex (0xff0e7f8f);   // the playhead's cyan, deeper: #3cc4d6 is too pale on white
        p.lcdInTune           = hex (0xff1f8f4e);

        // Text
        p.text                = hex (0xff1a1d22);
        p.textSecondary       = hex (0xff5f6672);
        p.textSoft            = hex (0xff4a515c);
        p.textMenu            = hex (0xff1a1d22);
        p.textDisabled        = hex (0xffa4aab4);

        // Sliders and knobs
        p.sliderTrack         = hex (0xffd5d9df);
        p.sliderFill          = hex (0xff1a1d22);
        p.sliderThumb         = hex (0xffffffff);
        p.thumbBorder         = hex (0xffc6cbd3);
        p.thumbShadow         = juce::Colours::black.withAlpha (0.15f);
        p.volumeSliderTrack   = hex (0xffdfe2e7);
        // The console fader: a brushed aluminium cap.
        p.faderSlot           = hex (0xffb9bfc8);
        p.faderCapLight       = hex (0xffffffff);
        p.faderCapHigh        = hex (0xffeef1f4);
        p.faderCapMid         = hex (0xffdde2e7);
        p.faderCapGroove      = hex (0xffb5bdc6);
        p.faderCapBottom      = hex (0xffcfd5dc);
        p.faderCapBorder      = hex (0xff8f98a2);
        p.faderCapHighlight   = hex (0xffffffff);
        p.faderCapRibs        = juce::Colours::black.withAlpha (0.13f);
        p.faderCapShadow      = hex (0xff141b22).withAlpha (0.25f);
        p.faderCapLine        = hex (0xff1a1d22);

        // The machined knob: silver turned metal.
        p.knobTrack           = hex (0xffd3d7de);
        p.knobTicks           = hex (0xff9aa2ad);
        p.knobShadow          = hex (0xff1a1d22).withAlpha (0.2f);
        p.knobSkirtTop        = hex (0xffe4e8ec);
        p.knobSkirtBottom     = hex (0xff8e98a3);
        p.knobSkirtEdge       = hex (0xff7d8792);
        p.knobCapLight        = hex (0xffffffff);
        p.knobCapMid          = hex (0xffd3d9df);
        p.knobCapDark         = hex (0xff97a1ab);
        p.knobCapEdge         = hex (0xff7d8792);
        p.knobSheen           = juce::Colours::white.withAlpha (0.6f);
        p.knobRings           = hex (0xff1a1d22).withAlpha (0.07f);
        p.knobGroove          = hex (0xff5f6672);
        p.knobGrooveHighlight = juce::Colours::white.withAlpha (0.8f);
        p.knobPointer         = hex (0xff1a1d22);
        p.knobGlow            = 0.18f;
        p.faderGlow           = 0.0f;

        p.smartCardBg         = hex (0xffffffff);
        p.smartCardBorder     = hex (0xffe2e5ea);
        p.smartCardShadow     = juce::Colours::black.withAlpha (0.06f);

        // The on-screen keyboard
        p.keyboardTray        = hex (0xffc9ced6);
        p.keyboardTrayBorder  = hex (0xffb7bec8);
        p.pianoWhite          = hex (0xffffffff);
        p.pianoWhiteDown      = hex (0xffc6cbd3);
        p.pianoKeyDivider     = hex (0xffd5d9df);
        p.pianoBlack          = hex (0xff1a1d22);
        p.pianoBlackDown      = hex (0xff4a515c);
        p.pianoBlackLip       = hex (0xff3a3f47);
        p.pianoLabel          = hex (0xff5f6672);
        p.rollKeyWhite        = hex (0xffffffff);
        p.rollKeyBlack        = hex (0xff1a1d22);

        p.previewFill         = hex (0xff1a1d22);
        p.previewIcon         = hex (0xffffffff);

        // Accent and status. Accent fills stay cyan, with a deeper border.
        p.accentBorder        = hex (0xff2aa3b4);
        p.record              = hex (0xffd9534a);
        p.warning             = hex (0xffb07800);   // the dark yellow is unreadable on light grey
        p.realtimeBadgeText   = hex (0xff1a8a4b);
        p.realtimeBadgeBg     = juce::Colour (31, 154, 85).withAlpha (0.12f);
        p.automated           = hex (0xff2f7fd6);   // a deeper blue for automated controls on the chrome

        // Hover and shading flip: on a light surface a hover darkens.
        p.tintLight           = hex (0xff000000);
        p.tintDark            = hex (0xffffffff);

        p.scrim               = juce::Colours::black.withAlpha (0.3f);
        p.scrimStrong         = juce::Colours::black.withAlpha (0.35f);
        p.scrimLight          = juce::Colours::black.withAlpha (0.25f);
        p.dialogShadow        = { juce::Colours::black.withAlpha (0.18f), 30, { 0, 8 } };
        p.popoverShadow       = { juce::Colours::black.withAlpha (0.15f), 14, { 0, 4 } };

        // Ink: deeper family shades for icons and arcs on the light chrome.
        for (size_t i = 0; i < ThemePalette::numFamilies; ++i)
            p.inkColours[i] = p.familyColours[i].withMultipliedSaturation (1.15f).withMultipliedBrightness (0.62f);

        // The arrangement stays dark, a little lighter than in dark mode.
        auto& a = p.arrangement;
        a.laneBg              = hex (0xff3e4249);
        a.laneDivider         = hex (0xff33363c);
        a.gridBar             = juce::Colours::white.withAlpha (0.08f);
        a.gridBeat            = juce::Colours::white.withAlpha (0.03f);
        a.selectedLane        = juce::Colours::white.withAlpha (0.05f);
        a.strip               = hex (0xff33363c);
        a.playhead            = hex (0xff3cc4d6);
        a.cycle               = hex (0xff3cc4d6);
        a.scrollbar           = hex (0xff5a5e66);

        return p;
    }

    const ThemePalette* active = nullptr;
}

const ThemePalette& darkPalette()
{
    // A function-local static is built the first time the function runs, so
    // the palette never depends on the order other files' globals are made.
    static const ThemePalette palette = makeDark();
    return palette;
}

const ThemePalette& lightPalette()
{
    static const ThemePalette palette = makeLight();
    return palette;
}

const ThemePalette& theme()
{
    return active != nullptr ? *active : darkPalette();
}

void setActivePalette (const ThemePalette& p)
{
    active = &p;
}

} // namespace lithium
