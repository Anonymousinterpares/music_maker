#include "MainComponent.h"
#include "RealTimeLogger.h"

MainComponent::MainComponent()
{
    RealTimeLogger::log ("Application Started");
    
    for (int i = 0; i < 8; ++i)
    {
        auto* voice = new SynthVoice();
        synth.addVoice (voice);
    }
    synth.addSound (new SynthSound());

    auto midiInputs = juce::MidiInput::getAvailableDevices();
    for (auto& input : midiInputs)
    {
        deviceManager.setMidiInputDeviceEnabled (input.identifier, true);
        deviceManager.addMidiInputDeviceCallback (input.identifier, this);
    }

    auto userDataPath = juce::File ("C:\\music_maker\\WebViewData");
    auto options = juce::WebBrowserComponent::Options{}
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}.withUserDataFolder (userDataPath))
        .withNativeIntegrationEnabled (true)
        .withEventListener ("playNoteEvent", [this] (juce::var params) {
            int note = params["note"];
            float vel = params["velocity"];
            if (vel > 0) 
            {
                synth.noteOn (1, note, vel);
                if (transport.getIsRecording())
                    pendingNotes.push_back ({ note, transport.getCurrentBeat() });
            }
            else        
            {
                synth.noteOff (1, note, 0.0f, true);
                if (transport.getIsRecording())
                {
                    auto it = std::find_if (pendingNotes.begin(), pendingNotes.end(), [&](const PendingNote& p) { return p.note == note; });
                    if (it != pendingNotes.end())
                    {
                        double duration = transport.getCurrentBeat() - it->startBeat;
                        if (duration < 0) duration += 16.0; // Handle loop wrap
                        model.addNote ({ note, 0.8f, it->startBeat, duration });
                        pendingNotes.erase (it);
                    }
                }
            }
        })
        .withEventListener ("parameterEvent", [this] (juce::var params) {
            juce::String name = params["name"];
            float val = (float)params["value"];
            if (name == "cutoff") currentCutoff = val;
            else if (name == "res") currentRes = val;
            else if (name == "osc") currentOscType = (int)val;
            updateSynthParams();
        })
        .withEventListener ("transportEvent", [this] (juce::var params) {
            juce::String cmd = params["command"];
            if (cmd == "play")       transport.setPlaying (true);
            else if (cmd == "stop")  transport.setPlaying (false);
            else if (cmd == "record") transport.setRecording (params["value"]);
            else if (cmd == "bpm")   transport.setBpm ((double)params["value"]);
            else if (cmd == "metronome") metronomeEnabled = (bool)params["value"];
            else if (cmd == "clear") { model.clear(); pendingNotes.clear(); }
            else if (cmd == "save")  saveProject();
        })
        .withEventListener ("editEvent", [this] (juce::var params) {
            juce::String type = params["type"];
            if (type == "add") 
                model.addNote ({ (int)params["note"], 0.8f, (double)params["beat"], 1.0 });
            else if (type == "remove")
                model.removeNote ((int)params["note"], (double)params["beat"]);
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
    startTimerHz (60);
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
    currentSampleRate = sampleRate;
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice(i)))
            v->prepare (sampleRate);
    }
    updateSynthParams();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    if (currentSampleRate > 0)
    {
        double beatBefore = transport.getCurrentBeat();
        transport.advance (bufferToFill.numSamples, currentSampleRate);
        double beatAfter = transport.getCurrentBeat();

        if (transport.getIsPlaying())
        {
            for (const auto& note : model.getNotes())
            {
                bool noteStarted = (note.startBeat >= beatBefore && note.startBeat < beatAfter);
                if (beatAfter < beatBefore) noteStarted = (note.startBeat >= beatBefore || note.startBeat < beatAfter);
                if (noteStarted) synth.noteOn (1, note.note, note.velocity);

                double endBeat = note.startBeat + note.durationBeats;
                if (endBeat >= 16.0) endBeat -= 16.0;
                bool noteEnded = (endBeat >= beatBefore && endBeat < beatAfter);
                if (beatAfter < beatBefore) noteEnded = (endBeat >= beatBefore || endBeat < beatAfter);
                if (noteEnded) synth.noteOff (1, note.note, 0.0f, true);
            }

            if (metronomeEnabled)
            {
                if (std::floor(beatAfter) > std::floor(beatBefore) || (beatAfter < beatBefore && beatAfter >= 0))
                    playMetronomeClick(bufferToFill);
            }
        }
    }
    juce::MidiBuffer incomingMidi; 
    synth.renderNextBlock (*bufferToFill.buffer, incomingMidi, bufferToFill.startSample, bufferToFill.numSamples);
}

void MainComponent::playMetronomeClick (const juce::AudioSourceChannelInfo& bufferToFill)
{
    float freq = 880.0f;
    float phaseDelta = (freq * 2.0f * juce::MathConstants<float>::pi) / (float)currentSampleRate;
    static float phase = 0;
    for (int i = 0; i < juce::jmin(500, bufferToFill.numSamples); ++i)
    {
        float sample = std::sin(phase) * 0.2f;
        for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
            bufferToFill.buffer->addSample(ch, bufferToFill.startSample + i, sample);
        phase += phaseDelta;
    }
    phase = 0; // Reset for next click
}

void MainComponent::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        synth.noteOn (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
        if (transport.getIsRecording()) pendingNotes.push_back ({ message.getNoteNumber(), transport.getCurrentBeat() });
    }
    else if (message.isNoteOff())
    {
        synth.noteOff (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity(), true);
        if (transport.getIsRecording())
        {
            auto it = std::find_if (pendingNotes.begin(), pendingNotes.end(), [&](const PendingNote& p) { return p.note == message.getNoteNumber(); });
            if (it != pendingNotes.end())
            {
                double duration = transport.getCurrentBeat() - it->startBeat;
                if (duration < 0) duration += 16.0;
                model.addNote ({ message.getNoteNumber(), message.getFloatVelocity(), it->startBeat, duration });
                pendingNotes.erase (it);
            }
        }
    }
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
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice(i)))
            v->updateParameters (currentOscType, currentCutoff, currentRes);
}

void MainComponent::saveProject()
{
    if (lastDirectory.getFullPathName().isEmpty())
        lastDirectory = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);

    fileChooser = std::make_unique<juce::FileChooser> ("Save Project", lastDirectory, "*.json");

    auto folderFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (folderFlags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file.getFullPathName().isNotEmpty())
        {
            model.saveToFile (file);
            lastDirectory = file.getParentDirectory();
            RealTimeLogger::log ("Project saved to: " + file.getFullPathName());
        }
    });
}

void MainComponent::timerCallback()
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("beat", transport.getCurrentBeat());
    obj->setProperty ("playing", transport.getIsPlaying());
    
    juce::Array<juce::var> notesArray;
    for (const auto& n : model.getNotes()) {
        juce::DynamicObject::Ptr nObj = new juce::DynamicObject();
        nObj->setProperty("n", n.note); nObj->setProperty("s", n.startBeat); nObj->setProperty("d", n.durationBeats);
        notesArray.add(juce::var(nObj));
    }
    obj->setProperty("notes", notesArray);

    juce::String js = "if(window.onUpdate) window.onUpdate(" + juce::JSON::toString(juce::var(obj)) + ");";
    webBrowser->evaluateJavascript (js);
}

void MainComponent::releaseResources() {}
void MainComponent::paint (juce::Graphics& g) { g.fillAll (juce::Colours::black); }
void MainComponent::resized() { webBrowser->setBounds (getLocalBounds()); }