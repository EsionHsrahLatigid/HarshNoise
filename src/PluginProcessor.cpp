#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
float sanitizeAudioSample(float value)
{
    if (!std::isfinite(value))
        return 0.0f;

    return std::clamp(value, -16.0f, 16.0f);
}

float foldToUnitRange(float value)
{
    if (!std::isfinite(value))
        return 0.0f;

    float folded = std::fmod(value + 1.0f, 4.0f);
    if (folded < 0.0f)
        folded += 4.0f;

    return folded <= 2.0f ? folded - 1.0f : 3.0f - folded;
}
}

//==============================================================================
HarshNoiseAudioProcessor::HarshNoiseAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Get parameter pointers
    crushParam = apvts.getRawParameterValue("crush");
    downsampleParam = apvts.getRawParameterValue("downsample");
    feedbackParam = apvts.getRawParameterValue("feedback");
    chaosParam = apvts.getRawParameterValue("chaos");
    mixParam = apvts.getRawParameterValue("mix");
    outputParam = apvts.getRawParameterValue("output");
    
    // Add listeners
    apvts.addParameterListener("crush", this);
    apvts.addParameterListener("downsample", this);
    apvts.addParameterListener("feedback", this);
    apvts.addParameterListener("chaos", this);
    apvts.addParameterListener("mix", this);
    apvts.addParameterListener("output", this);
    
    // Initialize random generator with time-based seed
    rng.seed(static_cast<unsigned int>(std::time(nullptr)));
    
    // Clear buffers
    delayBufferL.fill(0.0f);
    delayBufferR.fill(0.0f);
    glitchBuffer.fill(0.0f);
}

HarshNoiseAudioProcessor::~HarshNoiseAudioProcessor()
{
    apvts.removeParameterListener("crush", this);
    apvts.removeParameterListener("downsample", this);
    apvts.removeParameterListener("feedback", this);
    apvts.removeParameterListener("chaos", this);
    apvts.removeParameterListener("mix", this);
    apvts.removeParameterListener("output", this);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout HarshNoiseAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // CRUSH - Bit depth reduction (1-16 bits)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("crush", 1),
        "CRUSH",
        juce::NormalisableRange<float>(1.0f, 16.0f, 0.1f, 0.5f),
        8.0f,
        juce::AudioParameterFloatAttributes().withLabel("bits")
    ));
    
    // DOWNSAMPLE - Sample rate division factor (1-64x)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("downsample", 1),
        "DECIMATE",
        juce::NormalisableRange<float>(1.0f, 64.0f, 1.0f, 0.4f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel("x")
    ));
    
    // FEEDBACK - Aggressive feedback amount (0-150%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("feedback", 1),
        "FEEDBACK",
        juce::NormalisableRange<float>(0.0f, 1.5f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));
    
    // CHAOS - Unpredictable modulation/glitch intensity
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("chaos", 1),
        "CHAOS",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));
    
    // MIX - Dry/Wet balance
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mix", 1),
        "MIX",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));
    
    // OUTPUT - Output gain (with boost capability)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("output", 1),
        "OUTPUT",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));
    
    return { params.begin(), params.end() };
}

void HarshNoiseAudioProcessor::parameterChanged(const juce::String& /*parameterID*/, float /*newValue*/)
{
    // Parameters are read directly via atomic pointers
}

//==============================================================================
const juce::String HarshNoiseAudioProcessor::getName() const { return "HarshNoise"; }
bool HarshNoiseAudioProcessor::acceptsMidi() const { return false; }
bool HarshNoiseAudioProcessor::producesMidi() const { return false; }
bool HarshNoiseAudioProcessor::isMidiEffect() const { return false; }
double HarshNoiseAudioProcessor::getTailLengthSeconds() const { return 0.5; }
int HarshNoiseAudioProcessor::getNumPrograms() { return 1; }
int HarshNoiseAudioProcessor::getCurrentProgram() { return 0; }
void HarshNoiseAudioProcessor::setCurrentProgram(int) {}
const juce::String HarshNoiseAudioProcessor::getProgramName(int) { return {}; }
void HarshNoiseAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void HarshNoiseAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    
    // Reset all state
    crushHold[0] = crushHold[1] = 0.0f;
    crushCounter[0] = crushCounter[1] = 0;
    
    delayBufferL.fill(0.0f);
    delayBufferR.fill(0.0f);
    delayWritePos = 0;
    feedbackState[0] = feedbackState[1] = 0.0f;
    
    glitchBuffer.fill(0.0f);
    glitchPos = 0;
    glitchActive = false;
    glitchCounter = 0;
    
    dcIn[0] = dcIn[1] = 0.0f;
    dcOut[0] = dcOut[1] = 0.0f;
    
    // Initialize chaos with slight randomness
    chaosState[0] = 0.1f + dist(rng) * 0.01f;
    chaosState[1] = 0.2f + dist(rng) * 0.01f;
    chaosState[2] = 0.3f + dist(rng) * 0.01f;
    chaosState[3] = 0.4f + dist(rng) * 0.01f;
}

void HarshNoiseAudioProcessor::releaseResources() {}

bool HarshNoiseAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

//==============================================================================
// DSP PROCESSING FUNCTIONS
//==============================================================================

float HarshNoiseAudioProcessor::bitCrush(float input, float bits)
{
    input = sanitizeAudioSample(input);
    bits = std::clamp(std::isfinite(bits) ? bits : 8.0f, 1.0f, 16.0f);

    // Quantize to specified bit depth
    // Lower bits = more aggressive quantization noise
    const float levels = std::pow(2.0f, bits);
    const float scale = levels - 1.0f;
    
    // Quantize with rounding toward zero for harsher sound
    float quantized = std::floor(input * scale + 0.5f) / scale;
    
    // Add quantization noise emphasis at low bit depths
    if (bits < 4.0f)
    {
        float noise = (input - quantized) * (4.0f - bits);
        quantized += noise * 0.5f;
    }
    
    return quantized;
}

float HarshNoiseAudioProcessor::downsample(float input, int channel, int factor)
{
    input = sanitizeAudioSample(input);

    if (factor <= 1)
        return input;
    
    crushCounter[channel]++;
    
    if (crushCounter[channel] >= factor)
    {
        crushCounter[channel] = 0;
        crushHold[channel] = input;
    }
    
    return crushHold[channel];
}

float HarshNoiseAudioProcessor::waveshape(float input, float drive)
{
    input = sanitizeAudioSample(input);
    drive = std::clamp(std::isfinite(drive) ? drive : 0.0f, 0.0f, 1.0f);

    // Multi-stage aggressive waveshaping
    float x = input * (1.0f + drive * 10.0f);
    
    // Stage 1: Hard asymmetric clipping (rectifier-like distortion)
    if (x > 0.3f)
        x = 0.3f + (x - 0.3f) * 0.1f;
    if (x < -0.5f)
        x = -0.5f + (x + 0.5f) * 0.05f;
    
    // Stage 2: bounded triangle folding, replacing the old unbounded loop.
    x = foldToUnitRange(x);
    
    // Stage 3: Add harsh harmonics via triangle wave folding
    if (drive > 0.3f)
    {
        float fold = std::sin(x * 3.14159f * (1.0f + drive * 3.0f));
        x = x * (1.0f - drive * 0.5f) + fold * drive * 0.5f;
    }
    
    return x;
}

float HarshNoiseAudioProcessor::updateChaos()
{
    // Lorenz-inspired chaotic attractor (simplified for audio rate)
    // Creates unpredictable but continuous modulation
    const float sigma = 10.0f;
    const float rho = 28.0f;
    const float beta = 8.0f / 3.0f;
    const float dt = 0.001f;
    
    float x = sanitizeAudioSample(chaosState[0]);
    float y = sanitizeAudioSample(chaosState[1]);
    float z = sanitizeAudioSample(chaosState[2]);
    
    float dx = sigma * (y - x);
    float dy = x * (rho - z) - y;
    float dz = x * y - beta * z;
    
    chaosState[0] = sanitizeAudioSample(x + dx * dt);
    chaosState[1] = sanitizeAudioSample(y + dy * dt);
    chaosState[2] = sanitizeAudioSample(z + dz * dt);
    
    // Normalize output to -1..1 range
    return std::tanh(chaosState[0] * 0.1f);
}

float HarshNoiseAudioProcessor::glitchProcess(float input, float chaosAmount)
{
    input = sanitizeAudioSample(input);
    chaosAmount = std::clamp(std::isfinite(chaosAmount) ? chaosAmount : 0.0f, 0.0f, 1.0f);

    if (chaosAmount < 0.01f)
        return input;
    
    // Random glitch triggering based on chaos
    float chaosVal = updateChaos();
    
    // Update glitch buffer
    glitchBuffer[glitchPos] = input;
    glitchPos = (glitchPos + 1) % GLITCH_BUFFER_SIZE;
    
    // Trigger glitch randomly
    if (!glitchActive && std::abs(chaosVal) > (1.0f - chaosAmount * 0.8f))
    {
        glitchActive = true;
        glitchCounter = 0;
        glitchLength = 16 + static_cast<int>(std::abs(dist(rng)) * chaosAmount * 512.0f);
    }
    
    if (glitchActive)
    {
        glitchCounter++;
        
        // Read from random position in buffer (stutter/repeat effect)
        int readPos = (glitchPos - glitchLength + GLITCH_BUFFER_SIZE) % GLITCH_BUFFER_SIZE;
        readPos = (readPos + (glitchCounter % glitchLength)) % GLITCH_BUFFER_SIZE;
        
        float glitched = glitchBuffer[readPos];
        
        // Add digital artifacts
        if (chaosAmount > 0.5f)
        {
            // Bit flip simulation
            if (dist(rng) > (1.0f - chaosAmount * 0.3f))
            {
                glitched = -glitched;
            }
            
            // Random DC offset bursts
            if (dist(rng) > 0.95f)
            {
                glitched += dist(rng) * chaosAmount;
            }
        }
        
        if (glitchCounter >= glitchLength * 2)
        {
            glitchActive = false;
        }
        
        // Mix glitched signal
        return sanitizeAudioSample(input * (1.0f - chaosAmount * 0.7f) + glitched * chaosAmount * 0.7f);
    }
    
    return input;
}

float HarshNoiseAudioProcessor::processFeedback(float input, int channel, float fbAmount, float chaosAmount)
{
    input = sanitizeAudioSample(input);
    fbAmount = std::clamp(std::isfinite(fbAmount) ? fbAmount : 0.0f, 0.0f, 1.5f);
    chaosAmount = std::clamp(std::isfinite(chaosAmount) ? chaosAmount : 0.0f, 0.0f, 1.0f);

    if (fbAmount < 0.01f)
        return input;
    
    // Read from delay line with very short delay (creates comb filtering / harsh resonance)
    int delayTime = 32 + static_cast<int>(chaosAmount * 200.0f);  // 32-232 samples
    
    // Add chaos modulation to delay time
    if (chaosAmount > 0.1f)
    {
        delayTime += static_cast<int>(updateChaos() * chaosAmount * 50.0f);
        delayTime = std::clamp(delayTime, 8, MAX_DELAY - 1);
    }
    
    int readPos = (delayWritePos - delayTime + MAX_DELAY) % MAX_DELAY;
    
    auto& delayBuffer = (channel == 0) ? delayBufferL : delayBufferR;
    float delayed = sanitizeAudioSample(delayBuffer[readPos]);
    
    // Aggressive feedback with soft saturation
    float fb = delayed * fbAmount;
    
    // Add instability at high feedback (>100%)
    if (fbAmount > 1.0f)
    {
        float excess = fbAmount - 1.0f;
        fb += dist(rng) * excess * 0.1f;  // Add noise
        fb = std::tanh(fb * 2.0f) * 0.7f;  // Limit but allow some chaos
    }
    
    feedbackState[channel] = fb;
    
    // Combine input with feedback
    float output = input + fb;
    
    // Harsh limiting to prevent runaway
    output = std::tanh(output);
    
    return output;
}

float HarshNoiseAudioProcessor::dcBlock(float input, int channel)
{
    input = sanitizeAudioSample(input);

    // High-pass filter to remove DC offset
    // y[n] = x[n] - x[n-1] + R * y[n-1], R close to 1
    const float R = 0.995f;
    float output = sanitizeAudioSample(input - sanitizeAudioSample(dcIn[channel]) + R * sanitizeAudioSample(dcOut[channel]));
    dcIn[channel] = input;
    dcOut[channel] = output;
    return output;
}

float HarshNoiseAudioProcessor::softClip(float input)
{
    input = sanitizeAudioSample(input);

    // Final soft clipper / limiter
    return std::tanh(input * 1.5f) * 0.8f;
}

//==============================================================================
void HarshNoiseAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Get current parameter values
    const float crush = crushParam->load();
    const int downsampleFactor = static_cast<int>(downsampleParam->load());
    const float feedback = feedbackParam->load();
    const float chaos = chaosParam->load();
    const float mix = mixParam->load();
    const float outputGain = juce::Decibels::decibelsToGain(outputParam->load());
    
    // Pre-calculate chaos influence on processing
    const float chaosModulation = chaos > 0.01f ? updateChaos() : 0.0f;
    
    // Modulate crush amount with chaos
    float crushMod = crush;
    if (chaos > 0.1f)
    {
        crushMod = std::clamp(crush + chaosModulation * chaos * 8.0f, 1.0f, 16.0f);
    }
    
    float peakLevel = 0.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            float dry = sanitizeAudioSample(buffer.getSample(channel, sample));
            float wet = dry;
            
            // 1. Bit crushing (quantization noise)
            wet = bitCrush(wet, crushMod);
            
            // 2. Downsampling (aliasing, stepping)
            wet = downsample(wet, channel, downsampleFactor);
            
            // 3. Waveshaping / Distortion
            float drive = chaos * 0.5f + (1.0f - crush / 16.0f) * 0.3f;
            wet = waveshape(wet, drive);
            
            // 4. Feedback processing
            wet = processFeedback(wet, channel, feedback, chaos);
            
            // 5. Glitch processing
            wet = glitchProcess(wet, chaos);
            
            // 6. Write to delay buffer (after processing for feedback path)
            auto& delayBuffer = (channel == 0) ? delayBufferL : delayBufferR;
            delayBuffer[delayWritePos % MAX_DELAY] = wet;
            
            // 7. DC blocking
            wet = dcBlock(wet, channel);
            
            // 8. Final soft clipping
            wet = softClip(wet);
            
            // 9. Dry/Wet mix
            float output = dry * (1.0f - mix) + wet * mix;
            
            // 10. Output gain
            output *= outputGain;
            
            // Update peak level for metering
            peakLevel = std::max(peakLevel, std::abs(output));
            
            buffer.setSample(channel, sample, sanitizeAudioSample(output));
        }
        
        // Advance delay write position
        delayWritePos = (delayWritePos + 1) % MAX_DELAY;
    }
    
    // Update output level for UI metering (smoothed)
    outputLevel.store(outputLevel.load() * 0.9f + peakLevel * 0.1f);
}

//==============================================================================
bool HarshNoiseAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* HarshNoiseAudioProcessor::createEditor()
{
    return new HarshNoiseAudioProcessorEditor(*this);
}

//==============================================================================
void HarshNoiseAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HarshNoiseAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HarshNoiseAudioProcessor();
}
