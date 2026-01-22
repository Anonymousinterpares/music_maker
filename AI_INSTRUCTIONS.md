# AI Bridge: Interaction Protocol

This document defines how an LLM should interpret and modify the Music Maker project JSON.

## 1. Data Schema (Minified Protocol)

The project is serialized as a JSON object with the following structure:

```json
{
  "bpm": 120,
  "synth": {
    "osc": 1, 
    "cut": 2000.0,
    "res": 0.7
  },
  "t1": [
    [60, 0.8, 0.0, 1.0],
    [62, 0.8, 1.0, 1.0]
  ]
}
```

### Fields:
- **bpm**: Beats Per Minute (20.0 to 300.0).
- **synth**:
    - **osc**: Oscillator type (0=Sine, 1=Saw, 2=Square, 3=Triangle).
    - **cut**: Filter Cutoff frequency in Hz (20.0 to 15000.0).
    - **res**: Filter Resonance (0.1 to 15.0).
- **t1 (Track 1)**: An array of Note Events. Each note is a sub-array: `[Pitch, Velocity, StartBeat, Duration]`.
    - **Pitch**: MIDI Note Number (0-127). 60 is Middle C.
    - **Velocity**: Note intensity (0.0 to 1.0).
    - **StartBeat**: Position in the 16-beat loop (0.0 to 15.99).
    - **Duration**: Length of the note in beats.

---

## 2. LLM System Prompt (Copy-Paste this to the LLM)

"You are an expert music composer and sound designer. I will provide you with a JSON object representing a 16-beat musical loop. 

**Your Capabilities:**
1. **Analyze**: Describe the melody, rhythm, and synth settings.
2. **Transform**: Modify the music (e.g., 'Change to a minor key', 'Make it faster', 'Add a rhythmic bassline').
3. **Generate**: Create entirely new patterns from scratch.

**Constraints:**
- The loop is exactly 16 beats long.
- Use the 't1' array for notes: `[pitch, velocity, start_beat, duration]`.
- Keep notes within the 0.0 to 15.99 beat range.
- Return ONLY the raw JSON object so I can import it back into my DAW."

---

## 3. Example Tasks for the LLM

- "Transpose the whole melody up by 5 semitones."
- "Create a C Major Arpeggio using 8th notes (0.5 duration) for the first 4 beats."
- "Change the synth to a Square wave and lower the cutoff to 500Hz for a lo-fi sound."
- "Quantize all notes to the nearest 0.25 beat."
