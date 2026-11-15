#include "ui/bottompanel/EqView.h"
#include "ui/widgets/PaintProbe.h"
#include "engine/TrackController.h"
#include "ui/theme/Fonts.h"
#include "ui/theme/Tokens.h"

namespace lithium
{

using namespace tokens;

namespace
{
    // (EqSettings has no operator==, and adding one to EngineTypes.h would rebuild nearly everything.)
    bool sameEq (const EqSettings& a, const EqSettings& b)
    {
        if (a.enabled != b.enabled)
            return false;

        for (size_t i = 0; i < a.bands.size(); ++i)
            if (a.bands[i].name != b.bands[i].name || a.bands[i].frequencyHz != b.bands[i].frequencyHz
                 || a.bands[i].gainDb != b.bands[i].gainDb || a.bands[i].q != b.bands[i].q
                 || a.bands[i].on != b.bands[i].on)
                return false;

        return true;
    }

    constexpr float displayRangeDb = 24.0f;   // the graph shows -24..+24 dB (bands go to ±20)
    constexpr float pointRadius = 7.0f;
    constexpr int readoutHeight = 40;
    constexpr int knobGap = 10, bandTitleHeight = 28, cardPadding = 14;

    // The knobs turn 0..1. Frequency and Q go on a log scale (each turn of
    // the knob is the same musical step), gain straight across.
    float logTo01 (float v, float lo, float hi)    { return std::log (v / lo) / std::log (hi / lo); }
    float logFrom01 (float t, float lo, float hi)  { return lo * std::pow (hi / lo, juce::jlimit (0.0f, 1.0f, t)); }

    juce::String gainText (float db)   { return (db > 0.05f ? "+" : "") + juce::String (db, 1) + " dB"; }

    // Each band has a colour of its own, so its point and its readout match.
    // static_assert checks at compile time that there's one colour per band.
    static_assert (std::tuple_size_v<decltype (ThemePalette::eqBands)> == EqSettings::numBands);

    juce::Colour bandColour (int band)   { return theme().eqBands[(size_t) band]; }
}

//==============================================================================
EqView::EqView (AudioEngine& e) : engine (e)
{
    setTitle ("EQ");

    powerButton.setColours (IconButton::borderedColours);
    powerButton.setFont (fonts::uiSemiBold (textSize::md));
    powerButton.setIconSize (14.0f);
    powerButton.setTooltip ("Turn the EQ on or off, to hear the difference");
    powerButton.onClick = [this] { engine.getTracks().eq().setEnabled (target, ! settings.enabled); };
    addAndMakeVisible (powerButton);

    resetButton.setColours (IconButton::borderedColours);
    resetButton.setFont (fonts::uiSemiBold (textSize::md));
    resetButton.setTooltip ("Put every band back to 0 dB and turn the cuts off");
    resetButton.onClick = [this] { engine.getTracks().eq().reset (target); };
    addAndMakeVisible (resetButton);

    // The selected band's knobs. A drag is one undo step, like dragging its point.
    for (auto* knob : { &frequencyKnob, &gainKnob, &qKnob })
    {
        knob->onDragStart = [this] { engine.beginUndoStep ("EQ"); };
        addAndMakeVisible (knob);
    }

    frequencyKnob.onValueChange = [this] (float v)
    {
        const auto& b = settings.bands[(size_t) selectedBand];
        turnSelectedCutOn();
        setBand (selectedBand, logFrom01 (v, EqController::minFrequency, EqController::maxFrequency), b.gainDb, b.q);
    };

    gainKnob.onValueChange = [this] (float v)
    {
        const auto& b = settings.bands[(size_t) selectedBand];
        setBand (selectedBand, b.frequencyHz, juce::jmap (v, EqController::minGain, EqController::maxGain), b.q);
    };

    qKnob.onValueChange = [this] (float v)
    {
        const auto& b = settings.bands[(size_t) selectedBand];
        turnSelectedCutOn();
        setBand (selectedBand, b.frequencyHz, b.gainDb, logFrom01 (v, EqController::minQ, EqController::maxQ));
    };

    cutButton.setColours (IconButton::borderedColours);
    cutButton.setFont (fonts::uiSemiBold (textSize::md));
    cutButton.setIconSize (14.0f);
    cutButton.onClick = [this]
    {
        engine.beginUndoStep ("EQ");
        engine.getTracks().eq().setBandOn (target, selectedBand, ! settings.bands[(size_t) selectedBand].on);
        refresh();
    };
    addChildComponent (cutButton);
}

void EqView::setTarget (TrackId id, Family f)
{
    if (id != target)
        dragBand = hoverBand = -1;

    target = id;
    family = f;
    refresh();
}

void EqView::refresh()
{
    // Called on every knob change in the session (many a second while
    // automation plays): only redraw when the EQ itself changed.
    const auto newSettings = target != 0 ? engine.getTracks().eq().getEq (target) : EqController::getDefaults();

    if (shownOnce && sameEq (newSettings, settings) && target == shownTarget && family == shownFamily)
        return;

    settings = newSettings;
    shownTarget = target;
    shownFamily = family;
    shownOnce = true;
    powerButton.setToggleState (settings.enabled, juce::dontSendNotification);
    powerButton.setText (settings.enabled ? "On" : "Off");
    powerButton.setEnabled (target != 0);
    resetButton.setEnabled (target != 0);
    updateKnobs();
    updateCurve();
    repaint();
}

void EqView::selectBand (int band)
{
    if (band == selectedBand || ! juce::isPositiveAndBelow (band, EqSettings::numBands))
        return;

    selectedBand = band;
    updateKnobs();
    repaint();
}

void EqView::updateKnobs()
{
    const auto& b = settings.bands[(size_t) selectedBand];

    for (auto* knob : { &frequencyKnob, &gainKnob, &qKnob })
    {
        knob->setArcFamily (family);
        knob->setEnabled (target != 0);
    }

    frequencyKnob.setValue (logTo01 (b.frequencyHz, EqController::minFrequency, EqController::maxFrequency),
                            describeFrequency (b.frequencyHz));
    gainKnob.setValue (juce::jmap (b.gainDb, EqController::minGain, EqController::maxGain, 0.0f, 1.0f), gainText (b.gainDb));
    qKnob.setValue (logTo01 (b.q, EqController::minQ, EqController::maxQ), juce::String (b.q, 2));

    // A cut has no gain: just Frequency and Q, with On/Off beside its name.
    gainKnob.setVisible (! b.isCut());
    cutButton.setVisible (b.isCut());
    layOutKnobs();
    cutButton.setEnabled (target != 0);
    cutButton.setToggleState (b.on, juce::dontSendNotification);
    cutButton.setText (b.on ? "On" : "Off");
    cutButton.setTitle ((b.on ? "Turn off " : "Turn on ") + b.name);
    cutButton.setTooltip (b.on ? "On (click to turn it off)" : "Off (click to turn it on)");
}

void EqView::layOutKnobs()
{
    // Left to right: Frequency, Gain (not on a cut), Q.
    auto row = knobRow;

    for (auto* knob : { &frequencyKnob, &gainKnob, &qKnob })
    {
        if (! knob->isVisible())
            continue;

        knob->setBounds (row.removeFromLeft (Knob::preferredWidth));
        row.removeFromLeft (knobGap);
    }
}

void EqView::turnSelectedCutOn()
{
    // Turning a knob on a cut that's off turns it on (in the same undo step).
    const auto& b = settings.bands[(size_t) selectedBand];

    if (b.isCut() && ! b.on)
        engine.getTracks().eq().setBandOn (target, selectedBand, true);
}

int EqView::readoutColumnAt (juce::Point<float> p) const
{
    if (! readoutArea.toFloat().contains (p))
        return -1;

    const auto columnWidth = (float) readoutArea.getWidth() / (float) EqSettings::numBands;
    return juce::jlimit (0, EqSettings::numBands - 1, (int) ((p.x - (float) readoutArea.getX()) / columnWidth));
}

void EqView::updateCurve()
{
    // One point per column of the graph.
    std::vector<float> frequencies;

    for (int x = 0; x < graphArea.getWidth(); ++x)
        frequencies.push_back (xToFrequency ((float) (graphArea.getX() + x)));

    curve = target != 0 ? engine.getTracks().eq().getResponse (target, frequencies)
                        : std::vector<float> (frequencies.size(), 0.0f);
}

//==============================================================================
// Coordinates
float EqView::frequencyToX (float hz) const
{
    const auto proportion = std::log (hz / EqController::minFrequency) / std::log (EqController::maxFrequency / EqController::minFrequency);
    return (float) graphArea.getX() + proportion * (float) graphArea.getWidth();
}

float EqView::xToFrequency (float x) const
{
    const auto proportion = juce::jlimit (0.0f, 1.0f, (x - (float) graphArea.getX()) / (float) juce::jmax (1, graphArea.getWidth()));
    return EqController::minFrequency * std::pow (EqController::maxFrequency / EqController::minFrequency, proportion);
}

float EqView::gainToY (float db) const
{
    return juce::jmap (db, displayRangeDb, -displayRangeDb, (float) graphArea.getY(), (float) graphArea.getBottom());
}

float EqView::yToGain (float y) const
{
    return juce::jmap (y, (float) graphArea.getY(), (float) graphArea.getBottom(), displayRangeDb, -displayRangeDb);
}

juce::Point<float> EqView::bandPoint (int band) const
{
    const auto& b = settings.bands[(size_t) band];
    return { frequencyToX (b.frequencyHz), gainToY (b.isCut() ? 0.0f : b.gainDb) };
}

int EqView::bandAt (juce::Point<float> p) const
{
    // The nearest point within reach (a little more than its size).
    int best = -1;
    float bestDistance = pointRadius * 2.0f;

    for (int b = 0; b < EqSettings::numBands; ++b)
    {
        const auto d = bandPoint (b).getDistanceFrom (p);

        if (d < bestDistance)
        {
            best = b;
            bestDistance = d;
        }
    }

    return best;
}

void EqView::setBand (int band, float frequencyHz, float gainDb, float q)
{
    engine.getTracks().eq().setBand (target, band, frequencyHz, gainDb, q);
    refresh();   // straight away, so the point keeps up with the mouse
}

juce::String EqView::describeFrequency (float hz)
{
    return hz >= 1000.0f ? juce::String (hz / 1000.0f, hz >= 10000.0f ? 0 : 1) + " kHz"
                         : juce::String (juce::roundToInt (hz)) + " Hz";
}

//==============================================================================
// Mouse
void EqView::mouseMove (const juce::MouseEvent& e)
{
    const auto band = target != 0 ? bandAt (e.position) : -1;

    if (band != hoverBand)
    {
        hoverBand = band;
        setMouseCursor (band >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void EqView::mouseExit (const juce::MouseEvent&)
{
    if (hoverBand >= 0 && dragBand < 0)
    {
        hoverBand = -1;
        repaint();
    }
}

void EqView::mouseDown (const juce::MouseEvent& e)
{
    dragBand = target != 0 ? bandAt (e.position) : -1;

    // Clicking a band's readout selects it for the knobs.
    if (dragBand < 0)
    {
        if (const auto column = target != 0 ? readoutColumnAt (e.position) : -1; column >= 0)
            selectBand (column);

        return;
    }

    selectBand (dragBand);

    engine.beginUndoStep ("EQ");   // the whole drag undoes in one go

    // Clicking a cut that's off turns it on.
    if (settings.bands[(size_t) dragBand].isCut() && ! settings.bands[(size_t) dragBand].on)
    {
        engine.getTracks().eq().setBandOn (target, dragBand, true);
        refresh();
    }
}

void EqView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragBand < 0)
        return;

    const auto pos = graphArea.toFloat().getConstrainedPoint (e.position);
    setBand (dragBand, xToFrequency (pos.x),
             juce::jlimit (EqController::minGain, EqController::maxGain, yToGain (pos.y)),
             settings.bands[(size_t) dragBand].q);
}

void EqView::mouseUp (const juce::MouseEvent&)
{
    dragBand = -1;
}

void EqView::mouseDoubleClick (const juce::MouseEvent& e)
{
    // Back to 0 dB, keeping the frequency and width; a cut turns off.
    if (const auto band = target != 0 ? bandAt (e.position) : -1; band >= 0)
    {
        const auto& b = settings.bands[(size_t) band];
        engine.beginUndoStep ("EQ");

        if (b.isCut())
        {
            engine.getTracks().eq().setBandOn (target, band, false);
            refresh();
        }
        else
        {
            setBand (band, b.frequencyHz, 0.0f, b.q);
        }
    }
}

void EqView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    // Over a point: wider (wheel down) or narrower (wheel up).
    const auto band = target != 0 ? bandAt (e.position) : -1;

    if (band < 0 || wheel.deltaY == 0.0f)
        return;

    const auto& b = settings.bands[(size_t) band];
    engine.beginUndoStep ("EQ");
    setBand (band, b.frequencyHz, b.gainDb,
             juce::jlimit (EqController::minQ, EqController::maxQ, b.q * std::pow (2.0f, wheel.deltaY * 2.0f)));
}

//==============================================================================
void EqView::paint (juce::Graphics& g)
{
    LITHIUM_PERF_PAINT ("EqView");

    // The selected band's card: its name (and a cut's On/Off), over its knobs.
    {
        const auto card = knobCard.toFloat();
        g.setColour (theme().panelBoxBg);
        g.fillRoundedRectangle (card, size::radiusCard);
        g.setColour (theme().border);
        g.drawRoundedRectangle (card.reduced (0.5f), size::radiusCard, 1.0f);

        auto r = bandTitleArea;

        if (cutButton.isVisible())
            r.setRight (cutButton.getX() - 8);

        g.setColour (settings.enabled && target != 0 ? bandColour (selectedBand) : theme().textSecondary);
        g.fillEllipse (r.removeFromLeft (10).withSizeKeepingCentre (10, 10).toFloat());
        r.removeFromLeft (8);
        g.setColour (target != 0 ? theme().text : theme().textSecondary);
        g.setFont (fonts::uiBold (textSize::md));
        g.drawText (settings.bands[(size_t) selectedBand].name, r, juce::Justification::centredLeft, true);
    }

    // The graph's background and grid.
    g.setColour (theme().wellBg);
    g.fillRoundedRectangle (graphArea.toFloat(), size::radius);

    g.setFont (fonts::mono (textSize::tiny));

    for (float hz : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const auto x = frequencyToX (hz);
        const bool labelled = hz == 100.0f || hz == 1000.0f || hz == 10000.0f;
        g.setColour (theme().tintLight.withAlpha (labelled ? 0.08f : 0.04f));
        g.fillRect (x, (float) graphArea.getY(), 1.0f, (float) graphArea.getHeight());

        if (labelled)
        {
            g.setColour (theme().textSecondary);
            g.drawText (describeFrequency (hz), juce::Rectangle<float> (x + 4.0f, (float) graphArea.getBottom() - 16.0f, 60.0f, 14.0f),
                        juce::Justification::centredLeft);
        }
    }

    for (float db : { -12.0f, 0.0f, 12.0f })
    {
        const auto y = gainToY (db);
        g.setColour (theme().tintLight.withAlpha (db == 0.0f ? 0.12f : 0.05f));
        g.fillRect ((float) graphArea.getX(), y, (float) graphArea.getWidth(), 1.0f);
        g.setColour (theme().textSecondary);
        g.drawText ((db > 0.0f ? "+" : "") + juce::String ((int) db) + " dB",
                    juce::Rectangle<float> ((float) graphArea.getX() + 6.0f, y - 14.0f, 60.0f, 12.0f),
                    juce::Justification::centredLeft);
    }

    if (target == 0)
    {
        g.setColour (theme().textSecondary);
        g.setFont (fonts::ui (textSize::lg));
        g.drawText ("Select a track to EQ it", graphArea, juce::Justification::centred);
        return;
    }

    // The curve, filled down (or up) to the 0 dB line.
    const auto curveColour = settings.enabled ? theme().family (family) : theme().textSecondary;
    juce::Path line;

    for (size_t i = 0; i < curve.size(); ++i)
    {
        const auto x = (float) graphArea.getX() + (float) i;
        const auto y = gainToY (juce::jlimit (-displayRangeDb, displayRangeDb, curve[i]));

        if (i == 0)
            line.startNewSubPath (x, y);
        else
            line.lineTo (x, y);
    }

    if (! curve.empty())
    {
        auto fill = line;
        fill.lineTo ((float) graphArea.getRight(), gainToY (0.0f));
        fill.lineTo ((float) graphArea.getX(), gainToY (0.0f));
        fill.closeSubPath();
        g.setColour (curveColour.withAlpha (0.15f));
        g.fillPath (fill);
        g.setColour (curveColour);
        g.strokePath (line, juce::PathStrokeType (2.0f));
    }

    // The band points, numbered. A cut that's off is a hollow ring.
    for (int b = 0; b < EqSettings::numBands; ++b)
    {
        const auto p = bandPoint (b);
        const bool active = b == hoverBand || b == dragBand || b == selectedBand;
        const bool off = ! settings.bands[(size_t) b].on;
        const auto c = settings.enabled ? bandColour (b) : theme().textSecondary;
        auto circle = juce::Rectangle<float> (pointRadius * 2.0f, pointRadius * 2.0f).withCentre (p);

        g.setColour (c.withAlpha (active ? 1.0f : 0.85f));

        if (off)
            g.drawEllipse (circle.reduced (0.75f), 1.5f);
        else
            g.fillEllipse (circle);

        if (active)
        {
            g.setColour (theme().text);
            g.drawEllipse (circle.expanded (2.0f), 1.5f);
        }

        g.setColour (off ? c : theme().textOnAccent);
        g.setFont (fonts::uiBold (textSize::tiny));
        g.drawText (juce::String (b + 1), circle, juce::Justification::centred);
    }

    // Each band's settings, underneath.
    const int columnWidth = readoutArea.getWidth() / EqSettings::numBands;

    for (int b = 0; b < EqSettings::numBands; ++b)
    {
        const auto& band = settings.bands[(size_t) b];
        auto r = readoutArea.withX (readoutArea.getX() + b * columnWidth).withWidth (columnWidth).reduced (4, 0);

        if (b == selectedBand)
        {
            g.setColour (theme().selectedRow);
            g.fillRoundedRectangle (r.expanded (2, 0).toFloat(), size::radius);
        }

        g.setColour (settings.enabled ? bandColour (b) : theme().textSecondary);
        const auto dot = r.removeFromLeft (8).withSizeKeepingCentre (8, 8).toFloat();

        if (band.on)
            g.fillEllipse (dot);
        else
            g.drawEllipse (dot.reduced (0.5f), 1.0f);

        r.removeFromLeft (6);

        g.setColour (theme().text);
        g.setFont (fonts::uiBold (textSize::sm));
        g.drawText (band.name, r.removeFromTop (readoutHeight / 2), juce::Justification::bottomLeft, true);

        g.setColour (theme().textSecondary);
        g.setFont (fonts::ui (textSize::sm));
        // A cut shows Off, or its resonance; the other bands their gain and width.
        const auto gain = gainText (band.gainDb);
        const auto details = band.isCut() ? (band.on ? "Q " + juce::String (band.q, 1) : juce::String ("Off"))
                                          : gain + "  Q " + juce::String (band.q, 1);
        g.drawText (describeFrequency (band.frequencyHz) + "  " + details, r, juce::Justification::topLeft, true);
    }
}

void EqView::resized()
{
    auto r = getLocalBounds().reduced (18, 14);

    auto top = r.removeFromTop (28);
    powerButton.setBounds (top.removeFromLeft (72));
    resetButton.setBounds (top.removeFromRight (72));
    r.removeFromTop (10);

    // The selected band's card, to the right of the graph and readouts.
    auto column = r.removeFromRight (3 * Knob::preferredWidth + 2 * knobGap + 2 * cardPadding);
    r.removeFromRight (16);
    knobCard = column.removeFromTop (cardPadding * 2 + bandTitleHeight + 10 + Knob::preferredHeight);

    auto inside = knobCard.reduced (cardPadding);
    bandTitleArea = inside.removeFromTop (bandTitleHeight);
    cutButton.setBounds (bandTitleArea.withLeft (bandTitleArea.getRight() - 72));
    inside.removeFromTop (10);
    knobRow = inside.removeFromTop (Knob::preferredHeight);
    layOutKnobs();

    readoutArea = r.removeFromBottom (readoutHeight);
    r.removeFromBottom (6);
    graphArea = r;
    updateCurve();
}

} // namespace lithium
