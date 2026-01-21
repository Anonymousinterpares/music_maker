# Project Component: AI Bridge
Goal: Define a data format concise enough for an LLM to read/write a full song without running out of memory (Context Window).

## 1. The Challenge
A standard 3-minute song in raw MIDI XML can easily exceed 150,000 tokens. Most LLMs handle 4k–128k tokens. If the format is too heavy, the LLM will cut off halfway through the song.

## 2. The Solution: "Minified Notation Protocol" (MNP)
We will not use standard JSON naming conventions. We will use a positional array format (like a CSV inside JSON).
Comparison
Bad (Verbose - Standard JSON):
code
JSON
// Cost: ~40 Tokens per note
{ "track": "Bass", "note": "C2", "velocity": 100, "startTime": 1.0, "duration": 0.25 },
{ "track": "Bass", "note": "C2", "velocity": 90,  "startTime": 1.25, "duration": 0.25 }
Good (Optimized - MNP):
code
JSON
// Cost: ~6 Tokens per note
// Schema: [Pitch, Velocity, StartStep, DurationSteps]
"t1": [[36,100,0,4], [36,90,4,4]]

## 3. The Budget Calculation
Assumptions:
Song Length: 3 Minutes (at 120 BPM = 90 Measures).
Density: 4 Tracks. Average of 8 notes per measure per track.
Total Notes: ~2,880 notes.
Token Usage Breakdown:
Header (Tempo, Key, Settings): ~200 tokens.
Track Definition (Synth Patches): ~300 tokens per track (x4) = 1,200 tokens.
Note Data (Using MNP):
Raw numbers (e.g., [36,100,0,4]) ≈ 5 tokens per note.
2,880 notes * 5 tokens = 14,400 tokens.
Total Projected Project Size: ~16,000 Tokens.
Safety Margin: This fits comfortably within GPT-4 (128k context) and Claude 3 (200k context), leaving ample room for the LLM to "speak" and explain its changes.

## 4. Optimization Strategy (The "Zipper")
If the song grows too large, we implement "Pattern Referencing."
Instead of writing the drums for every bar, we define a pattern once and reference it.
"p1": [[36,100,0,4]...] (Pattern 1 defined)
"arrangement": ["p1", "p1", "p2", "p1"]
Result: Reduces token usage by 90% for repetitive music.
