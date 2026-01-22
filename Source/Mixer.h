#pragma once

#include <JuceHeader.h>
#include "Track.h"
#include <vector>

class Mixer
{
public:
    Mixer() {
        tracks.reserve(32);
    }

    void addTrack (std::unique_ptr<Track> track)
    {
        // On Message Thread
        auto* t = track.release();
        tracks.push_back(t);
        numTracks.store((int)tracks.size());
    }

    ~Mixer() {
        for (auto* t : tracks) delete t;
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock)
    {
        for (auto* t : tracks)
            t->prepareToPlay (sampleRate, samplesPerBlock);
            
        tempBuffer.setSize (2, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
        buffer.clear();
        
        int n = numTracks.load();
        bool anySoloed = false;
        
        for (int i = 0; i < n; ++i) {
            if (tracks[i]->getIsSoloed()) {
                anySoloed = true;
                break;
            }
        }

        for (int i = 0; i < n; ++i)
        {
            auto* track = tracks[i];
            
            if (anySoloed && !track->getIsSoloed())
                continue;
            
            if (track->getIsMuted())
                continue;

            tempBuffer.clear();
            juce::MidiBuffer trackMidi; 
            
            track->processBlock (tempBuffer, trackMidi);
            
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.addFrom (channel, 0, tempBuffer, channel, 0, buffer.getNumSamples());
        }
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* data = buffer.getWritePointer (channel);
            for (int j = 0; j < buffer.getNumSamples(); ++j)
                data[j] = juce::jlimit (-1.0f, 1.0f, data[j]);
        }
    }

    void allNotesOff()
    {
        int n = numTracks.load();
        for (int i = 0; i < n; ++i)
            tracks[i]->allNotesOff();
    }

    void releaseResources()
    {
        for (auto* t : tracks)
            t->releaseResources();
    }

    // Safe access for Message Thread
    int getNumTracks() const { return numTracks.load(); }
    Track* getTrack(int index) const { 
        if (index >= 0 && index < numTracks.load())
            return tracks[(size_t)index];
        return nullptr;
    }

private:
    std::vector<Track*> tracks; 
    std::atomic<int> numTracks { 0 };
    juce::AudioBuffer<float> tempBuffer;
};