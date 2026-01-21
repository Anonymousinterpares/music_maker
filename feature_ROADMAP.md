# Part 1: Feature Roadmap

## 1.1 The MVP (Minimum Viable Product)
Objective: Build the core "Sound Engine" and the "AI-Bridge" capable of looping, synthesizing, and serializing music.

### A. Core Audio Engine & Transport
Driver Support: Windows WASAPI (Shared and Exclusive modes) for low-latency audio.
Sample Rate: Fixed at 44.1kHz / 48kHz.
Master Bus: Stereo Output with a Limiter (to prevent clipping/ear damage).
Transport Control: Play, Stop, Record, BPM (Tempo), and Loop Region Toggle.
Mixer Graph: Basic summing mixer allowing volume and pan control for 4 tracks.

### B. Instrument & Sound Generation
Hybrid Synth Engine: A single synthesizer class capable of Subtractive Synthesis (Oscillators: Sine, Saw, Square, Triangle) with a Low-Pass Filter and ADSR Envelope.
Basic Sampler: A module to load a single WAV/MP3 file, map it to the keyboard (C3 = root pitch), and perform real-time pitch shifting using linear interpolation.
Effect Rack: One slot per track containing a Basic Delay and Simple Reverb.

### C. Sequencer & Arrangement (The "Ableton" Logic)
Session View (Clip Launcher): A grid-based system where you can trigger loops (Clips) independently of the timeline. (Essential for the "Ableton" feel).
Piano Roll: A graphical editor for MIDI notes (Pitch, Start Time, Duration, Velocity).
Quantization: "Snap-to-grid" functionality (1/4, 1/8, 1/16 notes).

### D. Input & Recording
MIDI Input: Support for generic USB MIDI Keyboards (Note On/Off, Velocity).
Audio Recording: Mono input recording (Microphone/Guitar) writing directly to RAM buffers (saved to disk on stop).

### E. AI & Data Bridge
JSON Serialization: The ability to save the entire project state (Notes, Sound Settings, Tempo) into a human-readable JSON file.
LLM "Read" Mode: A specific view that displays the song structure as text code for the user to copy/paste to an LLM.

## 1.2 The FULL Product (Commercial Spec)
Objective: A professional DAW with advanced theory tools and full bi-directional AI generation.

### A. Advanced Audio & Hosting
ASIO Driver Support: Professional Steinberg ASIO protocol integration for near-zero latency.
Plugin Hosting: VST3 and CLAP sandbox hosting (allowing external plugins like Serum or Kontakt).
Elastic Audio: Real-time time-stretching algorithms (changing BPM without affecting pitch) using spectral processing.
Unlimited Tracks: Dynamic memory allocation for N-number of tracks.

### B. Advanced Synthesis & Effects
Wavetable Synthesis: Loading 3D wavetables for complex sound design.
Multi-Zone Sampling: Mapping different samples to different key ranges (e.g., Key C1 plays a Kick drum, Key D1 plays a Snare).
Full DSP Suite: Parametric EQ (5-band), Compressor (with sidechain), Distortion, Chorus, Flanger, Phaser.

### C. Music Theory & Notation Engine
Smart Notation: An algorithm that analyzes MIDI data and renders it as standard Sheet Music (Treble/Bass clef).
Guitar Tablature Generator: An algorithm that calculates the optimal fret/string position for a given MIDI pitch (e.g., deciding whether to play Middle C on the B string or G string) based on context.
Chord Analyzer: Real-time text display of harmony (e.g., "G7(b9)") used to feed the AI context.
Scale Constraints: A "lock" feature that prevents the user (or the AI) from placing notes outside a selected scale (e.g., Mixolydian).

### D. The "AI Composer" Integration
Diff-Based Injection: The app can read a JSON snippet from an LLM and only update the specific track requested (e.g., "Keep the drums, but replace the bassline").
Style Prompts: A built-in menu to generate MIDI patterns procedurally based on genre descriptions (which are translated to JSON templates).
Code-to-Audio Rendering: The ability for the LLM to define a synthesizer patch via text parameters, and the engine instantly matches that sound.
