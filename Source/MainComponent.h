#pragma once

#include <JuceHeader.h>
#include "ProjectModel.h"

// A simple Synthesizer Voice
class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice() {}

    bool canPlaySound (juce::SynthesiserSound* sound) override { return dynamic_cast<juce::SynthesiserSound*> (sound) != nullptr; }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        level = velocity * 0.25f;
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
            float rawSample = 0.0f;

            switch (oscType)
            {
                case 0: rawSample = (float) std::sin (currentAngle); break; // Sine
                case 1: rawSample = (float) ((currentAngle / juce::MathConstants<double>::pi) - 1.0); break; // Saw
                case 2: rawSample = currentAngle < juce::MathConstants<double>::pi ? 1.0f : -1.0f; break; // Square
                case 3: rawSample = (float) (2.0 * std::abs (2.0 * (currentAngle / juce::MathConstants<double>::twoPi) - 1.0) - 1.0); break; // Triangle
                default: break;
            }

            auto sampleValue = rawSample * level * envelopeValue;
            
            // Apply filter to each sample (simplified for MVP)
            sampleValue = filter.processSample (0, sampleValue);
            
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
        juce::ADSR::Parameters adsrParams;
        adsrParams.attack = 0.05f;
        adsrParams.decay = 0.1f;
        adsrParams.sustain = 0.8f;
        adsrParams.release = 0.5f;
        adsr.setParameters (adsrParams);

        juce::dsp::ProcessSpec spec { sampleRate, 512, 2 };
        filter.prepare (spec);
        filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    void updateParameters (int type, float cutoff, float resonance)
    {
        oscType = type;
        filter.setCutoffFrequency (juce::jlimit (20.0f, 20000.0f, cutoff));
        filter.setResonance (juce::jlimit (0.1f, 20.0f, resonance));
    }

private:
    juce::ADSR adsr;
    juce::dsp::StateVariableTPTFilter<float> filter;
    double currentAngle = 0.0, angleDelta = 0.0;
    float level = 0.0f;
    int oscType = 1; // Default Saw
};

// A simple Synthesizer Sound
struct SynthSound : public juce::SynthesiserSound
{
    SynthSound() {}
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class MainComponent  : public juce::AudioAppComponent, 
                        public juce::MidiInputCallback,
                        public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    // MidiInputCallback
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;

    // Timer (for UI Playhead sync)
    void timerCallback() override;

private:
    std::unique_ptr<juce::WebBrowserComponent> webBrowser;
    
    // Synth Core
    juce::Synthesiser synth;
    float currentCutoff = 2000.0f;
    float currentRes = 0.7f;
    int currentOscType = 1;

    // Sequencing
    Transport transport;
    ProjectModel model;
    double lastProcessedBeat = -1.0;
    double currentSampleRate = 0.0;
    
    // Recording state
    struct PendingNote { int note; double startBeat; };
    std::vector<PendingNote> pendingNotes;

    // Metronome
    bool metronomeEnabled = false;
    double lastClickBeat = -1.0;

    void updateSynthParams();
    void playMetronomeClick (const juce::AudioSourceChannelInfo& bufferToFill);
    void saveProject();
    
    juce::String getFullProjectJson();
    void loadFullProjectJson (const juce::String& json);

    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File lastDirectory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};