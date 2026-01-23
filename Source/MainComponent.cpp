#include "MainComponent.h"
#include "RealTimeLogger.h"

MainComponent::MainComponent()
{
    RealTimeLogger::log ("Application Started");
    
    loadAudioSettings();

    // Create a default instrument track
    auto synthProc = std::make_unique<InternalSynthProcessor>();
    auto track = std::make_unique<InstrumentTrack> ("Lead Synth");
    track->setInstrument (std::move (synthProc));
    mixer.addTrack (std::move (track));

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
            
            if (vel > 0) postMidiToEngine(juce::MidiMessage::noteOn(1, note, vel));
            else        postMidiToEngine(juce::MidiMessage::noteOff(1, note, 0.0f));
        })
        .withEventListener ("editEvent", [this] (juce::var params) {
            juce::String type = params["type"];
            int note = params["note"];
            double rawBeat = params["beat"];
            
            // Quantize to 16th notes (0.25 beats) for the backend model
            double quantizedBeat = std::round(rawBeat * 4.0) / 4.0;

            if (type == "add") {
                model.addNote({ note, 0.8f, quantizedBeat, 1.0 });
                RealTimeLogger::log("Grid Add: " + juce::String(note) + " at " + juce::String(quantizedBeat, 2));
            } else if (type == "remove") {
                model.removeNote(note, quantizedBeat);
                RealTimeLogger::log("Grid Remove: " + juce::String(note) + " at " + juce::String(quantizedBeat, 2));
            }
        })
        .withEventListener ("audioDeviceEvent", [this] (juce::var params) {
            juce::String cmd = params["command"];
            if (cmd == "list") {
                webBrowser->evaluateJavascript("if(window.onAudioDevices) window.onAudioDevices(" + getAudioDevicesJson() + ");");
            } else if (cmd == "set") {
                setAudioDevice(params["type"], params["device"]);
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
            else if (cmd == "stop")  { 
                transport.setPlaying (false); 
                transport.reset(); 
                mixer.allNotesOff();
                RealTimeLogger::log ("Transport Stopped");
            }
            else if (cmd == "record") {
                bool val = (bool)params["value"];
                transport.setRecording (val);
                if (!val) activeRecordingNotes.clear();
                RealTimeLogger::log (val ? "Recording Armed" : "Recording Stopped");
            }
            else if (cmd == "bpm")   transport.setBpm ((double)params["value"]);
            else if (cmd == "metronome") {
                metronomeEnabled = (bool)params["value"];
                RealTimeLogger::log (juce::String("Metronome: ") + (metronomeEnabled ? "ON" : "OFF"));
            }
            else if (cmd == "clear") { model.clear(); RealTimeLogger::log("Project Cleared"); }
            else if (cmd == "save")  saveProject();
            else if (cmd == "export") {
                webBrowser->evaluateJavascript("if(window.onExport) window.onExport(" + getFullProjectJson() + ");");
            }
            else if (cmd == "import") {
                loadFullProjectJson(params["value"]);
                RealTimeLogger::log("Project Imported");
            }
        })
        .withEventListener ("mixerEvent", [this] (juce::var params) {
            juce::String cmd = params["command"];
            int trackIndex = params["trackIndex"];
            if (auto* track = mixer.getTrack(trackIndex)) {
                if (cmd == "mute") {
                    bool val = (bool)params["value"];
                    track->setMuted(val);
                    RealTimeLogger::log(track->getName() + (val ? " Muted" : " Unmuted"));
                }
                else if (cmd == "solo") {
                    bool val = (bool)params["value"];
                    track->setSoloed(val);
                    RealTimeLogger::log(track->getName() + (val ? " Soloed" : " Unsoloed"));
                }
                else if (cmd == "vol") track->setVolume((float)params["value"]);
                else if (cmd == "pan") track->setPan((float)params["value"]);
                else if (cmd == "select") {
                    selectedTrackIndex = trackIndex;
                    updateSynthParams();
                    RealTimeLogger::log("Selected Track: " + track->getName());
                }
            }
            if (cmd == "addTrack") {
                juce::String name = params["name"];
                auto synthProc = std::make_unique<InternalSynthProcessor>();
                if (currentSampleRate > 0) synthProc->prepareToPlay(currentSampleRate, 512);
                auto t = std::make_unique<InstrumentTrack> (name);
                t->setInstrument (std::move (synthProc));
                mixer.addTrack (std::move (t));
                RealTimeLogger::log("Added Track: " + name);
            }
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
    saveAudioSettings();
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    for (auto& input : midiInputs)
        deviceManager.removeMidiInputDeviceCallback (input.identifier, this);
    shutdownAudio(); 
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    mixer.prepareToPlay (sampleRate, samplesPerBlockExpected);
    updateSynthParams();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (currentSampleRate <= 0)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    // 1. Advance Transport and Trigger Notes for this block
    double beatBefore = transport.getCurrentBeat();
    if (transport.getIsPlaying())
        transport.advance (bufferToFill.numSamples, currentSampleRate);
    double beatAfter = transport.getCurrentBeat();

    if (transport.getIsPlaying())
    {
        if (auto* track = mixer.getTrack(selectedTrackIndex))
        {
            if (auto* inst = dynamic_cast<InstrumentTrack*>(track))
            {
                if (auto* synth = dynamic_cast<InternalSynthProcessor*>(inst->getProcessor()))
                {
                    const auto& notes = model.getNotes();
                    for (const auto& note : notes)
                    {
                        bool noteStarted = (note.startBeat >= beatBefore && note.startBeat < beatAfter);
                        if (beatAfter < beatBefore) noteStarted = (note.startBeat >= beatBefore || note.startBeat < beatAfter);
                        if (noteStarted) synth->noteOn (note.note, note.velocity);

                        double endBeat = note.startBeat + note.durationBeats;
                        if (endBeat >= 16.0) endBeat -= 16.0;
                        bool noteEnded = (endBeat >= beatBefore && endBeat < beatAfter);
                        if (beatAfter < beatBefore) noteEnded = (endBeat >= beatBefore || endBeat < beatAfter);
                        if (noteEnded) synth->noteOff (note.note, 0.0f, true);
                    }
                }
            }
        }
    }

    // 2. Process Mixer (Master Sum) - Mixer handles clearing the buffer
    juce::MidiBuffer midiMessages;
    mixer.processBlock (*bufferToFill.buffer, midiMessages);

    // 3. Post-Mixer Overlays (Metronome)
    if (metronomeEnabled && transport.getIsPlaying())
    {
        if (std::floor(beatAfter) != std::floor(beatBefore) || (beatBefore > beatAfter))
            playMetronomeClick(bufferToFill);
    }
}

void MainComponent::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    postMidiToEngine(message);
    
    juce::MessageManager::callAsync ([this, message]() {
        if (message.isNoteOn() || message.isNoteOff())
        {
            juce::DynamicObject::Ptr obj = new juce::DynamicObject();
            obj->setProperty ("note", message.getNoteNumber());
            obj->setProperty ("velocity", message.isNoteOn() ? message.getFloatVelocity() : 0.0f);
            webBrowser->evaluateJavascript ("if(window.onMidiIn) window.onMidiIn(" + juce::JSON::toString(juce::var(obj.get())) + ");");
        }
    });
}

void MainComponent::postMidiToEngine (const juce::MidiMessage& message)
{
    // MIDI RECORDING LOGIC
    if (transport.getIsRecording()) {
        if (message.isNoteOn()) {
            activeRecordingNotes[message.getNoteNumber()] = { transport.getCurrentBeat(), message.getFloatVelocity() };
        } else if (message.isNoteOff() || (message.isNoteOn() && message.getVelocity() == 0)) {
            int n = message.getNoteNumber();
            if (activeRecordingNotes.count(n)) {
                double start = activeRecordingNotes[n].startBeat;
                float vel = activeRecordingNotes[n].velocity;
                double end = transport.getCurrentBeat();
                
                if (end < start) end += 16.0; // Loop wrap
                
                double duration = end - start;
                if (duration < 0.05) duration = 0.1;

                model.addNote({ n, vel, start, duration });
                activeRecordingNotes.erase(n);
                
                RealTimeLogger::log("Captured: " + juce::String(n) + " @ beat " + juce::String(start, 2));
            }
        }
    }

    // Live Monitoring
    if (auto* track = mixer.getTrack(selectedTrackIndex)) {
        if (auto* inst = dynamic_cast<InstrumentTrack*>(track)) {
            if (auto* synth = dynamic_cast<InternalSynthProcessor*>(inst->getProcessor())) {
                if (message.isNoteOn()) synth->noteOn(message.getNoteNumber(), message.getFloatVelocity());
                else if (message.isNoteOff()) synth->noteOff(message.getNoteNumber(), message.getFloatVelocity(), true);
            }
        }
    }
}

void MainComponent::playMetronomeClick (const juce::AudioSourceChannelInfo& bufferToFill)
{
    const float freq = (std::floor(transport.getCurrentBeat()) == 0) ? 1200.0f : 800.0f;
    const float amplitude = 0.4f;
    const int clickLength = static_cast<int>(currentSampleRate * 0.02); // 20ms click

    for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
    {
        auto* data = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);
        for (int i = 0; i < std::min(clickLength, bufferToFill.numSamples); ++i)
        {
            float env = 1.0f - (static_cast<float>(i) / clickLength);
            data[i] += std::sin(i * freq * 2.0f * juce::MathConstants<float>::pi / (float)currentSampleRate) * amplitude * env;
        }
    }
}

void MainComponent::updateSynthParams()
{
    if (auto* track = mixer.getTrack(selectedTrackIndex)) {
        if (auto* inst = dynamic_cast<InstrumentTrack*>(track)) {
            if (auto* synth = dynamic_cast<InternalSynthProcessor*>(inst->getProcessor())) {
                synth->updateParameters (currentOscType, currentCutoff, currentRes);
            }
        }
    }
}

void MainComponent::saveProject()
{
    if (lastDirectory.getFullPathName().isEmpty())
        lastDirectory = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);

    fileChooser = std::make_unique<juce::FileChooser> ("Save Project", lastDirectory, "*.json");
    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.getFullPathName().isNotEmpty()) {
            model.saveToFile (file);
            lastDirectory = file.getParentDirectory();
            RealTimeLogger::log ("Project saved to: " + file.getFullPathName());
        }
    });
}

juce::String MainComponent::getFullProjectJson()
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("bpm", transport.getBpm());
    juce::DynamicObject::Ptr synthParams = new juce::DynamicObject();
    synthParams->setProperty("osc", currentOscType); 
    synthParams->setProperty("cut", currentCutoff); 
    synthParams->setProperty("res", currentRes);
    obj->setProperty("synth", juce::var(synthParams.get()));
    obj->setProperty("t1", model.toMinifiedVar());
    return juce::JSON::toString(juce::var(obj.get()));
}

void MainComponent::loadFullProjectJson(const juce::String& json)
{
    auto var = juce::JSON::parse(json);
    if (var.isObject()) {
        if (var.hasProperty("bpm")) transport.setBpm(var["bpm"]);
        if (var.hasProperty("synth")) {
            auto s = var["synth"];
            if (s.hasProperty("osc")) currentOscType = s["osc"];
            if (s.hasProperty("cut")) currentCutoff = s["cut"];
            if (s.hasProperty("res")) currentRes = s["res"];
            updateSynthParams();
        }
        if (var.hasProperty("t1")) model.fromMinifiedVar(var["t1"]);
        RealTimeLogger::log("Project Loaded via JSON");
    }
}

juce::String MainComponent::getAudioDevicesJson()
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    juce::Array<juce::var> typesArray;
    auto& types = deviceManager.getAvailableDeviceTypes();
    for (auto* t : types) {
        juce::DynamicObject::Ptr typeObj = new juce::DynamicObject();
        typeObj->setProperty("name", t->getTypeName());
        juce::Array<juce::var> devicesArray;
        auto devices = t->getDeviceNames();
        for (auto& d : devices) devicesArray.add(juce::var(d));
        typeObj->setProperty("devices", devicesArray);
        typesArray.add(juce::var(typeObj.get()));
    }
    root->setProperty("types", typesArray);
    auto* currentDevice = deviceManager.getCurrentAudioDevice();
    if (currentDevice != nullptr) {
        root->setProperty("currentType", currentDevice->getTypeName());
        root->setProperty("currentDevice", currentDevice->getName());
    }
    return juce::JSON::toString(juce::var(root.get()));
}

void MainComponent::setAudioDevice (const juce::String& type, const juce::String& deviceName)
{
    RealTimeLogger::log ("Request: Type=" + type + ", Device=" + deviceName);
    
    // 1. Switch Type if different
    juce::String currentTypeName = "";
    if (auto* currentDevice = deviceManager.getCurrentAudioDevice())
        currentTypeName = currentDevice->getTypeName();

    if (currentTypeName != type)
    {
        RealTimeLogger::log ("Switching Type: " + currentTypeName + " -> " + type);
        deviceManager.setCurrentAudioDeviceType (type, true);
    }

    // 2. Apply Setup
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    setup.outputDeviceName = deviceName;
    
    auto err = deviceManager.setAudioDeviceSetup (setup, true); 
    if (err.isNotEmpty()) {
        RealTimeLogger::log ("Device Error: " + err); 
    } else {
        RealTimeLogger::log ("Device Active: " + deviceName);
        saveAudioSettings();
    }
}

void MainComponent::saveAudioSettings()
{
    auto settings = deviceManager.createStateXml();
    if (settings != nullptr)
    {
        auto file = juce::File("C:\\music_maker\\audio_settings.xml");
        settings->writeTo(file);
        RealTimeLogger::log("Audio settings saved.");
    }
}

void MainComponent::loadAudioSettings()
{
    auto file = juce::File("C:\\music_maker\\audio_settings.xml");
    if (file.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(file);
        if (xml != nullptr)
        {
            auto err = deviceManager.initialise(0, 2, xml.get(), true);
            if (err.isEmpty())
            {
                RealTimeLogger::log("Audio settings loaded from file.");
                return;
            }
            RealTimeLogger::log("Error loading audio settings: " + err);
        }
    }
    
    // Fallback if no file or error: explicitly scan types
    RealTimeLogger::log("No valid audio settings file found. Using defaults.");
    deviceManager.initialiseWithDefaultDevices(0, 2);
    
    // Ensure the device types are available immediately
    auto& types = deviceManager.getAvailableDeviceTypes();
    for (auto* t : types) {
        RealTimeLogger::log("Driver Type Available: " + t->getTypeName());
    }
}

void MainComponent::timerCallback()
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("beat", transport.getCurrentBeat());
    obj->setProperty ("playing", transport.getIsPlaying());
    
    juce::Array<juce::var> notesArray;
    for (const auto& n : model.getNotes()) {
        juce::DynamicObject::Ptr nObj = new juce::DynamicObject();
        nObj->setProperty("n", n.note); 
        nObj->setProperty("s", n.startBeat); 
        nObj->setProperty("d", n.durationBeats);
        notesArray.add(juce::var(nObj.get()));
    }
    obj->setProperty("notes", notesArray);
    obj->setProperty("selectedTrack", selectedTrackIndex);

    juce::Array<juce::var> tracksArray;
    int nTracks = mixer.getNumTracks();
    for (int i = 0; i < nTracks; ++i) {
        if (auto* t = mixer.getTrack(i)) {
            juce::DynamicObject::Ptr tObj = new juce::DynamicObject();
            tObj->setProperty("name", t->getName());
            tObj->setProperty("vol", t->getVolume());
            tObj->setProperty("pan", t->getPan());
            tObj->setProperty("mute", t->getIsMuted());
            tObj->setProperty("solo", t->getIsSoloed());
            tracksArray.add(juce::var(tObj.get()));
        }
    }
    obj->setProperty("tracks", tracksArray);

    auto logs = RealTimeLogger::getPendingUiLogs();
    
    // Recording Pulse (to verify transport movement while recording)
    if (transport.getIsRecording()) {
        static int pulseCount = 0;
        if (++pulseCount % 120 == 0) // Every ~2 seconds
            RealTimeLogger::log("REC RUNNING: beat " + juce::String(transport.getCurrentBeat(), 1));
    }

    if (logs.size() > 0) {
        juce::Array<juce::var> logsVar;
        for (auto& l : logs) logsVar.add (l);
        obj->setProperty ("logs", logsVar);
    }

    webBrowser->evaluateJavascript ("if(window.onUpdate) window.onUpdate(" + juce::JSON::toString(juce::var(obj.get())) + ");");
}

void MainComponent::releaseResources() 
{
    mixer.releaseResources();
}
void MainComponent::paint (juce::Graphics& g) { g.fillAll (juce::Colours::black); }
void MainComponent::resized() { webBrowser->setBounds (getLocalBounds()); }