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
        notes.push_back (note);
        std::sort (notes.begin(), notes.end());
    }

    void removeNote (int note, double startBeat)
    {
        notes.erase (std::remove_if (notes.begin(), notes.end(), [&](const NoteEvent& e) {
            return e.note == note && std::abs(e.startBeat - startBeat) < 0.1;
        }), notes.end());
    }

    void clear() { notes.clear(); }

    const std::vector<NoteEvent>& getNotes() const { return notes; }

    juce::String toJson() const
    {
        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        juce::Array<juce::var> notesArray;

        for (const auto& n : notes)
        {
            juce::DynamicObject::Ptr noteObj = new juce::DynamicObject();
            noteObj->setProperty ("p", n.note);
            noteObj->setProperty ("v", n.velocity);
            noteObj->setProperty ("s", n.startBeat);
            noteObj->setProperty ("d", n.durationBeats);
            notesArray.add (juce::var (noteObj));
        }

        root->setProperty ("notes", notesArray);
        return juce::JSON::toString (juce::var (root));
    }

    void saveToFile (const juce::File& file)
    {
        file.replaceWithText (toJson());
    }

private:
    std::vector<NoteEvent> notes;
};

class Transport
{
public:
    Transport() : bpm (120.0), isPlaying (false), isRecording (false), currentBeat (0.0) {}

    void setBpm (double newBpm) { bpm = newBpm; }
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