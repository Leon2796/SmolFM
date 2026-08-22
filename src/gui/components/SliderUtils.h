/*
    SliderUtils.h collects the repeated slider-configuration used by the
    SmolFM GUI panels.

    Only inline free functions live here so the header can be included
    anywhere without extra build rules.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{

/**
    Configure a rotary slider with the standard SmolFM look and feel.

    Used identically by OscillatorPanel, AdsrPanel and FmAmountComponent.
*/
inline void configureRotarySlider (juce::Slider& slider, const juce::String& suffix) noexcept
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 24);
    slider.setTextValueSuffix (suffix);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::lightblue);
}

} // namespace gui
