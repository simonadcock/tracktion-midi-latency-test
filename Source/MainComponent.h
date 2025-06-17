#pragma once

#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>

class MainComponent  : public juce::AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    tracktion_engine::Engine engine;
    std::unique_ptr<tracktion_engine::Edit> edit;
    
    void setupTestTrack();
    void showAudioDeviceSettings();
    void playTestNote();

    juce::TextButton audioSettingsButton { "Audio Settings" };
    juce::TextButton playTestNoteButton { "Play Test Note" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
