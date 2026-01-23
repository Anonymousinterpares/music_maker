#pragma once

#include <JuceHeader.h>
#include <vector>
#include <algorithm>
#include <mutex>

struct NoteEvent
{
    int note;
    float velocity;
    double startBeat;
    double durationBeats;

    bool operator< (const NoteEvent& other) const { return startBeat < other.startBeat; }
    bool operator== (const NoteEvent& other) const 
    { 
        return note == other.note && std::abs(startBeat - other.startBeat) < 0.01; 
    }
};

class ProjectModel
{
public:
    ProjectModel() = default;

    void addNote (NoteEvent note)
    {
        std::lock_guard<std::mutex> lock(modelMutex);
        note.note = juce::jlimit(0, 127, note.note);
        note.velocity = juce::jlimit(0.0f, 1.0f, note.velocity);
        note.durationBeats = std::max(0.01, note.durationBeats);

        // Deduplication: Remove any existing note at the exact same position and pitch
        notes.erase (std::remove_if (notes.begin(), notes.end(), [&](const NoteEvent& e) {
            return e.note == note.note && std::abs(e.startBeat - note.startBeat) < 0.001;
        }), notes.end());

        notes.push_back (note);
        std::sort (notes.begin(), notes.end());
    }

    void removeNote (int note, double startBeat)
    {
        std::lock_guard<std::mutex> lock(modelMutex);
        notes.erase (std::remove_if (notes.begin(), notes.end(), [&](const NoteEvent& e) {
            return e.note == note && std::abs(e.startBeat - startBeat) < 0.1;
        }), notes.end());
    }

    void clear() { 
        std::lock_guard<std::mutex> lock(modelMutex);
        notes.clear(); 
    }

    std::vector<NoteEvent> getNotes() const { 
        std::lock_guard<std::mutex> lock(modelMutex);
        return notes; 
    }

    juce::var toMinifiedVar() const
    {
        std::lock_guard<std::mutex> lock(modelMutex);
        juce::Array<juce::var> notesArray;
        for (const auto& n : notes)
        {
            juce::Array<juce::var> noteData;
            noteData.add(n.note);
            noteData.add(std::round(n.velocity * 100) / 100.0);
            noteData.add(std::round(n.startBeat * 100) / 100.0);
            noteData.add(std::round(n.durationBeats * 100) / 100.0);
            notesArray.add(noteData);
        }
        return notesArray;
    }

    void fromMinifiedVar(const juce::var& v)
    {
        clear();
        if (auto* arr = v.getArray())
        {
            for (auto& noteVar : *arr)
            {
                if (auto* n = noteVar.getArray())
                {
                    if (n->size() >= 4)
                    {
                        addNote({ (int)(*n)[0], (float)(*n)[1], (double)(*n)[2], (double)(*n)[3] });
                    }
                }
            }
        }
    }

    void saveToFile (const juce::File& file)
    {
        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty("bpm", 120.0);
        root->setProperty("t1", toMinifiedVar());
        file.replaceWithText (juce::JSON::toString(juce::var(root)));
    }

private:
    std::vector<NoteEvent> notes;
    mutable std::mutex modelMutex;
};

class Transport
{
public:
    Transport() : bpm (120.0), isPlaying (false), isRecording (false), currentBeat (0.0) {}

    void setBpm (double newBpm) { bpm = juce::jlimit(20.0, 300.0, newBpm); }
    double getBpm() const { return bpm; }

    void setPlaying (bool shouldPlay) 
    { 
        isPlaying = shouldPlay; 
        if (!isPlaying) isRecording = false;
    }
    bool getIsPlaying() const { return isPlaying; }

    void setRecording (bool shouldRecord) 
    { 
        isRecording = shouldRecord; 
        if (isRecording) isPlaying = true;
    }
    bool getIsRecording() const { return isRecording; }

    void advance (int numSamples, double sampleRate)
    {
        if (!isPlaying) return;

        double secondsPerBeat = 60.0 / bpm;
        double beatsPerSample = 1.0 / (sampleRate * secondsPerBeat);
        currentBeat += numSamples * beatsPerSample;
        
        if (currentBeat >= 16.0)
            currentBeat -= 16.0;
    }

    void reset() { currentBeat = 0.0; }
    double getCurrentBeat() const { return currentBeat; }

private:
    double bpm;
    bool isPlaying;
    bool isRecording;
    double currentBeat;
};