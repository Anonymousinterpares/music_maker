#pragma once

#include <JuceHeader.h>
#include "SynthEngine.h"

class InternalSynthProcessor : public juce::AudioProcessor
{
public:
    InternalSynthProcessor()
        : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {
        for (int i = 0; i < 8; ++i)
            synth.addVoice (new SynthVoice()); // This requires SynthVoice to be accessible
        
        synth.addSound (new SynthSound());
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        synth.setCurrentPlaybackSampleRate (sampleRate);
        for (int i = 0; i < synth.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice(i)))
                v->prepare (sampleRate);
    }

    void releaseResources() override {}

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override
    {
        synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    void noteOn (int midiNoteNumber, float velocity)
    {
        synth.noteOn (1, midiNoteNumber, velocity);
    }

    void noteOff (int midiNoteNumber, float velocity, bool allowTailOff)
    {
        synth.noteOff (1, midiNoteNumber, velocity, allowTailOff);
    }

    void allNotesOff()
    {
        synth.allNotesOff (0, false); // Channel 0 (all channels), allowTailOff = false
    }

    // Boilerplate
    const juce::String getName() const override { return "Internal Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    void updateParameters (int type, float cutoff, float resonance)
    {
        for (int i = 0; i < synth.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice(i)))
                v->updateParameters (type, cutoff, resonance);
    }

private:
    juce::Synthesiser synth;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalSynthProcessor)
};
