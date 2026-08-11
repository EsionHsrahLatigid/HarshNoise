#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// BRUTAL LOOK AND FEEL
//==============================================================================

HarshNoiseAudioProcessorEditor::BrutalLookAndFeel::BrutalLookAndFeel()
{
    // Dark brutalist color scheme
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFCC0000));  // Dark red
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF333333));  // Dark gray
    setColour(juce::Slider::thumbColourId, juce::Colour(0xFFFF3333));  // Bright red
    
    setColour(juce::Label::textColourId, juce::Colour(0xFFCCCCCC));  // Light gray text
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
}

void HarshNoiseAudioProcessorEditor::BrutalLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& /*slider*/)
{
    const float radius = static_cast<float>(juce::jmin(width / 2, height / 2)) - 4.0f;
    const float centreX = static_cast<float>(x + width) * 0.5f;
    const float centreY = static_cast<float>(y + height) * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    // Background circle (dark)
    g.setColour(juce::Colour(0xFF1A1A1A));
    g.fillEllipse(rx, ry, rw, rw);
    
    // Outer ring
    g.setColour(juce::Colour(0xFF333333));
    g.drawEllipse(rx, ry, rw, rw, 2.0f);
    
    // Value arc (red)
    juce::Path arcPath;
    arcPath.addCentredArc(centreX, centreY, radius - 4.0f, radius - 4.0f,
                          0.0f, rotaryStartAngle, angle, true);
    
    g.setColour(juce::Colour(0xFFCC0000));
    g.strokePath(arcPath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    
    // Pointer line
    juce::Path pointerPath;
    const float pointerLength = radius * 0.6f;
    const float pointerThickness = 3.0f;
    
    pointerPath.addRectangle(-pointerThickness * 0.5f, -radius + 6.0f,
                              pointerThickness, pointerLength);
    
    g.setColour(juce::Colour(0xFFFF3333));
    g.fillPath(pointerPath, juce::AffineTransform::rotation(angle)
                                                   .translated(centreX, centreY));
    
    // Center dot
    g.setColour(juce::Colour(0xFF666666));
    g.fillEllipse(centreX - 4.0f, centreY - 4.0f, 8.0f, 8.0f);
}

void HarshNoiseAudioProcessorEditor::BrutalLookAndFeel::drawLabel(
    juce::Graphics& g, juce::Label& label)
{
    g.setColour(label.findColour(juce::Label::textColourId));
    
    auto font = juce::Font(juce::FontOptions()
                            .withHeight(12.0f)
                            .withStyle("Bold"));
    g.setFont(font);
    
    auto textArea = label.getLocalBounds();
    g.drawText(label.getText(), textArea, juce::Justification::centred, true);
}

//==============================================================================
// EDITOR IMPLEMENTATION
//==============================================================================

HarshNoiseAudioProcessorEditor::HarshNoiseAudioProcessorEditor(HarshNoiseAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&brutalLookAndFeel);
    
    // Configure all sliders
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& name)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFAAAAAA));
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF1A1A1A));
        addAndMakeVisible(slider);
        
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };
    
    setupSlider(crushSlider, crushLabel, "CRUSH");
    setupSlider(downsampleSlider, downsampleLabel, "DECIMATE");
    setupSlider(feedbackSlider, feedbackLabel, "FEEDBACK");
    setupSlider(chaosSlider, chaosLabel, "CHAOS");
    setupSlider(mixSlider, mixLabel, "MIX");
    setupSlider(outputSlider, outputLabel, "OUTPUT");
    
    // Create attachments
    crushAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "crush", crushSlider);
    downsampleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "downsample", downsampleSlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "feedback", feedbackSlider);
    chaosAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "chaos", chaosSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "mix", mixSlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "output", outputSlider);
    
    // Start timer for metering and glitch animation
    startTimerHz(30);
    
    setSize(450, 280);
}

HarshNoiseAudioProcessorEditor::~HarshNoiseAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

//==============================================================================
void HarshNoiseAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background - dark with subtle noise texture
    g.fillAll(juce::Colour(0xFF0D0D0D));
    
    // Add some visual noise/grain for brutalist aesthetic
    juce::Random random;
    random.setSeed(glitchFrame);
    
    if (isGlitching)
    {
        // Glitch effect - random colored rectangles
        for (int i = 0; i < 5; ++i)
        {
            int x = random.nextInt(getWidth());
            int y = random.nextInt(getHeight());
            int w = random.nextInt(100) + 20;
            int h = random.nextInt(10) + 2;
            
            g.setColour(juce::Colour::fromHSV(random.nextFloat(), 0.8f, 0.5f, 0.3f));
            g.fillRect(x, y, w, h);
        }
    }
    
    // Title
    g.setColour(juce::Colour(0xFFCC0000));
    g.setFont(juce::Font(juce::FontOptions()
                          .withHeight(24.0f)
                          .withStyle("Bold")));
    g.drawText("HARSH NOISE", getLocalBounds().removeFromTop(40), juce::Justification::centred, true);
    
    // Subtitle
    g.setColour(juce::Colour(0xFF666666));
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    g.drawText("DIGITAL DESTRUCTION UNIT", 0, 32, getWidth(), 15, juce::Justification::centred, true);
    
    // Output meter (right side)
    auto meterArea = getLocalBounds().removeFromRight(20).reduced(5, 60);
    
    // Meter background
    g.setColour(juce::Colour(0xFF1A1A1A));
    g.fillRect(meterArea);
    
    // Meter level
    float meterHeight = meterArea.getHeight() * std::min(1.0f, meterLevel);
    auto levelRect = meterArea.removeFromBottom(static_cast<int>(meterHeight));
    
    // Color gradient based on level
    juce::Colour meterColor;
    if (meterLevel > 0.9f)
        meterColor = juce::Colour(0xFFFF0000);  // Red (clipping)
    else if (meterLevel > 0.7f)
        meterColor = juce::Colour(0xFFFF6600);  // Orange
    else
        meterColor = juce::Colour(0xFFCC0000);  // Dark red
    
    g.setColour(meterColor);
    g.fillRect(levelRect);
    
    // Dividing lines between controls
    g.setColour(juce::Colour(0xFF222222));
    for (int i = 1; i < 6; ++i)
    {
        int x = 15 + (i * 70);
        g.drawVerticalLine(x, 50.0f, static_cast<float>(getHeight() - 10));
    }
    
    // Footer
    g.setColour(juce::Colour(0xFF444444));
    g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
    g.drawText("v1.0 // BRUTAL AUDIO", 0, getHeight() - 15, getWidth() - 25, 12,
               juce::Justification::centredRight, true);
}

void HarshNoiseAudioProcessorEditor::resized()
{
    const int sliderSize = 65;
    const int labelHeight = 18;
    const int topMargin = 55;
    const int spacing = 70;
    
    auto area = getLocalBounds();
    area.removeFromTop(topMargin);
    area.removeFromRight(25);  // Space for meter
    area.removeFromLeft(15);
    
    // Layout sliders in a row
    auto placeControl = [&](juce::Slider& slider, juce::Label& label, int index)
    {
        auto controlArea = area.withX(15 + index * spacing).withWidth(sliderSize);
        
        label.setBounds(controlArea.removeFromTop(labelHeight));
        controlArea.removeFromTop(5);
        slider.setBounds(controlArea.withHeight(sliderSize + 30));
    };
    
    placeControl(crushSlider, crushLabel, 0);
    placeControl(downsampleSlider, downsampleLabel, 1);
    placeControl(feedbackSlider, feedbackLabel, 2);
    placeControl(chaosSlider, chaosLabel, 3);
    placeControl(mixSlider, mixLabel, 4);
    placeControl(outputSlider, outputLabel, 5);
}

void HarshNoiseAudioProcessorEditor::timerCallback()
{
    // Update meter level with smoothing
    float newLevel = audioProcessor.outputLevel.load();
    meterLevel = meterLevel * 0.8f + newLevel * 0.2f;
    
    // Trigger visual glitch based on chaos and level
    float chaos = audioProcessor.getAPVTS().getRawParameterValue("chaos")->load();
    
    glitchFrame++;
    
    if (chaos > 0.5f && meterLevel > 0.5f)
    {
        juce::Random r;
        r.setSeed(static_cast<int64_t>(juce::Time::currentTimeMillis()));
        isGlitching = r.nextFloat() < chaos * 0.3f;
    }
    else
    {
        isGlitching = false;
    }
    
    repaint();
}
