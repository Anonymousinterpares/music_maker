# DOCUMENT 1: MVP EXECUTION CHECKLIST (WEBVIEW HYBRID)
**Project Phase:** Minimum Viable Product (Alpha)
**Goal:** Functional Audio Engine, Basic Sequencing, LLM Text Bridge, and HTML/CSS Interface.

## Phase 1: Environment & Audio Core
*Objective: Initialize the C++ environment, set up the Webview bridge, and establish a stable audio stream.*

*   [ ] **1.1 Development Setup**
    *   Install Visual Studio 2022 (C++ Desktop logic).
    *   Install JUCE Framework (latest stable build).
    *   Configure `CMakeLists.txt` to include `juce_audio_utils` and **`juce_gui_extra`** (required for WebBrowser).
    *   Initialize Git repository.
*   [ ] **1.2 Hybrid Architecture Setup**
    *   Create `MainComponent` holding a `juce::WebBrowserComponent`.
    *   Create a `Resources` folder containing a basic `index.html`.
    *   Configure CMake/JUCE to bundle the `Resources` folder into the binary data.
    *   **Task:** Ensure the app launches and displays "Hello World" from the HTML file.
*   [ ] **1.3 Audio Device Manager**
    *   Implement `AudioAppComponent` logic (Back-end).
    *   Initialize WASAPI (Windows Audio) driver.
    *   Implement `getNextAudioBlock` callback (Audio Thread).
    *   **Test:** Generate a static sine wave (440Hz) to Output 1/2 strictly from C++.
*   [ ] **1.4 The "Binding" Layer (Bridge)**
    *   Create `NativeBridge` class.
    *   Implement **C++ -> JS:** `evaluateJavascript()` to send data to the UI.
    *   Implement **JS -> C++:** `withJavascriptObject` to allow JS to trigger C++ functions (e.g., `native.playNote(60)`).

**SUCCESS CRITERIA:** Application opens a window rendering HTML/CSS. A button in HTML labeled "Test Sound" calls a C++ function that makes a beep.

## Phase 2: Sound Generation (Synth & Sampler)
*Objective: Create the C++ objects that generate music (Note: This phase is 100% C++ and largely unchanged).*

*   [ ] **2.1 Subtractive Synthesizer Class (C++)**
    *   Code Oscillator logic (Sine, Saw, Square).
    *   Code ADSR Envelope.
    *   Code State Variable Filter.
    *   Implement `renderNextBlock` to fill audio buffers.
*   [ ] **2.2 Basic Sampler Class (C++)**
    *   Implement `AudioFormatReader` to load WAV files.
    *   Implement Linear Interpolation for pitch shifting.
*   [ ] **2.3 Polyphony Management**
    *   Implement "Voice Manager" (8 voices).
    *   Implement "Voice Stealing".

**SUCCESS CRITERIA:** The C++ engine can play chords (8 notes) without UI visualization, verified via the internal Logger.

## Phase 3: Hybrid Interaction
*Objective: Connect the HTML UI to the C++ Sound Engine.*

*   [ ] **3.1 Virtual Keyboard (HTML/CSS)**
    *   Create `keyboard.js` and `style.css`.
    *   Use CSS Grid/Flexbox to draw piano keys.
    *   Add JS Event Listeners (`mousedown`, `mouseup`) on the Divs.
    *   **Binding:** JS calls `window.native.sendMidi(note, velocity)` on click.
*   [ ] **3.2 Hardware MIDI Support (C++)**
    *   Scan for USB MIDI devices in C++.
    *   **Binding:** When C++ receives a USB MIDI note, it executes JS: `updateKeyVisuals(note, true)` to add a CSS "active" class to the HTML key.
*   [ ] **3.3 Latency Check**
    *   Ensure the C++ `MidiMessageCollector` picks up JS events instantly.

**SUCCESS CRITERIA:** Pressing a key on the HTML screen produces sound. Pressing a key on a physical USB keyboard produces sound AND lights up the HTML key.

## Phase 4: Sequencing & Recording
*Objective: Record events over time and visualize them in a web canvas.*

*   [ ] **4.1 The Transport (C++ & JS)**
    *   **C++:** Manage `CurrentTime` (samples/beats).
    *   **JS:** Create Play/Stop buttons in HTML.
    *   **Binding:** JS Play button -> Calls C++ `transport.play()`.
*   [ ] **4.2 The Piano Roll Data (C++)**
    *   Create the `ProjectModel` vector storing Note Events.
    *   Implement the playback iterator in the Audio Thread.
*   [ ] **4.3 The Piano Roll UI (JS/Canvas)**
    *   Create a `<canvas>` element or a CSS Grid container in HTML.
    *   Implement JS logic to draw rectangles based on data received from C++.
    *   **Draw Mode:** JS calculates click position -> sends `addNote(pitch, time)` to C++.
*   [ ] **4.4 Playhead Synchronization**
    *   **C++:** Start a Timer (60fps) on the Message Thread.
    *   **C++:** Every frame, send current beat position to JS.
    *   **JS:** Update the X-position of the "Playhead" div using CSS `transform`.

**SUCCESS CRITERIA:** User can draw a note on the HTML Canvas. When Play is pressed, the CSS playhead moves smoothly across the note, and C++ generates the sound at the correct moment.

## Phase 5: The AI Bridge (Data Protocol)
*Objective: Make the song readable by an LLM.*

*   [ ] **5.1 JSON Serialization (C++)**
    *   Implement the "Minified" JSON logic (Arrays instead of verbose objects).
    *   Function `getProjectAsJson()` returns `std::string`.
*   [ ] **5.2 Text Export View (HTML)**
    *   Create an HTML `<textarea id="code-view">`.
    *   Create "Export" button.
    *   **Action:** Click Export -> C++ generates JSON -> C++ injects string into the `<textarea>`.
*   [ ] **5.3 Import Logic (Hybrid)**
    *   Create "Import" button.
    *   **Action:** Click Import -> JS sends text from `<textarea>` to C++.
    *   C++ validates schema -> Rebuilds `ProjectModel` -> Triggers `refreshUi()` event to JS.

**SUCCESS CRITERIA:** User can copy the song code from the HTML text area, modify it in ChatGPT, paste it back, and the HTML Piano Roll updates instantly to show the new notes.