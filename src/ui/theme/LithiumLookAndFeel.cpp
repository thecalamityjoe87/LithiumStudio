#include "ui/theme/LithiumLookAndFeel.h"
#include "ui/theme/Fonts.h"
#include "ui/theme/Tokens.h"
#include <tuple>

namespace lithium
{

using namespace tokens;

LithiumLookAndFeel::LithiumLookAndFeel()
{
    applyThemeColours();

    // Makes the bundled Barlow the fallback for any Font that doesn't name a typeface.
    setDefaultSansSerifTypeface (fonts::defaultTypeface());
}

void LithiumLookAndFeel::applyThemeColours()
{
    // LookAndFeel_V4 is driven by a ColourScheme; set ours so any widget we
    // don't customise below still matches the palette.
    setColourScheme ({ theme().windowBg,        // windowBackground
                       theme().toolbarBg,       // widgetBackground
                       theme().wellBg,          // menuBackground
                       theme().border,          // outline
                       theme().text,            // defaultText
                       theme().raised,          // defaultFill
                       theme().text,            // highlightedText
                       theme().neutralActive,   // highlightedFill (not the accent: that only means "on")
                       theme().text });         // menuText

    setColour (juce::ResizableWindow::backgroundColourId, theme().windowBg);
    setColour (juce::DocumentWindow::textColourId, theme().text);

    setColour (juce::TextButton::buttonColourId, theme().raised);
    setColour (juce::TextButton::buttonOnColourId, theme().neutralActive);
    setColour (juce::TextButton::textColourOffId, theme().text);
    setColour (juce::TextButton::textColourOnId, theme().text);

    setColour (juce::Label::textColourId, theme().text);
    setColour (juce::Label::textWhenEditingColourId, theme().text);
    setColour (juce::Label::backgroundWhenEditingColourId, theme().raised);
    setColour (juce::Label::outlineWhenEditingColourId, theme().highlight);

    // Slider fills and knob arcs default to neutral grey; track controls set
    // their own (the track colour) per slider.
    setColour (juce::Slider::trackColourId, theme().sliderFill);
    setColour (juce::Slider::backgroundColourId, theme().sliderTrack);
    setColour (juce::Slider::thumbColourId, theme().sliderThumb);
    setColour (knobArcColourId, theme().textSecondary);

    setColour (juce::TextEditor::backgroundColourId, theme().inputBg);
    setColour (juce::TextEditor::textColourId, theme().text);
    setColour (juce::TextEditor::highlightColourId, theme().highlight.withAlpha (0.25f));
    setColour (juce::TextEditor::outlineColourId, theme().inputBorder);
    setColour (juce::TextEditor::focusedOutlineColourId, theme().selectedBorder);
    setColour (juce::CaretComponent::caretColourId, theme().text);

    setColour (juce::PopupMenu::backgroundColourId, theme().popupBg);
    setColour (juce::PopupMenu::textColourId, theme().text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, theme().neutralActive);
    setColour (juce::PopupMenu::highlightedTextColourId, theme().text);
    setColour (juce::PopupMenu::headerTextColourId, theme().textSecondary);

    setColour (juce::ScrollBar::thumbColourId, theme().borderStrong);
    setColour (juce::TooltipWindow::backgroundColourId, theme().raised);
    setColour (juce::TooltipWindow::textColourId, theme().text);
    setColour (juce::TooltipWindow::outlineColourId, theme().border);

    // The value bubble a slider shows while it's dragged (see
    // Slider::setPopupDisplayEnabled). Its text uses the tooltip's colour.
    setColour (juce::BubbleComponent::backgroundColourId, theme().raised);
    setColour (juce::BubbleComponent::outlineColourId, theme().border);

    setColour (juce::ComboBox::backgroundColourId, theme().raised);
    setColour (juce::ComboBox::outlineColourId, theme().border);
    setColour (juce::ComboBox::textColourId, theme().text);
    setColour (juce::ComboBox::arrowColourId, theme().textSecondary);
    setColour (juce::ListBox::backgroundColourId, theme().wellBg);
    setColour (juce::ListBox::outlineColourId, theme().borderSoft);
    setColour (juce::ToggleButton::textColourId, theme().text);
    setColour (juce::ToggleButton::tickColourId, theme().accent);
    setColour (juce::AlertWindow::backgroundColourId, theme().dialogBg);
    setColour (juce::AlertWindow::textColourId, theme().text);
    setColour (juce::AlertWindow::outlineColourId, theme().border);
}

//==============================================================================
juce::Typeface::Ptr LithiumLookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    // Fonts created via lithium::fonts already carry their typeface. Anything
    // else (default JUCE widgets) falls back to Barlow.
    if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
        return fonts::defaultTypeface();

    return juce::LookAndFeel_V4::getTypefaceForFont (f);
}

juce::Font LithiumLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return fonts::uiSemiBold (juce::jmin (textSize::lg, (float) buttonHeight * 0.45f));
}

juce::Font LithiumLookAndFeel::getLabelFont (juce::Label& l)
{
    return l.getFont();
}

juce::Font LithiumLookAndFeel::getPopupMenuFont()
{
    return fonts::ui (textSize::lg);
}

void LithiumLookAndFeel::getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator, int standardHeight,
                                                   int& idealWidth, int& idealHeight)
{
    // Roomier menu items than JUCE's default (~18 px): easier to hit.
    juce::LookAndFeel_V4::getIdealPopupMenuItemSize (text, isSeparator, isSeparator ? standardHeight : juce::jmax (standardHeight, 30),
                                                     idealWidth, idealHeight);
    idealWidth += 16;
}

juce::Font LithiumLookAndFeel::getMenuBarFont (juce::MenuBarComponent&, int, const juce::String&)
{
    return fonts::ui (textSize::md);
}

//==============================================================================
void LithiumLookAndFeel::styleInlineEditor (juce::TextEditor& editor)
{
    // A flag rather than colours, so the colours are read from the theme
    // each time the field paints.
    editor.getProperties().set ("inline", true);
    editor.repaint();
}

void LithiumLookAndFeel::setFamilyColour (juce::Slider& s, Family f, FamilyShade shade)
{
    // getProperties() is a free-form NamedValueSet every component carries.
    // A juce::var holds an int, not our enums, hence the casts. set() says
    // whether the value changed: views call this on every refresh (many a
    // second while automation plays), so only repaint when it did.
    const bool familyChanged = s.getProperties().set ("family", static_cast<int> (f));
    const bool shadeChanged = s.getProperties().set ("familyShade", static_cast<int> (shade));

    if (familyChanged || shadeChanged)
        s.repaint();
}

juce::Colour LithiumLookAndFeel::valueColourFor (juce::Slider& s, int colourId)
{
    const auto& family = s.getProperties()["family"];   // a void var if it was never set

    if (family.isVoid())
        return s.findColour (colourId);

    const auto f = static_cast<Family> ((int) family);
    const auto shade = static_cast<FamilyShade> ((int) s.getProperties()["familyShade"]);
    return shade == FamilyShade::knobArc ? theme().knobArc (f) : theme().ink (f);
}

void LithiumLookAndFeel::drawFocusRing (juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius)
{
    g.setColour (theme().highlight);
    g.drawRoundedRectangle (bounds.reduced (1.0f), cornerRadius, 2.0f);
}

void LithiumLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bg,
                                              bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    auto fill = b.getToggleState() ? b.findColour (juce::TextButton::buttonOnColourId) : bg;

    if (down)       fill = fill.brighter (0.12f);
    else if (over)  fill = fill.brighter (0.06f);

    g.setColour (fill);
    g.fillRoundedRectangle (r, size::radius);

    g.setColour (theme().border);
    g.drawRoundedRectangle (r, size::radius, 1.0f);

    if (b.hasKeyboardFocus (false))
        drawFocusRing (g, r, size::radius);
}

//==============================================================================
namespace
{
    // The console fader's slot: sunk into the panel, so darker at its edges
    // than down its middle, with a faint highlight along its lower lip.
    void drawSlot (juce::Graphics& g, juce::Rectangle<float> slot)
    {
        const auto radius = juce::jmin (slot.getWidth(), slot.getHeight()) * 0.5f;
        const bool vertical = slot.getHeight() > slot.getWidth();
        g.setColour (theme().faderCapHighlight.withMultipliedAlpha (0.25f));
        g.fillRoundedRectangle (slot.translated (0.0f, 1.0f), radius);

        const auto edge = theme().faderSlot.darker (0.6f);
        juce::ColourGradient shade (edge, slot.getTopLeft(), edge, vertical ? slot.getTopRight() : slot.getBottomLeft(), false);
        shade.addColour (0.5, theme().faderSlot);
        g.setGradientFill (shade);
        g.fillRoundedRectangle (slot, radius);
    }
}

void LithiumLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float sliderPos, float, float,
                                          juce::Slider::SliderStyle style, juce::Slider& s)
{
    const bool vertical = style == juce::Slider::LinearVertical;

    // An automated control (see setAutomatedLook) has an amber thumb: the
    // automation moves it, not the user.
    const bool automated = s.getProperties()["automated"];
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    auto trackColour = valueColourFor (s, juce::Slider::trackColourId);

    if (vertical)
    {
        // The console fader: a sunken slot, a tick across it at unity gain
        // (the "unityValue" property, if set), and the ribbed cap. The cap is
        // as tall as JUCE keeps free at each end of the travel, so it never
        // leaves the slider's bounds.
        const auto slot = bounds.withSizeKeepingCentre (6.0f, bounds.getHeight() + 8.0f);
        drawSlot (g, slot);

        if (const auto& unity = s.getProperties()["unityValue"]; ! unity.isVoid())
        {
            const auto uy = (float) s.getPositionOfValue ((double) unity);
            g.setColour (theme().borderStrong);
            g.fillRect (juce::Rectangle<float> (slot.getX() - 6.0f, uy - 0.5f, slot.getWidth() + 12.0f, 1.0f));
        }

        const auto capHeight = 2.0f * (float) getSliderThumbRadius (s);
        const auto cap = juce::Rectangle<float> (juce::jmin (28.0f, bounds.getWidth()), capHeight).withCentre ({ slot.getCentreX(), sliderPos });
        drawConsoleCap (g, cap, true, s);
    }
    else
    {
        // Track header volume: thin track, filled in trackColourId, round thumb.
        // JUCE hands us the thumb's travel, which is inset by the thumb's
        // radius at each end; the track reaches past it to the slider's
        // edges, as far as the thumb itself goes.
        auto track = bounds.expanded ((float) getSliderThumbRadius (s), 0.0f);
        track = track.withSizeKeepingCentre (track.getWidth(), 4.0f);
        g.setColour (theme().sliderTrack);
        g.fillRoundedRectangle (track, size::radiusHair);

        g.setColour (trackColour);
        g.fillRoundedRectangle (track.withRight (sliderPos), size::radiusHair);

        // The thumb, with a border and soft shadow in light mode (both
        // transparent in dark).
        auto thumb = juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ sliderPos, track.getCentreY() });
        juce::Path disc;
        disc.addEllipse (thumb);
        juce::DropShadow (theme().thumbShadow, 3, { 0, 1 }).drawForPath (g, disc);
        g.setColour (automated ? theme().automated : theme().sliderThumb);
        g.fillEllipse (thumb);
        g.setColour (theme().thumbBorder);
        g.drawEllipse (thumb.reduced (0.5f), 1.0f);
    }

    if (s.hasKeyboardFocus (false))
        drawFocusRing (g, bounds.expanded (2.0f), size::radiusXs);
}

// A pedal's knob (see PedalView): a dark knob with a white pointer and the
// value arc in the pedal's lettering colour (the slider's knobArcColourId).
// Pedals look the same in both themes, so this uses fixed colours, not the
// theme's. The Amp's knobs (see AmpView) are drawn here too, with two more
// slider properties: "knobCap" ("chrome" or "cream" for a light cap with a
// dark pointer; anything else is the dark cap) and "capSize" (the cap's
// diameter in pixels).
static void drawPedalKnob (juce::Graphics& g, juce::Rectangle<float> area, float pos,
                           float startAngle, float endAngle, juce::Slider& s)
{
    const auto size = juce::jmin (area.getWidth(), area.getHeight());
    const auto centre = area.getCentre();
    const auto arcWidth = juce::jmax (2.0f, size * 0.09f);
    const auto radius = (size - arcWidth) * 0.5f - 0.5f;
    const auto valueAngle = startAngle + pos * (endAngle - startAngle);
    const auto arcColour = s.findColour (LithiumLookAndFeel::knobArcColourId);
    const juce::PathStrokeType stroke (arcWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path ring;
    ring.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (arcColour.withAlpha (0.22f));
    g.strokePath (ring, stroke);

    if (valueAngle - startAngle > 0.02f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, valueAngle, true);
        g.setColour (arcColour);
        g.strokePath (value, stroke);
    }

    // The knob: a small shadow under it, then a body lit from the top left.
    const auto& properties = s.getProperties();
    const auto bodyRadius = properties.contains ("capSize") ? (float) properties["capSize"] * 0.5f
                                                            : radius - arcWidth * 0.5f - 2.0f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);
    juce::Path bodyPath;
    bodyPath.addEllipse (body);
    juce::DropShadow (juce::Colours::black.withAlpha (0.4f), 4, { 0, 2 }).drawForPath (g, bodyPath);

    // The cap's colours: lit spot, middle, edge.
    const auto cap = properties["knobCap"].toString();
    const bool lightCap = cap == "chrome" || cap == "cream";
    const auto [lit, middle, edge] = cap == "chrome" ? std::make_tuple (juce::Colour (0xfffbfcfd), juce::Colour (0xffc7ccd3), juce::Colour (0xff7c838d))
                                   : cap == "cream"  ? std::make_tuple (juce::Colour (0xfffbf4e3), juce::Colour (0xffeadcbc), juce::Colour (0xffb7a17a))
                                                     : std::make_tuple (juce::Colour (0xff42454c), juce::Colour (0xff2a2c31), juce::Colour (0xff121316));

    juce::ColourGradient shading (lit, body.getX() + body.getWidth() * 0.3f, body.getY() + (lightCap ? body.getHeight() * 0.25f : 0.0f),
                                  edge, body.getCentreX(), body.getBottom(), true);
    shading.addColour (0.5, middle);
    g.setGradientFill (shading);
    g.fillEllipse (body);
    g.setColour (juce::Colours::black.withAlpha (lightCap ? 0.35f : 0.6f));
    g.drawEllipse (body, 1.0f);

    g.setColour (lightCap ? juce::Colour (0xff1b1c1f) : juce::Colours::white);
    g.drawLine ({ centre.getPointOnCircumference (bodyRadius * 0.3f, valueAngle),
                  centre.getPointOnCircumference (bodyRadius * 0.85f, valueAngle) }, juce::jmax (1.8f, size * 0.07f));

    if (s.hasKeyboardFocus (false))
        LithiumLookAndFeel::drawFocusRing (g, juce::Rectangle<float> (size, size).withCentre (centre).expanded (2.0f), size * 0.5f);
}

void LithiumLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float pos, float startAngle, float endAngle, juce::Slider& s)
{
    // A pedal's knob, on the pedalboard. Set with
    // slider.getProperties().set ("pedal", true).
    if ((bool) s.getProperties()["pedal"])
    {
        drawPedalKnob (g, juce::Rectangle<int> (x, y, w, h).toFloat(), pos, startAngle, endAngle, s);
        return;
    }

    // Every other knob is the machined knob: a small one (e.g. a track
    // header's pan knob, set with slider.getProperties().set ("compact",
    // true)) in its compact form, filling its bounds; the big knobs keep a
    // little margin inside theirs.
    const auto area = juce::Rectangle<int> (x, y, w, h).toFloat();
    const bool compact = s.getProperties()["compact"];
    drawMachinedKnob (g, compact ? area : area.reduced (4.0f), pos, startAngle, endAngle, s, compact);
}

void LithiumLookAndFeel::drawMachinedKnob (juce::Graphics& g, juce::Rectangle<float> area, float pos,
                                           float startAngle, float endAngle, juce::Slider& s, bool compact)
{
    // Drawn in units of a hundredth of the knob's size (u), so it scales.
    // Outside in: the tick scale (big knobs only), the value arc in its
    // groove, the skirt with its shadow, the turned-metal cap with two
    // bands of sheen, and the pointer. The shading is fixed (lit from the
    // top left); only the arc and pointer move with the value.
    const auto size = juce::jmin (area.getWidth(), area.getHeight());
    compact = compact || size < 32.0f;   // too small for the ticks and the engraving
    const auto u = size / 100.0f;
    const auto c = area.getCentre();
    const auto enabled = s.isEnabled();
    const bool automated = s.getProperties()["automated"];
    const auto angle = startAngle + pos * (endAngle - startAngle);

    // A "bipolar" knob (e.g. pan) draws its arc out from 12 o'clock. Set with
    // slider.getProperties().set ("bipolar", true).
    const bool bipolar = s.getProperties()["bipolar"];
    const auto fromAngle = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

    auto circle = [&] (float radius, float dy = 0.0f)
    {
        return juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c.translated (0.0f, dy));
    };
    auto arc = [&] (float radius, float a0, float a1)
    {
        juce::Path p;
        p.addCentredArc (c.x, c.y, radius, radius, 0.0f, a0, a1, true);
        return p;
    };
    auto roundStroke = [] (float width) { return juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded); };

    if (! compact)
    {
        juce::Path ticks;

        for (int i = 0; i <= 10; ++i)
        {
            const auto a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
            ticks.startNewSubPath (c.getPointOnCircumference (45.5f * u, a));
            ticks.lineTo (c.getPointOnCircumference (49.5f * u, a));
        }

        g.setColour (theme().knobTicks);
        g.strokePath (ticks, roundStroke (juce::jmax (1.0f, 1.6f * u)));
    }

    // The value arc, in the knob's colour (knobArcColourId or its family),
    // with a soft glow in dark mode; in the "automated" colour while
    // automation moves it (see setAutomatedLook).
    const auto arcRadius = (compact ? 44.0f : 40.0f) * u;
    const auto arcWidth = juce::jmax (2.0f, (compact ? 7.0f : 4.5f) * u);
    g.setColour (theme().knobTrack);
    g.strokePath (arc (arcRadius, startAngle, endAngle), roundStroke (arcWidth));

    if (std::abs (angle - fromAngle) > 0.001f)
    {
        const auto value = arc (arcRadius, juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle));
        auto colour = automated ? theme().automated : valueColourFor (s, knobArcColourId);

        if (! enabled)
            colour = colour.withMultipliedAlpha (0.35f);
        else if (! compact && theme().knobGlow > 0.0f)
        {
            g.setColour (colour.withMultipliedAlpha (theme().knobGlow));
            g.strokePath (value, roundStroke (arcWidth * 2.2f));
        }

        g.setColour (colour);
        g.strokePath (value, roundStroke (arcWidth));
    }

    // The skirt, on its shadow.
    const auto skirt = circle ((compact ? 31.0f : 32.0f) * u);
    g.setColour (theme().knobShadow);
    g.fillEllipse (circle (skirt.getWidth() * 0.5f, 3.5f * u));
    g.setGradientFill (juce::ColourGradient (theme().knobSkirtTop, skirt.getTopLeft(),
                                             theme().knobSkirtBottom, skirt.getBottomLeft(), false));
    g.fillEllipse (skirt);
    g.setColour (theme().knobSkirtEdge);
    g.drawEllipse (skirt.reduced (0.5f), 1.0f);

    // The cap: lit from the top left, two bands of sheen from the bottom
    // left to the top right, and (on big knobs) faint turning rings.
    const auto cap = circle ((compact ? 22.0f : 24.0f) * u);
    juce::ColourGradient capFill (theme().knobCapLight, cap.getX() + cap.getWidth() * 0.15f, cap.getY() + cap.getHeight() * 0.1f,
                                  theme().knobCapDark, cap.getX() + cap.getWidth() * 0.85f, cap.getY() + cap.getHeight() * 0.95f, false);
    capFill.addColour (0.5, theme().knobCapMid);
    g.setGradientFill (capFill);
    g.fillEllipse (cap);

    const auto sheen = theme().knobSheen;
    juce::ColourGradient sheenFill (sheen.withAlpha (0.0f), cap.getBottomLeft(), sheen.withAlpha (0.0f), cap.getTopRight(), false);
    sheenFill.addColour (0.32, sheen);
    sheenFill.addColour (0.5, sheen.withAlpha (0.0f));
    sheenFill.addColour (0.68, sheen.withMultipliedAlpha (0.8f));
    g.setGradientFill (sheenFill);
    g.fillEllipse (cap);

    if (s.isMouseOverOrDragging() && enabled)
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillEllipse (cap);
    }

    if (! compact)
    {
        g.setColour (theme().knobRings);
        g.drawEllipse (circle (18.0f * u), 1.0f);
        g.drawEllipse (circle (11.0f * u), 1.0f);
        g.setColour (theme().knobCapEdge);
        g.drawEllipse (cap, 1.0f);
    }

    // The pointer, drawn pointing up and turned to the value (getPointOnCircumference
    // and AffineTransform::rotation both go clockwise from 12 o'clock, like
    // the slider). A big knob has a bright mark on the skirt that runs into
    // an engraved groove across the cap; a compact one a plain bar.
    const auto turn = juce::AffineTransform::rotation (angle, c.x, c.y);
    auto line = [&] (float r0, float r1, float width, juce::Colour colour, float dx = 0.0f)
    {
        juce::Path p;
        p.startNewSubPath (c.x + dx, c.y - r0);
        p.lineTo (c.x + dx, c.y - r1);
        g.setColour (colour);
        g.strokePath (p, roundStroke (width), turn);
    };

    const auto pointer = ! enabled ? theme().textSecondary : automated ? theme().automated : theme().knobPointer;

    if (compact)
    {
        line (29.0f * u, 14.0f * u, juce::jmax (1.5f, 6.0f * u), pointer);
    }
    else
    {
        line (30.5f * u, 24.0f * u, juce::jmax (1.5f, 3.2f * u), pointer);
        line (22.0f * u, 10.0f * u, juce::jmax (1.2f, 2.6f * u), theme().knobGroove);
        line (21.5f * u, 10.5f * u, juce::jmax (0.6f, 0.8f * u), theme().knobGrooveHighlight, 1.2f * u);
    }

    if (s.hasKeyboardFocus (false))
        drawFocusRing (g, juce::Rectangle<float> (size, size).withCentre (c).expanded (2.0f), size * 0.5f);
}

void LithiumLookAndFeel::drawConsoleCap (juce::Graphics& g, juce::Rectangle<float> cap, bool vertical, juce::Slider& s)
{
    const bool automated = s.getProperties()["automated"];
    const auto corner = juce::jmin (4.0f, juce::jmin (cap.getWidth(), cap.getHeight()) * 0.25f);
    const auto line = automated ? theme().automated
                    : s.getProperties().contains ("family") ? valueColourFor (s, juce::Slider::trackColourId)
                                                            : theme().faderCapLine;

    if (! s.isEnabled())
        g.beginTransparencyLayer (0.5f);

    // Its shadow: two soft steps below it (cheaper than blurring a
    // DropShadow on every repaint while the fader moves).
    g.setColour (theme().faderCapShadow.withMultipliedAlpha (0.35f));
    g.fillRoundedRectangle (cap.translated (0.0f, 4.0f).expanded (1.0f), corner + 1.0f);
    g.setColour (theme().faderCapShadow.withMultipliedAlpha (0.6f));
    g.fillRoundedRectangle (cap.translated (0.0f, 2.0f), corner);

    // The body, shaded along the fader's travel, darkest in the middle
    // where the line runs.
    juce::ColourGradient body (theme().faderCapLight, cap.getTopLeft(),
                               theme().faderCapBottom, vertical ? cap.getBottomLeft() : cap.getTopRight(), false);
    body.addColour (0.10, theme().faderCapHigh);
    body.addColour (0.42, theme().faderCapMid);
    body.addColour (0.50, theme().faderCapGroove);
    body.addColour (0.58, theme().faderCapMid);
    body.addColour (0.90, theme().faderCapHigh);
    g.setGradientFill (body);
    g.fillRoundedRectangle (cap, corner);

    if (automated)
    {
        g.setColour (theme().automated.withAlpha (0.45f));
        g.fillRoundedRectangle (cap, corner);
    }

    // The ribs: across the cap every 3px, either side of the line.
    const auto start = vertical ? cap.getY() : cap.getX();
    const auto mid = start + (vertical ? cap.getHeight() : cap.getWidth()) * 0.5f;
    const auto across = (vertical ? cap.getWidth() : cap.getHeight()) - 6.0f;
    g.setColour (theme().faderCapRibs);

    for (auto t = start + 3.0f; t < mid - 3.0f; t += 3.0f)
        for (const auto rib : { t, 2.0f * mid - t - 1.0f })
            g.fillRect (vertical ? juce::Rectangle<float> (cap.getX() + 3.0f, rib, across, 1.0f)
                                 : juce::Rectangle<float> (rib, cap.getY() + 3.0f, 1.0f, across));

    g.setColour (theme().faderCapBorder);
    g.drawRoundedRectangle (cap.reduced (0.5f), corner, 1.0f);
    g.setColour (theme().faderCapHighlight);
    g.fillRect (juce::Rectangle<float> (cap.getX() + corner, cap.getY() + 1.0f, cap.getWidth() - 2.0f * corner, 1.0f));

    // The line, in the track's colour, with a soft glow in dark mode.
    const auto lineArea = vertical ? juce::Rectangle<float> (cap.getWidth() - 4.0f, 2.0f).withCentre (cap.getCentre())
                                   : juce::Rectangle<float> (2.0f, cap.getHeight() - 4.0f).withCentre (cap.getCentre());

    if (theme().faderGlow > 0.0f)
    {
        g.setColour (line.withMultipliedAlpha (theme().faderGlow));
        g.fillRoundedRectangle (lineArea.expanded (2.0f), 2.0f);
    }

    g.setColour (line);
    g.fillRoundedRectangle (lineArea, 1.0f);

    if (! s.isEnabled())
        g.endTransparencyLayer();
}

int LithiumLookAndFeel::getSliderThumbRadius (juce::Slider& s)
{
    // How far the thumb's centre stays from a slider's ends. Our horizontal
    // thumbs are 12px round, so 6 lets them reach the slider's edges, and
    // the slider's bounds are where it visibly starts and ends (JUCE's
    // default is up to 12, which leaves a gap). Vertical faders keep it.
    return s.isHorizontal() ? 6 : juce::LookAndFeel_V4::getSliderThumbRadius (s);
}

juce::Font LithiumLookAndFeel::getSliderPopupFont (juce::Slider&)
{
    return fonts::mono (textSize::xs);
}

//==============================================================================
void LithiumLookAndFeel::drawMenuBarBackground (juce::Graphics& g, int w, int h, bool, juce::MenuBarComponent&)
{
    g.setColour (theme().titleBarBg);
    g.fillRect (0, 0, w, h);
}

void LithiumLookAndFeel::drawMenuBarItem (juce::Graphics& g, int w, int h, int, const juce::String& text,
                                         bool isMouseOverItem, bool isMenuOpen, bool, juce::MenuBarComponent&)
{
    auto r = juce::Rectangle<float> (0.0f, 2.0f, (float) w, (float) h - 4.0f);

    if (isMenuOpen || isMouseOverItem)
    {
        g.setColour (theme().raised);
        g.fillRoundedRectangle (r, size::radiusSmall);
    }

    g.setColour (isMenuOpen ? theme().text : theme().textMenu);
    g.setFont (fonts::ui (textSize::md));
    g.drawFittedText (text, r.toNearestInt(), juce::Justification::centred, 1);
}

//==============================================================================
void LithiumLookAndFeel::drawResizableWindowBorder (juce::Graphics& g, int w, int h,
                                                   const juce::BorderSize<int>& border, juce::ResizableWindow&)
{
    // The border strip doubles as the resize handle. Fill it with the menu-bar
    // colour so it reads as part of the window, with a 1px outline so the
    // window edge stays visible against dark desktops.
    juce::Rectangle<int> r (w, h);
    g.setColour (theme().titleBarBg);
    g.fillRect (r.removeFromTop (border.getTop()));
    g.fillRect (r.removeFromBottom (border.getBottom()));
    g.fillRect (r.removeFromLeft (border.getLeft()));
    g.fillRect (r.removeFromRight (border.getRight()));

    g.setColour (theme().border);
    g.drawRect (0, 0, w, h, 1);
}

//==============================================================================
void LithiumLookAndFeel::drawCallOutBoxBackground (juce::CallOutBox&, juce::Graphics& g,
                                                  const juce::Path& path, juce::Image&)
{
    // A soft shadow, then the popover body in the toolbar colour with a border.
    theme().popoverShadow.drawForPath (g, path);

    g.setColour (theme().popupBg);
    g.fillPath (path);

    g.setColour (theme().border);
    g.strokePath (path, juce::PathStrokeType (1.0f));
}

float LithiumLookAndFeel::getCallOutBoxCornerSize (const juce::CallOutBox&)
{
    return size::radiusLarge;
}

//==============================================================================
void LithiumLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor& e)
{
    const bool inlineField = e.getProperties()["inline"];   // see styleInlineEditor
    g.setColour (inlineField ? theme().raised : e.findColour (juce::TextEditor::backgroundColourId));
    g.fillRoundedRectangle (juce::Rectangle<float> ((float) w, (float) h), size::radius);
}

void LithiumLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& e)
{
    auto r = juce::Rectangle<float> ((float) w, (float) h).reduced (0.5f);
    const bool focused = e.hasKeyboardFocus (true) && ! e.isReadOnly();

    if ((bool) e.getProperties()["inline"])   // see styleInlineEditor
        g.setColour (theme().selectedBorder);
    else
        g.setColour (e.findColour (focused ? juce::TextEditor::focusedOutlineColourId
                                           : juce::TextEditor::outlineColourId));
    g.drawRoundedRectangle (r, size::radius, focused ? 2.0f : 1.0f);
}

//==============================================================================
int LithiumLookAndFeel::getDefaultScrollbarWidth()
{
    return size::scrollbarWidth;
}

void LithiumLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar& bar, int x, int y, int w, int h,
                                       bool vertical, int thumbStart, int thumbSize, bool over, bool down)
{
    auto thumb = vertical ? juce::Rectangle<int> (x, thumbStart, w, thumbSize)
                          : juce::Rectangle<int> (thumbStart, y, thumbSize, h);

    const bool onArrangement = bar.getProperties()["arrangement"];   // see ArrangementView
    const auto colour = onArrangement ? theme().arrangement.scrollbar : theme().borderStrong;
    g.setColour (colour.brighter (down ? 0.3f : (over ? 0.15f : 0.0f)));
    g.fillRoundedRectangle (thumb.toFloat().reduced (2.5f), size::radiusTiny);
}

} // namespace lithium
