# Project Component: UI / UX
Goal: Create a stunning, Ableton-quality UI without writing complex C++ graphics code.

## 1. The Pivot: JUCE + Webview
Instead of using JUCE's native Component classes for drawing, we will embed a Web Browser engine inside the window.
Backend (C++): Handles Audio, MIDI, File I/O, AI Logic.
Frontend (HTML/CSS/JS): Handles the visuals (Knobs, Piano Roll, Timeline).

### 2. Benefits for LLM Orchestration
Speed: I (the LLM) can generate a "Dark Mode CSS Grid Layout" in seconds. Generating the equivalent in C++ would take multiple iteration cycles and complex coordinate math.
Modernity: We can use libraries like React.js or Vue.js to manage the UI state.
Visuals: We can use CSS shadows, animations, and gradients to make it look like a high-end product immediately.

### 3. Architecture: The "Binding" Layer
We need a bridge so Javascript can talk to C++.
C++ -> JS: When the playback passes a beat, C++ sends an event: emitEvent("playhead", position). JS updates the cursor position.
JS -> C++: When user drags a knob, JS calls: native.setParameter("cutoff", 0.5). C++ updates the DSP filter.

### 4. Implementation Steps (Revised for MVP)
Dependencies: Add juce_gui_extra module (contains WebBrowserComponent).
Setup: Configure the Main Window to load index.html from the Resources folder.
Development:
You ask me to "Create an HTML slider that looks like an Ableton knob."
I give you the HTML/CSS.
You drop it in the folder.
It renders instantly.

### 5. Risk Mitigation
Latency: There is a slight delay between the UI and Audio (approx 15ms). This is acceptable for visual feedback (meters), but the Audio Engine must remain purely C++. The Webview never processes sound, only graphics.
