#include "MainComponent.h"

MainComponent::MainComponent()
{
    addAndMakeVisible (nativeBtn);
    nativeBtn.setButtonText ("PLAY C4 (NATIVE)");
    nativeBtn.onClick = [this] { synth.noteOn (1, 60, 0.8f); };

    // Setup 8 voices for polyphony
    for (int i = 0; i < 8; ++i)
    {
        auto* voice = new SynthVoice();
        synth.addVoice (voice);
    }
    synth.addSound (new SynthSound());

    auto userDataPath = juce::File ("C:\\music_maker\\WebViewData");
    auto options = juce::WebBrowserComponent::Options{}
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}.withUserDataFolder (userDataPath))
        .withNativeIntegrationEnabled (true)
        .withNativeFunction ("playNote", [this] (const juce::Array<juce::var>& params, auto completion) {
            if (params.size() >= 2)
            {
                int note = params[0];
                float vel = (float)params[1];
                if (vel > 0) synth.noteOn (1, note, vel);
                else        synth.noteOff (1, note, 0.0f, true);
            }
            completion (true);
        })
        .withEventListener ("playNoteEvent", [this] (juce::var params) {
            int note = params["note"];
            float vel = params["velocity"];
            if (vel > 0) synth.noteOn (1, note, vel);
            else        synth.noteOff (1, note, 0.0f, true);
        })
        .withResourceProvider ([this] (const juce::String&) -> std::optional<juce::WebBrowserComponent::Resource> {
            return juce::WebBrowserComponent::Resource { 
                { (const std::byte*) BinaryData::index_html, (const std::byte*) BinaryData::index_html + BinaryData::index_htmlSize },
                "text/html" 
            };
        }, juce::WebBrowserComponent::getResourceProviderRoot());

    webBrowser = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (*webBrowser);
    webBrowser->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (1000, 700);
    setAudioChannels (0, 2);
}

MainComponent::~MainComponent() { shutdownAudio(); }

void MainComponent::prepareToPlay (int, double sampleRate)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice(i)))
            v->prepare (sampleRate);
    }
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    juce::MidiBuffer incomingMidi;
    synth.renderNextBlock (*bufferToFill.buffer, incomingMidi, bufferToFill.startSample, bufferToFill.numSamples);
}

void MainComponent::releaseResources() {}
void MainComponent::paint (juce::Graphics& g) { g.fillAll (juce::Colours::black); }
void MainComponent::resized()
{
    auto area = getLocalBounds();
    nativeBtn.setBounds (area.removeFromTop (40).reduced (5));
    webBrowser->setBounds (area);
}
