# DOCUMENT 3: INVARIANTS.md

## 1. The Real-Time Audio Invariant (The "Golden Rule")
**Invariant:** The Audio Thread (`processBlock`) must **NEVER** block, allocate memory, or wait for a lock.

*   **Rule:** No use of `std::mutex`, `malloc`, `new`, `free`, `delete`, `std::cout`, or file I/O inside the audio callback.
*   **Reasoning:** If the audio thread pauses for even 1 millisecond to wait for a UI lock or memory allocation, the audio buffer will empty, causing a "pop/click" artifact and potentially crashing the ASIO driver.
*   **Enforcement:** All data passed to the Audio Thread must be pre-allocated. All communication must happen via **Lock-Free FIFOs** (First-In-First-Out queues).

## 2. The "Single Source of Truth" Invariant
**Invariant:** The C++ `ProjectModel` is the only valid state. The Webview (UI) is purely a reflection.

*   **Rule:** JavaScript **cannot** modify audio parameters directly. JavaScript sends a **Command** (e.g., `UpdateVol(0.5)`). The C++ Message Thread applies this to the Model, and the Model pushes the new value to the Audio Engine.
*   **Reasoning:** If JS modifies a value while the Audio Engine is reading it, a Race Condition occurs, leading to weird bugs (e.g., volume spiking to 1000% for one sample).
*   **Enforcement:** Data flow is strictly unidirectional: `JS Event -> C++ Command -> Update Model -> Audio Engine reads Model`.

## 3. The DSP/UI Separation Invariant
**Invariant:** No Digital Signal Processing (DSP) logic shall exist in JavaScript.

*   **Rule:** The Webview is strictly for **Control** and **Visualization**. It never generates or modifies audio samples.
*   **Reasoning:** JavaScript timing is imprecise (garbage collection pauses). C++ is precise. Mixing them leads to timing drift.
*   **Enforcement:** The `WebBrowserComponent` must not have access to audio buffers, only to metadata (Peak meters, Playhead position).

## 4. The Data Protocol Integrity Invariant
**Invariant:** The application must be capable of rebuilding the entire runtime state from the JSON Serialization alone.

*   **Rule:** There can be no "hidden state" in the UI or C++ variables that isn't saved to the JSON object.
*   **Reasoning:** To allow the LLM to modify the song, the JSON must represent 100% of the song. If the Synthesizer has a "Filter Resonance" knob, but that knob isn't in the JSON, the LLM cannot control it, and reloading the song will sound wrong.
*   **Enforcement:** Serialization Unit Tests. `Load(Save(State)) == State`.

## 5. The AI Safety Invariant
**Invariant:** Imported Data must be sanitized *before* it touches the Audio Engine.

*   **Rule:** The JSON Parser acts as a firewall.
    *   Velocity must be clamped 0-127.
    *   Note Duration must be > 0.
    *   Filter Frequencies must be clamped 20Hz - 20kHz.
*   **Reasoning:** An LLM might hallucinate `{"velocity": 9999}`. If this raw number hits the amplifier code, it will destroy the user's speakers and ears.
*   **Enforcement:** A `Validator` class runs on the JSON string before the `ProjectModel` accepts it.

## 6. The "Headless" Invariant
**Invariant:** The Audio Engine must be able to run and render audio even if the UI (Webview) fails to load or is invisible.

*   **Rule:** The C++ backend initializes independently of the HTML frontend.
*   **Reasoning:** This decouples the systems. It allows us to write Unit Tests for the engine without needing to spin up a web browser instance, and it ensures stability if the WebView crashes.

## 7. The Notation Logic Invariant
**Invariant:** Audio Performance Data (MIDI) and Notation Display Data are separate layers.

*   **Rule:** Quantizing the *Sheet Music view* (to look pretty) must never destructively quantize the *Audio Performance* (how it sounds), unless explicitly requested.
*   **Reasoning:** A user might play a "human" swing feel. The sheet music should look clean (readable), but the playback should sound human (swing). Forcing them to be identical ruins the musicality.