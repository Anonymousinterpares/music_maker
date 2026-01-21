# DOCUMENT 2: FULL PRODUCT EXECUTION CHECKLIST (WEBVIEW HYBRID)
**Project Phase:** Commercial Release (V1.0)
**Goal:** Professional Features, Advanced Theory, Bi-directional AI, Industry Standards.

## Phase 1: Professional Audio Standards
*Objective: Upgrade the engine to compete with commercial DAWs using the Hybrid Architecture.*

*   [ ] **1.1 ASIO Implementation (C++)**
    *   Integrate Steinberg ASIO SDK.
    *   **Binding:** Create C++ functions to list ASIO drivers and Buffer Sizes.
    *   **UI:** Create HTML Settings Menu that populates from the C++ data.
*   [ ] **1.2 Plugin Hosting (VST3)**
    *   Implement VST3 Host scanner (C++).
    *   **Window Management:** Implement `PluginWindow` class. Note: VST GUIs are native windows (HWND) and must float *over* the Webview; they cannot be embedded inside an HTML `<div>`.
    *   **UI:** Add "Open Plugin Interface" button in the HTML Mixer.
    *   **Automation:** Map HTML range sliders to `VSTParameter` IDs in C++.
*   [ ] **1.3 Advanced DSP Rack (C++)**
    *   Code 5-Band Parametric EQ (Bi-quad filters).
    *   Code Compressor (RMS detection, Gain Reduction).
    *   Code Delay (Circular Buffer with Feedback).
    *   Code Reverb (Schroeder/Moorer algorithm).

**SUCCESS CRITERIA:** User can load "Serum" (or any 3rd party VST), open its native window via an HTML button, apply an internal Reverb, and play it with 5ms latency via an ASIO driver.

## Phase 2: Advanced Music Theory Engine
*Objective: The "Brain" calculates theory in C++, the "View" renders it in HTML.*

*   [ ] **2.1 Smart Quantization Layer (C++)**
    *   Algorithm to analyze raw MIDI timestamp and "snap" it to logical musical grid (1/16, 1/8T) specifically for notation display (non-destructive to audio).
*   [ ] **2.2 Chord Recognition (C++)**
    *   Implement Interval analysis logic (e.g., Root + Major 3rd + Perfect 5th = Major Triad).
    *   **Binding:** Push string (e.g., "Cmaj7") to the Webview during playback.
*   [ ] **2.3 Sheet Music Renderer (Hybrid)**
    *   **C++:** Determine Key Signature and Pitch logic.
    *   **C++:** Generate a "Glyph List" (JSON array of note positions).
    *   **JS:** Render the Glyph List using an HTML5 Canvas or SVG library (like VexFlow) to draw the staff.
*   [ ] **2.4 Guitar Fretboard Solver (Hybrid)**
    *   **C++:** Implement Standard Tuning (EADGBE) logic.
    *   **C++:** Run "Pathfinding Algorithm" to determine string/fret coordinates (Cost of Movement).
    *   **JS:** Render a 6-line Tablature view using CSS or SVG based on coordinates received from C++.

**SUCCESS CRITERIA:** Playing a C Major scale on the MIDI keyboard triggers the C++ engine to calculate the theory, sending JSON to the Webview which renders correct Sheet Music and Tablature instantly.

## Phase 3: Deep AI Integration
*Objective: Full bi-directional composition workflow.*

*   [ ] **3.1 Diff-Based Importing**
    *   Update JSON parser to support "Partial Updates" (e.g., LLM sends only Track 2; App updates Track 2, leaves Track 1 alone).
*   [ ] **3.2 Context Optimization**
    *   Implement "Token Saver": Algorithm to abbreviate JSON (e.g., using RLE compression for repeated notes) to fit larger songs into LLM context windows.
*   [ ] **3.3 Style Injection**
    *   Create Internal Templates (Genre definitions: Techno, Jazz, Rock).
    *   "Suggest Accompaniment" feature: Sends current chord progression + Genre Template to LLM API (optional) or internal logic to generate a bassline.

**SUCCESS CRITERIA:** User can paste a modified JSON block from ChatGPT/Claude that changes only the melody while keeping the drums, and the app updates instantly without reloading the whole project.

## Phase 4: Production UX & Logic
*Objective: Workflow efficiency and audio manipulation.*

*   [ ] **4.1 Clip Launcher (Non-Linear)**
    *   **C++:** Implement "Session View" engine logic (Trigger Queues, Sync-to-Bar).
    *   **JS:** Implement an HTML Grid Layout (Tracks vs Scenes).
    *   **Binding:** Clicking an HTML Cell triggers C++ `queueClip(track, slot)`.
*   [ ] **4.2 Audio Recording to Disk**
    *   Implement writing stream to `.wav` file on hard drive in chunks (to avoid RAM overflow).
    *   **Waveform Viz:** C++ calculates "Peak Data" (Min/Max volume per chunk) and sends a lightweight array to JS.
    *   **JS:** Renders the waveform on `<canvas>` using the peak data.
*   [ ] **4.3 Elastic Audio (Time Stretching)**
    *   Implement Phase Vocoder or WSOLA algorithm (C++).
    *   Allow BPM change without altering the pitch of recorded audio clips.

**SUCCESS CRITERIA:** User can launch clips live via the HTML grid, record a guitar while the clips play, and then speed up the tempo; the guitar recording stretches to match the new tempo without pitch shifting.

## Phase 5: Final Polish & Optimization
*Objective: Stability and Release.*

*   [ ] **5.1 Thread Safety Audit**
    *   Use TSAN (Thread Sanitizer) to detect data races.
    *   Ensure the Webview Message Loop never blocks the Audio Processing callback.
*   [ ] **5.2 CPU Optimization**
    *   Implement SIMD instructions (SSE/AVX) for heavy DSP math.
    *   Implement "Freeze Track" (Render track to audio to save CPU).
*   [ ] **5.3 Installer**
    *   Create Windows Installer (.msi) bundling necessary VS Redistributables and the `WebView2` loader.

**SUCCESS CRITERIA:** Software passes 24-hour stress test (looping playback) without crashing or increasing memory usage.