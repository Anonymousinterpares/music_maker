#pragma once

#include <JuceHeader.h>
#include "ProjectModel.h"
#include "SynthEngine.h"
#include "Mixer.h"
#include "InternalSynth.h"

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
    
    // Multi-track Mixer
    Mixer mixer;
    int selectedTrackIndex = 0;
    
    // Sequencing
    Transport transport;
    ProjectModel model;
    double lastProcessedBeat = -1.0;
    double currentSampleRate = 0.0;
    
    // Recording state (Professional Logic)
    struct RecordedNote { double startBeat; float velocity; };
    std::map<int, RecordedNote> activeRecordingNotes; // noteNumber -> {startBeat, velocity}

    // Metronome
    bool metronomeEnabled = false;
    double lastClickBeat = -1.0;

    void updateSynthParams();
    void postMidiToEngine (const juce::MidiMessage& message);
    void playMetronomeClick (const juce::AudioSourceChannelInfo& bufferToFill);
    void saveProject();
    
    juce::String getFullProjectJson();
    void loadFullProjectJson (const juce::String& json);
    
    // Audio Device Management
    juce::String getAudioDevicesJson();
    void setAudioDevice (const juce::String& type, const juce::String& deviceName);
    void saveAudioSettings();
    void loadAudioSettings();

    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File lastDirectory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};