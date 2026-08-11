#include "PluginProcessor.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
bool bufferIsFinite(const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* samples = buffer.getReadPointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            if (!std::isfinite(samples[sample]))
                return false;
        }
    }
    return true;
}

void setParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float normalizedValue)
{
    if (auto* parameter = apvts.getParameter(id))
        parameter->setValueNotifyingHost(normalizedValue);
}

bool runBlock(HarshNoiseAudioProcessor& processor, int channels, int samples, float scale)
{
    juce::AudioBuffer<float> buffer(channels, samples);
    juce::MidiBuffer midi;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float value = std::sin(static_cast<float>(sample) * 0.05f) * scale;
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample(channel, sample, channel == 0 ? value : -value);
    }

    processor.processBlock(buffer, midi);
    return bufferIsFinite(buffer);
}
}

int main()
{
    HarshNoiseAudioProcessor processor;
    processor.prepareToPlay(44100.0, 128);
    setParameter(processor.getAPVTS(), "crush", 0.0f);
    setParameter(processor.getAPVTS(), "downsample", 1.0f);
    setParameter(processor.getAPVTS(), "feedback", 1.0f);
    setParameter(processor.getAPVTS(), "chaos", 1.0f);
    setParameter(processor.getAPVTS(), "mix", 1.0f);
    setParameter(processor.getAPVTS(), "output", 1.0f);

    if (!runBlock(processor, 2, 128, 0.2f))
    {
        std::cerr << "HarshNoise normal block produced a non-finite sample\n";
        return 1;
    }

    for (int samples : { 1, 7, 64, 257, 1024 })
    {
        if (!runBlock(processor, 2, samples, 8.0f))
        {
            std::cerr << "HarshNoise variable block produced a non-finite sample\n";
            return 1;
        }
    }

    juce::AudioBuffer<float> buffer(2, 16);
    juce::MidiBuffer midi;
    const float badValues[] = {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    };

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float value = badValues[sample % 5];
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, -value);
    }

    processor.processBlock(buffer, midi);

    if (!bufferIsFinite(buffer))
    {
        std::cerr << "HarshNoise non-finite/extreme regression failed\n";
        return 1;
    }

    juce::MemoryBlock state;
    processor.getStateInformation(state);

    HarshNoiseAudioProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    restored.prepareToPlay(48000.0, 64);

    if (!runBlock(restored, 1, 64, 0.5f))
    {
        std::cerr << "HarshNoise state round-trip processor failed\n";
        return 1;
    }

    processor.releaseResources();
    restored.releaseResources();
    return 0;
}
