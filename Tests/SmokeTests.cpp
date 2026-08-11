#include "PluginProcessor.h"

#include <cmath>
#include <iostream>

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
}

int main()
{
    HarshNoiseAudioProcessor processor;
    processor.prepareToPlay(44100.0, 128);

    juce::AudioBuffer<float> buffer(2, 128);
    juce::MidiBuffer midi;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float value = std::sin(static_cast<float>(sample) * 0.05f) * 0.2f;
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, -value);
    }

    processor.processBlock(buffer, midi);

    if (!bufferIsFinite(buffer))
    {
        std::cerr << "HarshNoise produced a non-finite sample\n";
        return 1;
    }

    processor.releaseResources();
    return 0;
}
