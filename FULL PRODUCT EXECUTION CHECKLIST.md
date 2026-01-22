# DOCUMENT 2: FULL PRODUCT EXECUTION CHECKLIST (WEBVIEW HYBRID)
**Project Phase:** Commercial Release (V1.0)
**Goal:** Professional Features, Advanced Theory, Bi-directional AI, Industry Standards.

## Phase 1: Core Engine Refactoring (The DAW Backbone)
*Objective: Transition from a single-loop MVP to a multi-track linear DAW.*

*   [x] **1.1 Multi-Track Mixer Architecture (C++)**
    *   [x] Create `Track` class (Audio vs MIDI).
    *   [x] Create `Mixer` class (Summing junction, volume, pan).
    *   [ ] Implement `AudioProcessorGraph` (JUCE) for flexible routing (Replaced by custom Mixer for now).
    *   [x] Refactor Synth into `InternalSynthProcessor`.
*   [ ] **1.2 Linear Timeline & Clip System (C++)**
    *   Replace `Transport` loop logic with a linear sample counter.
    *   Implement `Clip` objects (Start time, Length, Offset).
    *   Support for multiple Clips per track.
*   [ ] **1.3 Thread-Safe Project Model**
    *   Implement a lock-free "Audio Thread View" of the project.
    *   Ensure Message Thread edits (from JS) don't block the Audio Thread.
*   [x] **1.4 ASIO Implementation & Persistence (C++)**
    *   Integrate Steinberg ASIO SDK. (Already implemented)
    *   **Persistence:** Save and autoload last audio setup (XML).
    *   **UI:** Create HTML Settings Menu that populates from the C++ data. (Already implemented)

## Phase 2: Professional Audio & Plugin Hosting
*Objective: Support industry-standard tools and recording.*

*   [ ] **2.1 VST3 Plugin Hosting (C++)**
    *   Implement VST3 Host scanner.
    *   **Window Management:** Implement `PluginWindow` (Native HWND floating over Webview).
    *   **Automation:** Map HTML range sliders to VST Parameters.
*   [ ] **2.2 Audio Recording & Disk I/O**
    *   Implement `AudioFileWriter` (WAV/FLAC).
    *   Background Waveform calculation (Peak/RMS).
    *   **JS:** Renders the waveform on `<canvas>`.
*   [ ] **2.3 Advanced DSP Suite (C++)**
    *   Code 5-Band Parametric EQ.
    *   Code Compressor with sidechain.
    *   Code Delay/Reverb modules.
*   [ ] **2.4 Sampler Engine (Advanced)**
    *   Multi-sample mapping (Velocity/Key zones).
    *   Pitch-stretching (Phase Vocoder).

## Phase 3: AI Composition & Analysis (The Unique Edge)
*Objective: Integrate ML/AI features directly into the DAW workflow.*

*   [ ] **3.1 Audio-to-MIDI Transcription (C++/ML)**
    *   Integrate onset detection (Transients).
    *   Integrate Pitch Detection (YIN/MPM or ML-based).
    *   **Action:** User right-clicks Audio Clip -> "Convert to MIDI".
*   [ ] **3.2 Source Separation (Demixing)**
    *   Integrate pre-trained model (e.g., Spleeter/Demucs) for stem separation.
    *   **Action:** Drag-and-drop song -> Generates 4 Stem Tracks (Vocals, Drums, Bass, Other).
*   [ ] **3.3 Smart Notation & Theory (Hybrid)**
    *   **C++:** Chord Recognition & Scale analysis.
    *   **JS:** Render Sheet Music/Tabs via VexFlow or SVG.
*   [ ] **3.4 Deep LLM Integration**
    *   Diff-based JSON updates (Modify only selection).
    *   Token optimization for large arrangements.

## Phase 4: Production UI & Workflow
*Objective: High-performance interface for complex projects.*

*   [ ] **4.1 Arrangement View (Linear UI)**
    *   Implement zooming/scrolling timeline in HTML Canvas.
    *   Support for dragging clips between tracks.
*   [ ] **4.2 Mixer View (HTML/CSS)**
    *   Vertical faders, Solo/Mute/Arm buttons.
    *   Visual peak meters (C++ -> JS streaming).
*   [ ] **4.3 Browser / Library (HTML)**
    *   File system navigator for samples and presets.
    *   Drag-and-drop from Browser to Timeline.

## Phase 5: Final Polish & Stability
*Objective: Commercial readiness.*

*   [ ] **5.1 Thread Safety Audit & Stress Test**
    *   Simultaneous recording/playback/UI manipulation.
*   [ ] **5.2 CPU Optimization**
    *   SIMD (SSE/AVX) for DSP math.
    *   "Freeze Track" rendering.
*   [ ] **5.3 Windows Installer**
    *   MSI bundling WebView2 and Visual C++ Redistributables.
