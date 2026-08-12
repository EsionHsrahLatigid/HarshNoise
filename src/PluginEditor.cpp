#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
struct ControlSpec
{
    const char* id;
    const char* label;
    const char* tip;
};

constexpr ControlSpec controls[] {
    { "crush", "CRUSH", "Bit depth reduction from full resolution to coarse quantization." },
    { "downsample", "DECIMATE", "Sample hold division for rate reduction." },
    { "feedback", "FDBK", "Short unstable feedback amount." },
    { "chaos", "CHAOS", "Chaotic modulation and stutter probability." },
    { "mix", "MIX", "Dry to destroyed blend." },
    { "output", "OUTPUT", "Final output gain in dB." },
};

static_assert(std::size(controls) == 6);

float normalizedSliderValue(juce::Slider& slider) noexcept
{
    const auto normalized = static_cast<float>(slider.valueToProportionOfLength(slider.getValue()));
    return std::isfinite(normalized) ? juce::jlimit(0.0f, 1.0f, normalized) : 0.0f;
}
} // namespace

HarshNoiseAudioProcessorEditor::HarshNoiseAudioProcessorEditor(HarshNoiseAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      tooltipText("HarshNoise: crush, decimate, feedback, chaos, mix, and output controls in the EHL monochrome 8-bit system.")
{
    setLookAndFeel(&lookAndFeel);
    setResizeLimits(minimumWidth, minimumHeight,
                    ehl::juce_design::Metrics::maximumWidth,
                    ehl::juce_design::Metrics::maximumHeight);
    setResizable(true, true);
    setName("HarshNoise editor");
    setComponentID("harshnoise-editor");
    setTitle("HarshNoise");
    setDescription("HarshNoise monochrome 8-bit distortion editor");
    setWantsKeyboardFocus(true);

    parameterDisplay.setComponentID("harshnoise-parameter-display");
    parameterDisplay.setName("HarshNoise parameter display");
    parameterDisplay.setInterceptsMouseClicks(false, false);
    parameterDisplay.setWantsKeyboardFocus(false);
    addAndMakeVisible(parameterDisplay);

    for (int i = 0; i < static_cast<int>(std::size(controls)); ++i)
        addControl(i, controls[i].id, controls[i].label, controls[i].tip);

    updateParameterDisplay();
    startTimerHz(30);
    setSize(defaultWidth, defaultHeight);
}

HarshNoiseAudioProcessorEditor::~HarshNoiseAudioProcessorEditor()
{
    stopTimer();
    for (auto& slider : sliders)
        slider.setLookAndFeel(nullptr);
    for (auto& label : labels)
        label.setLookAndFeel(nullptr);
    tooltipWindow.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void HarshNoiseAudioProcessorEditor::addControl(int index, const juce::String& parameterId, const juce::String& labelText, const juce::String& tip)
{
    auto& slider = sliders[static_cast<std::size_t>(index)];
    ehl::juce_design::styleSlider(slider);
    slider.setComponentID("harshnoise-control-" + parameterId);
    slider.setName("HarshNoise " + labelText);
    slider.setTitle(labelText);
    slider.setDescription(tip);
    slider.setTooltip(tip);
    slider.setWantsKeyboardFocus(true);
    addAndMakeVisible(slider);

    auto& label = labels[static_cast<std::size_t>(index)];
    label.setText(labelText, juce::dontSendNotification);
    ehl::juce_design::styleLabel(label);
    label.setComponentID("harshnoise-label-" + parameterId);
    label.setName(labelText);
    label.setTooltip(tip);
    addAndMakeVisible(label);

    attachments[static_cast<std::size_t>(index)] = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), parameterId, slider);
}

void HarshNoiseAudioProcessorEditor::paint(juce::Graphics& g)
{
    ehl::juce_design::paintEditorChrome(g, getLocalBounds(), "HarshNoise", "DISTORTION");
}

void HarshNoiseAudioProcessorEditor::resized()
{
    parameterDisplay.setBounds(ehl::juce_design::parameterDisplayArea(getLocalBounds()));

    for (int i = 0; i < static_cast<int>(sliders.size()); ++i)
        ehl::juce_design::layoutLabelledControl(labels[static_cast<std::size_t>(i)],
                                                sliders[static_cast<std::size_t>(i)],
                                                ehl::juce_design::controlCell(getLocalBounds(), static_cast<std::size_t>(i)));
}

void HarshNoiseAudioProcessorEditor::timerCallback()
{
    updateParameterDisplay();
}

void HarshNoiseAudioProcessorEditor::updateParameterDisplay()
{
    parameterDisplay.setValues({ normalizedSliderValue(sliders[0]),
                                 normalizedSliderValue(sliders[1]),
                                 normalizedSliderValue(sliders[2]),
                                 normalizedSliderValue(sliders[3]) });
}
