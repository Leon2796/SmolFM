/*
    FAdsrComponent implementation.
*/

#include "FAdsrComponent.h"
#include "SliderUtils.h"
// Interval names + exact ratios: fscaleIntervalNames lives in the graph header
// (ISmolFmParser.h), which pulls in FrequencyScaleProcessor.h for the ratio table.
#include "../../graph/ISmolFmParser.h"

namespace gui
{

namespace
{
    /*
        Build the ADSR curve path for the current timing slider values.

        The four phases share the display width proportionally to their
        duration; attack is linear, decay and release fall exponentially to
        match the juce::ADSR shape (same math as AdsrPanel).
    */
    void buildCurvePath (juce::Path& path,
                         const juce::Rectangle<float>& bounds,
                         float attackSeconds,
                         float decaySeconds,
                         float sustainLevel,
                         float releaseSeconds)
    {
        const float total = juce::jmax (attackSeconds + decaySeconds + releaseSeconds,
                                        1.0e-6f);
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        const float x0 = bounds.getX();
        const float yBottom = bounds.getBottom();

        const float ax = w * (attackSeconds / total);
        const float dx = w * (decaySeconds / total);
        const float rx = w * (releaseSeconds / total);

        auto xFor = [&x0] (float offset) { return x0 + offset; };
        auto yFor = [&yBottom, &h] (float level)
        {
            return yBottom - juce::jlimit (0.0f, 1.0f, level) * h;
        };

        path.clear();
        path.startNewSubPath (x0, yBottom);

        // Linear attack up to 1.0.
        path.lineTo (xFor (ax), yFor (1.0f));

        // Exponential decay down to the sustain level.
        if (dx > 0.0f && sustainLevel < 1.0f)
        {
            path.quadraticTo (xFor (ax + dx * 0.5f), yFor (1.0f - (1.0f - sustainLevel) * 0.6f),
                              xFor (ax + dx), yFor (sustainLevel));
        }
        else
        {
            path.lineTo (xFor (ax + dx), yFor (sustainLevel));
        }

        // Sustain holds until the release starts.
        path.lineTo (xFor (ax + dx), yFor (sustainLevel));

        // Exponential release back to zero.
        if (rx > 0.0f)
        {
            path.quadraticTo (xFor (ax + dx + rx * 0.5f), yFor (sustainLevel * 0.4f),
                              xFor (ax + dx + rx), yBottom);
        }
        else
        {
            path.lineTo (xFor (ax + dx + rx), yBottom);
        }
    }
} // namespace

FAdsrComponent::FAdsrComponent (juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& attackParameterID,
                                const juce::String& decayParameterID,
                                const juce::String& sustainParameterID,
                                const juce::String& releaseParameterID,
                                const juce::String& upParameterID,
                                const juce::String& downParameterID)
{
    configureRotarySlider (attackSlider,  " s");
    configureRotarySlider (decaySlider,   " s");
    configureRotarySlider (sustainSlider, "");
    configureRotarySlider (releaseSlider, " s");
    configureRotarySlider (upSlider,   "x");
    configureRotarySlider (downSlider, "x");

    addAndMakeVisible (attackSlider);
    addAndMakeVisible (decaySlider);
    addAndMakeVisible (sustainSlider);
    addAndMakeVisible (releaseSlider);
    addAndMakeVisible (upSlider);
    addAndMakeVisible (downSlider);

    // Right-click on the two factor knobs opens the interval preset menu
    // (see mouseDown); JUCE's own slider pop-up menu stays off by default.
    upSlider.addMouseListener (this, false);
    downSlider.addMouseListener (this, false);
    addAndMakeVisible (curveDisplay);

    attackSlider.addListener (this);
    decaySlider.addListener (this);
    sustainSlider.addListener (this);
    releaseSlider.addListener (this);

    attackAttachment.reset  (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, attackParameterID,  attackSlider));
    decayAttachment.reset   (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, decayParameterID,   decaySlider));
    sustainAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, sustainParameterID, sustainSlider));
    releaseAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, releaseParameterID, releaseSlider));
    upAttachment.reset      (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, upParameterID,      upSlider));
    downAttachment.reset    (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, downParameterID,    downSlider));

    // Slightly wider than the other processor contents (220 default) so the
    // six sliders keep usable thumb sizes; height matches the standard box.
    setSize (280, 260);
}

FAdsrComponent::~FAdsrComponent()
{
}

void FAdsrComponent::sliderValueChanged (juce::Slider*)
{
    curveDisplay.repaint();
}

/*
    Right-click on an up/down knob: popup with the 11 interval names; picking
    one writes its exact 12-TET ratio (fscaleIntervalRatios) as the knob value,
    sendNotification lets the APVTS attachment forward it to the DSP.
*/
void FAdsrComponent::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isRightButtonDown())
        return;

    // The mouse listeners above are attached only to the two factor knobs.
    juce::Slider* factorSlider = (e.eventComponent == &upSlider ? &upSlider : &downSlider);

    const juce::StringArray names = smolfm::fscaleIntervalNames();
    juce::PopupMenu menu;
    for (int i = 0; i < smolfm::numFscaleIntervals; ++i)
        menu.addItem (i + 1, names[i] + "  (" + juce::String (smolfm::fscaleIntervalRatios[i], 4) + "x)");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (factorSlider),
        [this, factorSlider] (int result)
        {
            if (result > 0)
                factorSlider->setValue (smolfm::fscaleIntervalRatios[result - 1], juce::sendNotification);
        });
}

void FAdsrComponent::CurveDisplay::paint (juce::Graphics& g)
{
    auto* panel = findParentComponentOfClass<FAdsrComponent>();
    if (panel == nullptr)
        return;

    g.fillAll (getLookAndFeel().findColour (juce::Slider::textBoxOutlineColourId).withAlpha (0.2f));
    g.setColour (juce::Colours::lightgreen);

    const float attack  = static_cast<float> (panel->attackSlider.getValue());
    const float decay   = static_cast<float> (panel->decaySlider.getValue());
    const float sustain = static_cast<float> (panel->sustainSlider.getValue());
    const float release = static_cast<float> (panel->releaseSlider.getValue());

    juce::Path curve;
    buildCurvePath (curve, getLocalBounds().toFloat().reduced (4.0f), attack, decay, sustain, release);
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Closed shape under the curve, faint fill so the envelope area reads well.
    // Curve starts and ends on the bottom edge, so closeSubPath fills the area.
    juce::Path fill (curve);
    fill.closeSubPath();
    g.setColour (juce::Colours::lightgreen.withAlpha (0.25f));
    g.fillPath (fill);
}

void FAdsrComponent::resized()
{
    juce::Grid grid;
    grid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Fr (2)),
                          juce::Grid::TrackInfo (juce::Grid::Fr (4)),
                          juce::Grid::TrackInfo (juce::Grid::Fr (3)) };
    grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                             juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                             juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                             juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
    grid.rowGap = juce::Grid::Px (8);
    grid.columnGap = juce::Grid::Px (8);

    // The ADSR curve preview spans all four columns.
    juce::GridItem curveItem (curveDisplay);
    curveItem.column = { juce::GridItem::Span (4) };
    grid.items.add (curveItem);

    // First row: the four envelope timing sliders.
    grid.items.add (juce::GridItem (attackSlider));
    grid.items.add (juce::GridItem (decaySlider));
    grid.items.add (juce::GridItem (sustainSlider));
    grid.items.add (juce::GridItem (releaseSlider));

    // Second row: the two frequency scaling factors, each spanning two
    // of the four columns so they sit centred side by side.
    juce::GridItem upItem (upSlider);
    upItem.column = { 1, 2 };
    upItem.row = { 3 };
    grid.items.add (upItem);

    juce::GridItem downItem (downSlider);
    downItem.column = { 3, 4 };
    downItem.row = { 3 };
    grid.items.add (downItem);

    grid.performLayout (getLocalBounds());
}

} // namespace gui