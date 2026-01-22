#pragma once

#include <JuceHeader.h>
#include <vector>

struct NoteEvent
{
    int note;
    float velocity;
    double startBeat;
    double durationBeats;

    bool operator< (const NoteEvent& other) const { return startBeat < other.startBeat; }
};

class ProjectModel
{
public:
    ProjectModel() = default;

    void addNote (NoteEvent note)
    {
        notes.push_back (note);
        std::sort (notes.begin(), notes.end());
    }

    void clear() { notes.clear(); }

    const std::vector<NoteEvent>& getNotes() const { return notes; }

private:
    std::vector<NoteEvent> notes;
};

class Transport
{
public:
    Transport() : bpm (120.0), isPlaying (false), currentBeat (0.0) {}

    void setBpm (double newBpm) { bpm = newBpm; }
    double getBpm() const { return bpm; }

    void setPlaying (bool shouldPlay) 
    { 
        if (shouldPlay != isPlaying)
        {
            isPlaying = shouldPlay; 
            if (!isPlaying) currentBeat = 0.0; // Reset on stop for MVP
        }
    }
    bool getIsPlaying() const { return isPlaying; }

    void advance (int numSamples, double sampleRate)
    {
        if (!isPlaying) return;

        double secondsPerBeat = 60.0 / bpm;
        double beatsPerSample = 1.0 / (sampleRate * secondsPerBeat);
        currentBeat += numSamples * beatsPerSample;
        
        // Loop every 4 bars (16 beats) for now
        if (currentBeat >= 16.0)
            currentBeat -= 16.0;
    }

    double getCurrentBeat() const { return currentBeat; }

private:
    double bpm;
    bool isPlaying;
    double currentBeat;
};
