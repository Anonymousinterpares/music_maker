#include "MainComponent.h"

MainComponent::MainComponent()
{
    juce::Logger::writeToLog ("--- MainComponent Constructor Starting ---");

    addAndMakeVisible (nativeBtn);
    nativeBtn.setButtonText ("NATIVE C++ BEEP (FALLBACK)");
    nativeBtn.onClick = [this] {
        isPlayingTestSound = !isPlayingTestSound;
        nativeBtn.setColour (juce::TextButton::buttonColourId, isPlayingTestSound.load() ? juce::Colours::red : juce::Colours::darkgrey);
    };

    auto userDataPath = juce::File ("C:\\music_maker\\WebViewData");
    if (!userDataPath.exists()) userDataPath.createDirectory();

    auto options = juce::WebBrowserComponent::Options{}.withBackend (juce::WebBrowserComponent::Options::Backend::webview2).withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}.withUserDataFolder (userDataPath)).withNativeIntegrationEnabled (true).withNativeFunction ("testSound", [this] (const juce::Array<juce::var>&, auto completion) {
            isPlayingTestSound = !isPlayingTestSound;
            juce::Logger::writeToLog ("BRIDGE SUCCESS: testSound called!");
            completion (true);
        }).withEventListener ("toggleSoundEvent", [this] (juce::var) {
            isPlayingTestSound = !isPlayingTestSound;
            juce::Logger::writeToLog ("BRIDGE SUCCESS: toggleSoundEvent triggered!");
        }).withResourceProvider ([this] (const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource> {
            std::vector<std::byte> data;
            data.assign ((const std::byte*) BinaryData::index_html, (const std::byte*) BinaryData::index_html + BinaryData::index_htmlSize);
            return juce::WebBrowserComponent::Resource { std::move (data), "text/html" };
        }, juce::WebBrowserComponent::getResourceProviderRoot());

    webBrowser = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (*webBrowser);
    
    webBrowser->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (1000, 700);
    setAudioChannels (0, 2);
    juce::Logger::writeToLog ("--- MainComponent Constructor Finished ---");
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    auto cyclesPerSample = 440.0 / sampleRate;
    phaseDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (isPlayingTestSound.load())
    {
        auto* leftBuffer  = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
        auto* rightBuffer = bufferToFill.buffer->getWritePointer (1, bufferToFill.startSample);

        for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
        {
            float sampleValue = (float)std::sin (currentPhase) * 0.125f;
            currentPhase += phaseDelta;
            leftBuffer[sample] = sampleValue;
            rightBuffer[sample] = sampleValue;
        }
    }
    else
    {
        bufferToFill.clearActiveBufferRegion();
    }
}

void MainComponent::releaseResources() {}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    nativeBtn.setBounds (area.removeFromTop (40).reduced (5));
    webBrowser->setBounds (area);
}