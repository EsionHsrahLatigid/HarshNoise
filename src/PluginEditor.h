#pragma once

#include "PluginProcessor.h"

#include <ehl/juce_design/EhlDesign.h>
#include <JuceHeader.h>

#include <array>
#include <memory>

class HarshNoiseAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit HarshNoiseAudioProcessorEditor(HarshNoiseAudioProcessor&);
    ~HarshNoiseAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    juce::String getTooltip() { return tooltipText; }

    static constexpr int defaultWidth = ehl::juce_design::Metrics::defaultWidth;
    static constexpr int defaultHeight = ehl::juce_design::Metrics::defaultHeight;
    static constexpr int minimumWidth = ehl::juce_design::Metrics::minimumWidth;
    static constexpr int minimumHeight = ehl::juce_design::Metrics::minimumHeight;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void addControl(int index, const juce::String& parameterId, const juce::String& labelText, const juce::String& tip);
    void timerCallback() override;
    void updateParameterDisplay();
    
    HarshNoiseAudioProcessor& audioProcessor;
    ehl::juce_design::LookAndFeel lookAndFeel;
    ehl::juce_design::ParameterDisplay parameterDisplay { ehl::juce_design::DisplayKind::distortion };
    juce::TooltipWindow tooltipWindow { this, 700 };
    juce::String tooltipText;

    std::array<juce::Slider, 6> sliders;
    std::array<juce::Label, 6> labels;
    std::array<std::unique_ptr<SliderAttachment>, 6> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarshNoiseAudioProcessorEditor)
};
