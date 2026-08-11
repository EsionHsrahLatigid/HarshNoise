#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * HarshNoise Editor - Brutalist minimal UI
 * 
 * Inspired by early 2000s experimental software aesthetics
 * Dark theme with aggressive red accents
 */
class HarshNoiseAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit HarshNoiseAudioProcessorEditor(HarshNoiseAudioProcessor&);
    ~HarshNoiseAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    
    // Custom slider look
    class BrutalLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        BrutalLookAndFeel();
        
        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle,
                              float rotaryEndAngle, juce::Slider& slider) override;
        
        void drawLabel(juce::Graphics& g, juce::Label& label) override;
    };
    
    HarshNoiseAudioProcessor& audioProcessor;
    BrutalLookAndFeel brutalLookAndFeel;
    
    // Parameter controls
    juce::Slider crushSlider;
    juce::Slider downsampleSlider;
    juce::Slider feedbackSlider;
    juce::Slider chaosSlider;
    juce::Slider mixSlider;
    juce::Slider outputSlider;
    
    juce::Label crushLabel;
    juce::Label downsampleLabel;
    juce::Label feedbackLabel;
    juce::Label chaosLabel;
    juce::Label mixLabel;
    juce::Label outputLabel;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crushAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> downsampleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chaosAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    
    // Metering
    float meterLevel = 0.0f;
    
    // Glitch animation state
    int glitchFrame = 0;
    bool isGlitching = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarshNoiseAudioProcessorEditor)
};
