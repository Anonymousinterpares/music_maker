#pragma once

#include <JuceHeader.h>
#include <fstream>
#include <vector>
#include <mutex>

class RealTimeLogger : private juce::Thread
{
public:
    RealTimeLogger() : juce::Thread ("RealTimeLogger")
    {
        auto logFile = juce::File ("C:\\music_maker\\debug_log.txt");
        if (!logFile.exists()) logFile.create();
        
        fileStream.open (logFile.getFullPathName().toRawUTF8(), std::ios::out | std::ios::app);
        fifo.setTotalSize (1000);
        startThread();
    }

    ~RealTimeLogger() override
    {
        stopThread (2000);
        if (fileStream.is_open()) fileStream.close();
    }

    static void log (const juce::String& message)
    {
        if (auto* instance = getInstance())
            instance->pushMessage (message);
    }

    // Get logs for UI (called from Message Thread)
    static juce::StringArray getPendingUiLogs()
    {
        if (auto* instance = getInstance())
        {
            std::lock_guard<std::mutex> lock (instance->uiLogMutex);
            auto logs = instance->uiLogs;
            instance->uiLogs.clear();
            return logs;
        }
        return {};
    }

private:
    void pushMessage (const juce::String& message)
    {
        auto timestamp = juce::Time::getCurrentTime().toString (true, true);
        auto msgWithTimestamp = timestamp + ": " + message;
        
        int start1, size1, start2, size2;
        fifo.prepareToWrite (1, start1, size1, start2, size2);
        
        if (size1 > 0)
        {
            messages[start1] = msgWithTimestamp;
            fifo.finishedWrite (1);
        }

        // Also push to UI queue (thread safe)
        {
            std::lock_guard<std::mutex> lock (uiLogMutex);
            uiLogs.add (msgWithTimestamp);
            if (uiLogs.size() > 100) uiLogs.remove (0);
        }
    }

    void run() override
    {
        while (!threadShouldExit())
        {
            int start1, size1, start2, size2;
            int numReady = fifo.getNumReady();
            
            if (numReady > 0)
            {
                fifo.prepareToRead (numReady, start1, size1, start2, size2);
                for (int i = 0; i < size1; ++i) fileStream << messages[start1 + i] << "\n";
                for (int i = 0; i < size2; ++i) fileStream << messages[start2 + i] << "\n";
                fileStream.flush();
                fifo.finishedRead (size1 + size2);
            }
            wait (100);
        }
    }

    static RealTimeLogger* getInstance()
    {
        static RealTimeLogger instance;
        return &instance;
    }

    juce::AbstractFifo fifo { 1000 };
    juce::String messages[1000];
    std::ofstream fileStream;

    std::mutex uiLogMutex;
    juce::StringArray uiLogs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeLogger)
};