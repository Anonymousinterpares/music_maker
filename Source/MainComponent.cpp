#include "MainComponent.h"
#include "RealTimeLogger.h"

MainComponent::MainComponent()
{
    RealTimeLogger::log ("Application Started");
    
    // JUCE 7: initialiseWithDefaultDevices will try to find and open the best devices
    deviceManager.initialiseWithDefaultDevices (0, 2);

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
            if (vel > 0) synth.noteOn (1, note, vel);
            else        synth.noteOff (1, note, 0.0f, true);
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
            else if (cmd == "bpm")   transport.setBpm ((double)params["data"]);
            else if (cmd == "clear") { model.clear(); }
            else if (cmd == "save")  saveProject();
            else if (cmd == "export") {
                webBrowser->evaluateJavascript("if(window.onExport) window.onExport(" + getFullProjectJson() + ");");
            }
            else if (cmd == "import") loadFullProjectJson(params["data"]);
        })
        .withEventListener ("audioDeviceEvent", [this] (juce::var params) {
            juce::String cmd = params["command"];
            RealTimeLogger::log("BRIDGE audioDeviceEvent: " + cmd);
            if (cmd == "list") {
                webBrowser->evaluateJavascript("if(window.onAudioDevices) window.onAudioDevices(" + getAudioDevicesJson() + ");");
            }
            else if (cmd == "set") {
                setAudioDevice(params["type"], params["device"]);
            }
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
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice(i)))
            v->prepare (sampleRate);
    updateSynthParams();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    if (currentSampleRate > 0 && transport.getIsPlaying())
    {
        double beatBefore = transport.getCurrentBeat();
        transport.advance (bufferToFill.numSamples, currentSampleRate);
        double beatAfter = transport.getCurrentBeat();

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
    }
    juce::MidiBuffer incomingMidi; 
    synth.renderNextBlock (*bufferToFill.buffer, incomingMidi, bufferToFill.startSample, bufferToFill.numSamples);
}

void MainComponent::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    if (message.isNoteOn()) synth.noteOn (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff()) synth.noteOff (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity(), true);
    
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
    synthParams->setProperty("osc", currentOscType); synthParams->setProperty("cut", currentCutoff); synthParams->setProperty("res", currentRes);
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
    if (err.isNotEmpty()) RealTimeLogger::log ("Device Error: " + err); 
    else                  RealTimeLogger::log ("Device Active: " + deviceName);
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

    auto logs = RealTimeLogger::getPendingUiLogs();
    if (logs.size() > 0) {
        juce::Array<juce::var> logsVar;
        for (auto& l : logs) logsVar.add (l);
        obj->setProperty ("logs", logsVar);
    }

    webBrowser->evaluateJavascript ("if(window.onUpdate) window.onUpdate(" + juce::JSON::toString(juce::var(obj.get())) + ");");
}

void MainComponent::releaseResources() {}
void MainComponent::paint (juce::Graphics& g) { g.fillAll (juce::Colours::black); }
void MainComponent::resized() { webBrowser->setBounds (getLocalBounds()); }