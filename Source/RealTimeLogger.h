#pragma once

#include <JuceHeader.h>
#include <fstream>

class RealTimeLogger : private juce::Thread
{
public:
    RealTimeLogger() : juce::Thread ("RealTimeLogger")
    {
        auto logFile = juce::File ("C:\\music_maker\\debug_log.txt");
        if (!logFile.exists()) logFile.create();
        
        fileStream.open (logFile.getFullPathName().toRawUTF8(), std::ios::out | std::ios::app);
        
        // Pre-allocate FIFO for 1000 messages
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

private:
    void pushMessage (const juce::String& message)
    {
        // This is safe to call from the audio thread
        auto msgWithTimestamp = juce::Time::getCurrentTime().toString (true, true) + ": " + message;
        
        int start1, size1, start2, size2;
        fifo.prepareToWrite (1, start1, size1, start2, size2);
        
        if (size1 > 0)
        {
            messages[start1] = msgWithTimestamp;
            fifo.finishedWrite (1);
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
                
                for (int i = 0; i < size1; ++i)
                    fileStream << messages[start1 + i] << "\n";
                
                for (int i = 0; i < size2; ++i)
                    fileStream << messages[start2 + i] << "\n";
                
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeLogger)
};
