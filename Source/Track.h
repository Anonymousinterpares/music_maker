#pragma once

#include <JuceHeader.h>

enum class TrackType { Audio, Midi };

class Track
{
public:
    Track (const juce::String& name, TrackType type)
        : trackName (name), trackType (type) {}

    virtual ~Track() = default;

    void setVolume (float v) { volume.store (juce::jlimit (0.0f, 2.0f, v)); }
    float getVolume() const { return volume.load(); }

    void setPan (float p) { pan.store (juce::jlimit (-1.0f, 1.0f, p)); }
    float getPan() const { return pan.load(); }

    void setMuted (bool m) { isMuted.store (m); }
    bool getIsMuted() const { return isMuted.load(); }

    void setSoloed (bool s) { isSoloed.store (s); }
    bool getIsSoloed() const { return isSoloed.load(); }

    const juce::String& getName() const { return trackName; }
    TrackType getType() const { return trackType; }

    virtual void prepareToPlay (double sampleRate, int samplesPerBlock) = 0;
    virtual void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) = 0;
    virtual void releaseResources() = 0;
    virtual void allNotesOff() = 0;

protected:
    juce::String trackName;
    TrackType trackType;
    std::atomic<float> volume { 0.8f };
    std::atomic<float> pan { 0.0f };
    std::atomic<bool> isMuted { false };
    std::atomic<bool> isSoloed { false };
};

#include "InternalSynth.h"

class InstrumentTrack : public Track
{
public:
    InstrumentTrack (const juce::String& name) : Track (name, TrackType::Midi) {}

    void setInstrument (std::unique_ptr<juce::AudioProcessor> newInstrument)
    {
        // This happens on the Message Thread
        auto* oldInstrument = instrument.exchange (newInstrument.release());
        if (oldInstrument != nullptr)
            deletionQueue.add (oldInstrument);
    }

    ~InstrumentTrack() override
    {
        if (auto* inst = instrument.load())
            delete inst;
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        if (auto* inst = instrument.load())
            inst->prepareToPlay (sampleRate, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override
    {
        auto* inst = instrument.load();
        if (inst == nullptr || isMuted.load()) {
            buffer.clear();
            return;
        }

        inst->processBlock (buffer, midiMessages);
        
        float v = volume.load();
        float p = pan.load();
        
        // Constant Power Panning (Pro Standard)
        float angle = (p + 1.0f) * (juce::MathConstants<float>::pi / 4.0f);
        float gainL = v * std::cos (angle);
        float gainR = v * std::sin (angle);

        if (buffer.getNumChannels() >= 2) {
            buffer.applyGain (0, 0, buffer.getNumSamples(), gainL);
            buffer.applyGain (1, 0, buffer.getNumSamples(), gainR);
        } else {
            buffer.applyGain (v);
        }
    }

    void allNotesOff() override {
        if (auto* inst = instrument.load()) {
            if (auto* internalSynth = dynamic_cast<InternalSynthProcessor*> (inst)) {
                internalSynth->allNotesOff();
            }
        }
    }

    void releaseResources() override {
        if (auto* inst = instrument.load())
            inst->releaseResources();
    }

    juce::AudioProcessor* getProcessor() const { return instrument.load(); }

private:
    std::atomic<juce::AudioProcessor*> instrument { nullptr };
    juce::OwnedArray<juce::AudioProcessor> deletionQueue; // Simple way to defer deletion
};