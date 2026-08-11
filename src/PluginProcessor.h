#pragma once

#include <JuceHeader.h>
#include <random>
#include <array>

//==============================================================================
/**
 * HarshNoise - Brutal Digital Noise Generator
 * 
 * mego/Farmers Manual/Pita inspired aggressive digital destruction
 * Classic Mac OS 9 era harsh digital aesthetics
 */
class HarshNoiseAudioProcessor : public juce::AudioProcessor,
                                  public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    HarshNoiseAudioProcessor();
    ~HarshNoiseAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    // For metering in UI
    std::atomic<float> outputLevel { 0.0f };

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // DSP parameters (atomic for thread safety)
    std::atomic<float>* crushParam = nullptr;
    std::atomic<float>* downsampleParam = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* chaosParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* outputParam = nullptr;
    
    // Internal state
    double currentSampleRate = 44100.0;
    
    // Bit crusher state
    float crushHold[2] = { 0.0f, 0.0f };
    int crushCounter[2] = { 0, 0 };
    
    // Feedback delay line (short, aggressive)
    static constexpr int MAX_DELAY = 4096;
    std::array<float, MAX_DELAY> delayBufferL;
    std::array<float, MAX_DELAY> delayBufferR;
    int delayWritePos = 0;
    float feedbackState[2] = { 0.0f, 0.0f };
    
    // Chaos generator state
    float chaosState[4] = { 0.1f, 0.2f, 0.3f, 0.4f };  // Lorenz-like attractor
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist { -1.0f, 1.0f };
    
    // Glitch buffer for stuttering
    static constexpr int GLITCH_BUFFER_SIZE = 2048;
    std::array<float, GLITCH_BUFFER_SIZE> glitchBuffer;
    int glitchPos = 0;
    int glitchLength = 64;
    bool glitchActive = false;
    int glitchCounter = 0;
    
    // DC blocker state
    float dcIn[2] = { 0.0f, 0.0f };
    float dcOut[2] = { 0.0f, 0.0f };
    
    //==============================================================================
    // DSP Processing Functions
    
    // Aggressive bit crushing with configurable resolution
    float bitCrush(float input, float bits);
    
    // Sample rate reduction (classic lo-fi)
    float downsample(float input, int channel, int factor);
    
    // Harsh waveshaping with multiple modes
    float waveshape(float input, float drive);
    
    // Chaotic modulation source (attractor-based)
    float updateChaos();
    
    // Glitch/stutter processor
    float glitchProcess(float input, float chaosAmount);
    
    // Feedback processor with instability
    float processFeedback(float input, int channel, float fbAmount, float chaosAmount);
    
    // DC blocking filter
    float dcBlock(float input, int channel);
    
    // Soft clip for final limiting
    float softClip(float input);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarshNoiseAudioProcessor)
};
