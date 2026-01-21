# Project Component: Debugging Infrastructure
Goal: Allow "Real-Time" debugging without crashing the Audio Driver.

## 1. The Problem
You cannot use std::cout or printf inside the audio callback (processBlock). These commands pause the CPU to talk to the Operating System.
Result: Audio Glitch / Crackle / Driver Crash.

## 2. The Solution: Ring-Buffer Logger
We need a "Fire and Forget" system. The audio thread throws a message into a box and immediately keeps working. A separate, slower thread empties the box and writes to a text file.

### 3. Technical Implementation
Class: RealTimeLogger
Member 1: LockFreeFifo (First-In-First-Out Queue)
Allocated at startup (e.g., size for 1000 messages).
Must be Atomic (Thread-safe without locks).
Member 2: BackgroundThread
Runs every 100ms.
Checks if the FIFO has data.
If yes, pops data and writes to debug_log.txt.
Usage Protocol
Inside Audio Thread (High Priority):
code
C++
// This must take < 0.001ms
Logger::log("Note On: C3");
Inside Background Thread (Low Priority):
code
C++
// This handles the slow File I/O
fileStream << timestamp << " " << message << "\n";

### 4. Success Criteria
Stress Test: Log a message every single audio buffer (400 times per second).
Result: The generated text file grows rapidly, but the audio output remains perfectly clean with zero stuttering.