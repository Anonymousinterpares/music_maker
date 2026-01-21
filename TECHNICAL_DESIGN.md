# Technical Stack & Environment
OS: Windows 10/11 (64-bit).
Language: C++20 (utilizing concepts, modules, and coroutines where applicable).
Framework: JUCE 7+ (Dual license: GPL/Commercial).
Build System: CMake (for modularity and IDE independence).
IDE: Visual Studio 2022 (MSVC Compiler).
Graphics API: Direct2D / OpenGL (via JUCE) for high-FPS UI rendering.

## 2. System Architecture
We will utilize a Clean Architecture approach with strict separation of concerns.

### 2.1 Layer 1: The DSP Kernel (Audio Thread)
This is the "Real-Time" layer. No memory allocation, no locks, no exceptions.
AudioEngine Class: The root object processed by the audio driver callback.
GraphNode Interface: An abstract base class for everything that makes sound.
SynthesizerNode: Generates audio.
SamplerNode: Manipulates audio buffers.
EffectNode: Modifies incoming audio buffers.
MidiMessageCollector: A lock-free FIFO (First-In-First-Out) queue that passes MIDI events from the UI to the DSP kernel.

### 2.2 Layer 2: The Application Logic (Message Thread)
ProjectModel: The single source of truth. It holds the state of the song (tracks, clips, settings).
NotationEngine:
Input: Raw MIDI data (ticks).
Process: Quantizes timing -> Detects Key Signature -> Assigns Accidentals (Sharps/Flats) -> Calculates Spacing.
Output: Vector of glyph positions (for drawing Sheet Music) and Fret numbers (for Tablature).
TransportController: Manages Play/Stop state and synchronizes the timeline cursor.

### 2.3 Layer 3: The View (GUI)
MainComponent: The root window.
ClipMatrixComponent: The Ableton-style grid view.
PianoRollComponent: The detailed MIDI editor.
NotationComponent: Renders the Sheet Music/Tabs using vector graphics.
Architecture Pattern: We will use ValueTree (JUCE specific data structure) with Listener patterns. When the ProjectModel changes, it broadcasts a message; the UI hears it and repaints.

### 2.4 Layer 4: The AI Bridge (The JSON Protocol)
We will define a strict DSL (Domain Specific Language).
The JSON Schema Specification:
code
JSON
{
  "projectInfo": {
    "bpm": 120,
    "timeSignature": "4/4",
    "key": "C Minor"
  },
  "tracks": [
    {
      "id": 1,
      "name": "Lead Synth",
      "type": "instrument_synth",
      "settings": {
        "oscillator": "saw",
        "filterCutoff": 2000,
        "release": 0.5
      },
      "clips": [
        {
          "startBar": 1,
          "lengthBars": 4,
          "notes": [
            {"p": 60, "v": 100, "t": 0.0, "d": 0.25}, 
            {"p": 62, "v": 90, "t": 0.25, "d": 0.25}
          ]
        }
      ]
    }
  ]
}
Key: p=Pitch, v=Velocity, t=Start Time (in beats), d=Duration.

## 3. Detailed Component Implementation Specs

### 3.1 Audio Engine Standards
To ensure the app does not crackle or crash:
Buffer Handling: Double-buffering is required for recording.
Sample Interpolation: The Sampler must implement Hermite or Lagrange interpolation (better than Linear, faster than Sinc) to prevent aliasing artifacts when pitch-shifting.
Plugin Sandbox: Hosted VSTs will be wrapped in try/catch blocks. If a VST throws an exception, the host bypasses it and logs the error, rather than crashing to desktop.

### 3.2 Music Notation Logic (The "Smart" Layer)
This is the most complex logical component.
Quantization Pre-pass: Raw user performance is "messy." The engine must snap note start times to the nearest logical grid (e.g., 16th note) before attempting to draw sheet music, otherwise, the score will be unreadable.
Guitar Logic (Fretboard Solver):
The algorithm calculates "Cost."
Moving from Fret 2 to Fret 15 has a high "Cost."
Moving from Fret 2 to Fret 4 has a low "Cost."
The solver chooses the string path with the lowest movement cost.

### 3.3 AI Interaction Flow
Export: ProjectModel -> specialized JsonSerializer -> std::string.
LLM Process: User pastes string to LLM; LLM returns modified JSON.
Import: JsonParser -> Validates Schema -> Updates ValueTree -> Triggers UI Repaint -> Updates DSP Audio Graph.

### 4. Coding & Quality Standards

### 4.1 Modularization
The code must be separated into static libraries linked to the main executable:
App_AudioEngine.lib (Strictly DSP code)
App_MusicTheory.lib (Notation and Chord logic)
App_UI.lib (Graphics)
App_DataProtocol.lib (JSON/AI parsing)

### 4.2 C++ Style Guide
Ownership: Explicit ownership. If a class owns a pointer, use std::unique_ptr. If resources are shared (like audio samples used by multiple tracks), use std::shared_ptr.
Immutability: Data structures passed to the Audio Thread must be immutable or atomic.
Formatting: Clang-Format with Microsoft style preset.

### 4.3 Testing Framework
Unit Tests (GTest):
Test the Theory Engine: Input "C, E, G" -> Expect Output "C Major".
Test the JSON Parser: Input Broken JSON -> Expect "Error: Invalid Format" (No crash).
Integration Tests:
Test Rendering: Create a 4-bar loop programmatically, render to WAV, compare checksum against a known good reference.

### 4.4 Documentation Strategy
Code Comments: Doxygen style for every header file.
Diagrams: Mermaid.js graphs included in the repo README detailing the Audio Signal Flow and the Class Hierarchy.