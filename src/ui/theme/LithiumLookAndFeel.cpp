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
        // Mixer fader, as on Lithium Drums: a thin track filled below the cap
        // in the channel's colour (trackColourId, or its family), a tick
        // across it at unity gain (the "unityValue" property, if set), and a
        // small raised cap with a line across its middle.
        const auto track = bounds.withSizeKeepingCentre (4.0f, bounds.getHeight());
        g.setColour (theme().wellBg);
        g.fillRoundedRectangle (track, 2.0f);

        g.setColour (trackColour.withMultipliedAlpha (s.isEnabled() ? 0.8f : 0.3f));
        g.fillRoundedRectangle (track.withTop (sliderPos), 2.0f);

        if (const auto& unity = s.getProperties()["unityValue"]; ! unity.isVoid())
        {
            const auto uy = (float) s.getPositionOfValue ((double) unity);
            g.setColour (theme().borderStrong);
            g.fillRect (juce::Rectangle<float> (track.getX() - 6.0f, uy - 0.5f, track.getWidth() + 12.0f, 1.0f));
        }

        const auto cap = juce::Rectangle<float> (juce::jmin (26.0f, bounds.getWidth()), 14.0f).withCentre ({ track.getCentreX(), sliderPos });
        g.setColour ((automated ? theme().automated : theme().faderCap).withMultipliedAlpha (s.isEnabled() ? 1.0f : 0.5f));
        g.fillRoundedRectangle (cap, 3.0f);
        g.setColour (theme().faderCapBorder);
        g.drawRoundedRectangle (cap.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (theme().faderCapLine);
        g.fillRect (cap.withSizeKeepingCentre (cap.getWidth() - 8.0f, 1.5f));
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

    // A small knob (e.g. a track header's pan knob) uses the compact
    // colours, so its ring stays visible on a selected track. Set with
    // slider.getProperties().set ("compact", true).
    if ((bool) s.getProperties()["compact"])
    {
        drawCompactKnob (g, juce::Rectangle<int> (x, y, w, h).toFloat(), pos, startAngle, endAngle, s);
        return;
    }

    // The big knobs keep a little margin inside their bounds.
    drawKnob (g, juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f), pos, startAngle, endAngle, s,
              theme().knobTrack, theme().knobBody, theme().knobRim, theme().knobPointer);
}

void LithiumLookAndFeel::drawCompactKnob (juce::Graphics& g, juce::Rectangle<float> area, float pos,
                                          float startAngle, float endAngle, juce::Slider& s)
{
    // The same knob, in the compact colours: the ring is a shade lighter so it
    // still shows on a selected track's lighter background.
    drawKnob (g, area, pos, startAngle, endAngle, s,
              theme().compactKnobTrack, theme().compactKnobBody, theme().compactKnobRim, theme().compactKnobPointer);
}

void LithiumLookAndFeel::drawKnob (juce::Graphics& g, juce::Rectangle<float> area, float pos, float startAngle, float endAngle,
                                   juce::Slider& s, juce::Colour trackColour, juce::Colour bodyColour,
                                   juce::Colour rimColour, juce::Colour pointerColour)
{
    // Lithium Drums' knob: a thin ring, the value arc along it in the
    // knob's colour, a flat round body inside with a hairline rim, and a
    // straight pointer from near the middle to the body's edge.
    const auto size = juce::jmin (area.getWidth(), area.getHeight());
    const auto centre = area.getCentre();
    const auto radius = size * 0.5f - 2.0f;
    const auto thickness = juce::jmax (2.0f, size * 0.08f);
    const auto enabled = s.isEnabled();
    const auto angle = startAngle + pos * (endAngle - startAngle);

    // A "bipolar" knob (e.g. pan) draws its arc out from 12 o'clock. Set with
    // slider.getProperties().set ("bipolar", true).
    const bool bipolar = s.getProperties()["bipolar"];
    const auto fromAngle = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

    auto arc = [&] (float a0, float a1)
    {
        juce::Path p;
        p.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, a0, a1, true);
        return p;
    };

    const juce::PathStrokeType stroke (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    g.setColour (trackColour);
    g.strokePath (arc (startAngle, endAngle), stroke);

    if (std::abs (angle - fromAngle) > 0.001f)
    {
        const auto colour = valueColourFor (s, knobArcColourId);
        g.setColour (enabled ? colour : colour.withMultipliedAlpha (0.35f));
        g.strokePath (arc (juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle)), stroke);
    }

    const auto bodyRadius = radius - thickness - 2.0f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);
    g.setColour (s.isMouseOverOrDragging() && enabled ? bodyColour.brighter (0.12f) : bodyColour);
    g.fillEllipse (body);
    g.setColour (rimColour);
    g.drawEllipse (body, 1.0f);

    // getPointOnCircumference measures angles clockwise from 12 o'clock, like the slider does.
    // An automated knob's pointer is in the "automated" colour (see drawLinearSlider).
    const auto tip = centre.getPointOnCircumference (bodyRadius - 1.5f, angle);
    const auto root = centre.getPointOnCircumference (bodyRadius * 0.35f, angle);
    g.setColour ((bool) s.getProperties()["automated"] ? theme().automated
                                                        : (enabled ? pointerColour : theme().textSecondary));
    g.drawLine ({ root, tip }, juce::jmax (1.5f, size * 0.05f));

    if (s.hasKeyboardFocus (false))
        drawFocusRing (g, juce::Rectangle<float> (size, size).withCentre (centre).expanded (2.0f), size * 0.5f);
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
