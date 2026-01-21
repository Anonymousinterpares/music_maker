#pragma once

#include <JuceHeader.h>

// A simple Synthesizer Voice
class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice() {}

    bool canPlaySound (juce::SynthesiserSound* sound) override { return dynamic_cast<juce::SynthesiserSound*> (sound) != nullptr; }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        level = velocity * 0.15f;
        auto cyclesPerSample = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber) / getSampleRate();
        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
        currentAngle = 0.0;
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        adsr.noteOff();
        if (!allowTailOff) clearCurrentNote();
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (!adsr.isActive()) {
            clearCurrentNote();
            return;
        }

        for (int sample = 0; sample < numSamples; ++sample)
        {
            auto envelopeValue = adsr.getNextSample();
            
            // Sawtooth: (2.0 * angle / 2PI) - 1.0
            auto sampleValue = (float) (((currentAngle / juce::MathConstants<double>::pi) - 1.0) * level * envelopeValue);
            
            for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
                outputBuffer.addSample (channel, startSample + sample, sampleValue);

            currentAngle += angleDelta;
            if (currentAngle >= juce::MathConstants<double>::twoPi)
                currentAngle -= juce::MathConstants<double>::twoPi;
        }
    }

    void prepare (double sampleRate)
    {
        adsr.setSampleRate (sampleRate);
        juce::ADSR::Parameters params;
        params.attack = 0.05f;
        params.decay = 0.1f;
        params.sustain = 0.8f;
        params.release = 0.5f;
        adsr.setParameters (params);
    }

private:
    juce::ADSR adsr;
    double currentAngle = 0.0, angleDelta = 0.0;
    float level = 0.0f;
};

// A simple Synthesizer Sound
struct SynthSound : public juce::SynthesiserSound
{
    SynthSound() {}
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class MainComponent  : public juce::AudioAppComponent
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::TextButton nativeBtn;
    std::unique_ptr<juce::WebBrowserComponent> webBrowser;
    
    // Synth Core
    juce::Synthesiser synth;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
