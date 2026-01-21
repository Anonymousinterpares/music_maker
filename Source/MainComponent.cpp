#include "MainComponent.h"

MainComponent::MainComponent()
{
    // Setup 8 voices for polyphony
    for (int i = 0; i < 8; ++i)
    {
        auto* voice = new SynthVoice();
        synth.addVoice (voice);
    }
    synth.addSound (new SynthSound());

    // Enable MIDI Input
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    for (auto& input : midiInputs)
    {
        deviceManager.setMidiInputDeviceEnabled (input.identifier, true);
        deviceManager.addMidiInputDeviceCallback (input.identifier, this);
    }

    auto userDataPath = juce::File ("C:\\music_maker\\WebViewData");
    auto options = juce::WebBrowserComponent::Options{}.withBackend (juce::WebBrowserComponent::Options::Backend::webview2).withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}.withUserDataFolder (userDataPath)).withNativeIntegrationEnabled (true).withNativeFunction ("playNote", [this] (const juce::Array<juce::var>& params, auto completion) {
            if (params.size() >= 2)
            {
                int note = params[0];
                float vel = (float)params[1];
                if (vel > 0) synth.noteOn (1, note, vel);
                else        synth.noteOff (1, note, 0.0f, true);
            }
            completion (true);
        }).withNativeFunction ("setParam", [this] (const juce::Array<juce::var>& params, auto completion) {
            if (params.size() >= 2)
            {
                juce::String name = params[0].toString();
                float val = (float)params[1];
                
                if (name == "cutoff") currentCutoff = val;
                else if (name == "res") currentRes = val;
                else if (name == "osc") currentOscType = (int)val;
                
                updateSynthParams();
            }
            completion (true);
        }).withEventListener ("playNoteEvent", [this] (juce::var params) {
            int note = params["note"];
            float vel = params["velocity"];
            if (vel > 0) synth.noteOn (1, note, vel);
            else        synth.noteOff (1, note, 0.0f, true);
        }).withResourceProvider ([this] (const juce::String&) -> std::optional<juce::WebBrowserComponent::Resource> {
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

MainComponent::~MainComponent() 
{ 
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    for (auto& input : midiInputs)
        deviceManager.removeMidiInputDeviceCallback (input.identifier, this);

    shutdownAudio(); 
}
void MainComponent::prepareToPlay (int, double sampleRate)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice(i)))
        {
            v->prepare (sampleRate);
            v->updateParameters (currentOscType, currentCutoff, currentRes);
        }
    }
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    juce::MidiBuffer incomingMidi; // We could use hardware MIDI here too, but for now we rely on noteOn calls
    synth.renderNextBlock (*bufferToFill.buffer, incomingMidi, bufferToFill.startSample, bufferToFill.numSamples);
}

void MainComponent::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    // Pass hardware MIDI to synth
    if (message.isNoteOn())
        synth.noteOn (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff())
        synth.noteOff (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity(), true);
    else if (message.isAllNotesOff())
        synth.allNotesOff (0, true);

    // Also notify UI (bidirectional bridge test)
    juce::MessageManager::callAsync ([this, message]() {
        if (message.isNoteOn() || message.isNoteOff())
        {
            juce::DynamicObject::Ptr obj = new juce::DynamicObject();
            obj->setProperty ("note", message.getNoteNumber());
            obj->setProperty ("velocity", message.isNoteOn() ? message.getFloatVelocity() : 0.0f);
            
            juce::String js = "if(window.onMidiIn) window.onMidiIn(" + juce::JSON::toString(juce::var(obj)) + ");";
            webBrowser->evaluateJavascript (js);
        }
    });
}

void MainComponent::updateSynthParams()
{
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice(i)))
            v->updateParameters (currentOscType, currentCutoff, currentRes);
    }
}

void MainComponent::releaseResources() {}
void MainComponent::paint (juce::Graphics& g) { g.fillAll (juce::Colours::black); }
void MainComponent::resized()
{
    webBrowser->setBounds (getLocalBounds());
}