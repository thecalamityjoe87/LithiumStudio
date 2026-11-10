#include "ui/titlebar/WindowControls.h"
#include "ui/theme/Tokens.h"

namespace lithium
{

using namespace tokens;

namespace
{
    // The glyphs, in a unit square, as JUCE's LookAndFeel_V4 makes them.
    constexpr float strokeThickness = 0.14f;

    juce::Path minimiseShape()
    {
        juce::Path p;
        p.addLineSegment ({ 0.0f, 0.5f, 1.0f, 0.5f }, strokeThickness);
        return p;
    }

    juce::Path maximiseShape()
    {
        juce::Path p;
        p.addLineSegment ({ 0.5f, 0.0f, 0.5f, 1.0f }, strokeThickness);
        p.addLineSegment ({ 0.0f, 0.5f, 1.0f, 0.5f }, strokeThickness);
        return p;
    }

    // Shown while the window is maximised: two overlapping corners.
    juce::Path restoreShape()
    {
        juce::Path outline;
        outline.startNewSubPath (45.0f, 100.0f);
        outline.lineTo (0.0f, 100.0f);
        outline.lineTo (0.0f, 0.0f);
        outline.lineTo (100.0f, 0.0f);
        outline.lineTo (100.0f, 45.0f);
        outline.addRectangle (45.0f, 45.0f, 100.0f, 100.0f);

        juce::Path p;
        juce::PathStrokeType (30.0f).createStrokedPath (p, outline);
        return p;
    }

    juce::Path closeShape()
    {
        juce::Path p;
        p.addLineSegment ({ 0.0f, 0.0f, 1.0f, 1.0f }, strokeThickness);
        p.addLineSegment ({ 1.0f, 0.0f, 0.0f, 1.0f }, strokeThickness);
        return p;
    }
}

WindowControls::WindowButton::WindowButton (const juce::String& name, juce::Colour c, juce::Path normal, juce::Path toggled)
    : juce::Button (name), colour (c), normalShape (std::move (normal)), toggledShape (std::move (toggled))
{
    setTitle (name);
    setTooltip (name);
}

void WindowControls::WindowButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto glyphColour = down ? colour.withAlpha (0.6f) : colour;

    if (highlighted || down)
    {
        g.fillAll (glyphColour);
        g.setColour (theme().titleBarBg);
    }
    else
    {
        g.setColour (glyphColour);
    }

    // The glyph: a third of the button's height, in the middle.
    const auto& shape = getToggleState() ? toggledShape : normalShape;
    const auto h = (float) getHeight();
    const auto area = getLocalBounds().toFloat().withSizeKeepingCentre (h, h).reduced (h / 3.0f);
    g.fillPath (shape, shape.getTransformToScaleToFit (area, true));
}

WindowControls::WindowControls()
    : minimiseButton ("Minimize", juce::Colour (0xffaa8811), minimiseShape(), minimiseShape()),
      maximiseButton ("Maximize", juce::Colour (0xff0a830a), maximiseShape(), restoreShape()),
      closeButton    ("Close",    juce::Colour (0xff9a131d), closeShape(), closeShape())
{
    for (auto* b : { &minimiseButton, &maximiseButton, &closeButton })
        addAndMakeVisible (b);

    // The DocumentWindow already knows how to do each of these; we just call
    // the same methods its built-in title-bar buttons would.
    minimiseButton.onClick = [this] { if (auto* w = getWindow()) w->minimiseButtonPressed(); };
    maximiseButton.onClick = [this] { if (auto* w = getMaximisableWindow()) w->toggleMaximised(); };
    closeButton.onClick    = [this] { if (auto* w = getWindow()) w->closeButtonPressed(); };
}

juce::DocumentWindow* WindowControls::getWindow() const
{
    // findParentComponentOfClass walks up the parent chain and returns the
    // first ancestor of that type (or nullptr).
    return findParentComponentOfClass<juce::DocumentWindow>();
}

MaximisableWindow* WindowControls::getMaximisableWindow() const
{
    // dynamic_cast across to the interface: the top-level window implements it.
    return dynamic_cast<MaximisableWindow*> (getTopLevelComponent());
}

void WindowControls::update()
{
    auto* window = getMaximisableWindow();
    const bool maximised = window != nullptr && window->isMaximised();

    if (maximised != showingRestore)
    {
        showingRestore = maximised;
        maximiseButton.setToggleState (maximised, juce::dontSendNotification);
        maximiseButton.setTitle (maximised ? "Restore" : "Maximize");
        maximiseButton.setTooltip (maximised ? "Restore" : "Maximize");
    }
}

juce::Component::WindowControlKind WindowControls::getControlAt (juce::Point<float> p) const
{
    // Plain rectangle tests only (see the warning on Component::findControlAtPoint).
    if (minimiseButton.getBounds().toFloat().contains (p))  return WindowControlKind::minimise;
    if (maximiseButton.getBounds().toFloat().contains (p))  return WindowControlKind::maximise;
    if (closeButton.getBounds().toFloat().contains (p))     return WindowControlKind::close;
    return WindowControlKind::caption;
}

void WindowControls::resized()
{
    auto r = getLocalBounds();

    minimiseButton.setBounds (r.removeFromLeft (buttonWidth));
    maximiseButton.setBounds (r.removeFromLeft (buttonWidth));
    closeButton.setBounds (r.removeFromLeft (buttonWidth));
}

} // namespace lithium
