#pragma once

#include <JuceHeader.h>

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
    // Native fallback button
    juce::TextButton nativeBtn;
    
    // WebBrowserComponent must be a unique_ptr to ensure options are set before construction
    std::unique_ptr<juce::WebBrowserComponent> webBrowser;
    
    std::atomic<bool> isPlayingTestSound { false };
    double currentPhase = 0.0;
    double phaseDelta = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};