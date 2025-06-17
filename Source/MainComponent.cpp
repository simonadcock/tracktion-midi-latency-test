#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
: engine("Latency Test")
{
    addAndMakeVisible(audioSettingsButton);
    audioSettingsButton.onClick = [this] { showAudioDeviceSettings(); };
    
    addAndMakeVisible(playTestNoteButton);
    playTestNoteButton.onClick = [this] { playTestNote(); };
    
    setSize (800, 600);

    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        setAudioChannels (2, 2);
    }
    
    const auto editFilePath = juce::JUCEApplication::getCommandLineParameters().replace ("-NSDocumentRevisionsDebugMode YES", "").unquoted().trim();
    const juce::File editFile (editFilePath);
    edit = tracktion_engine::createEmptyEdit (engine, editFile);
    edit->ensureNumberOfAudioTracks(1);
    
    juce::Timer::callAfterDelay(500, [this]()
    {
        DBG("Delayed setup after engine init");
        setupTestTrack();
        edit->getTransport().play(false);
    });
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

// This sets up a track with 4OSC and MIDI input enabled
void MainComponent::setupTestTrack()
{
    DBG("MainComponent::setupTestTrack()");
    auto track = tracktion::engine::getAudioTracks(*edit)[0];

    if (auto plugin = edit->getPluginCache().createNewPlugin(tracktion::engine::FourOscPlugin::xmlTypeName, {}))
    {
        track->pluginList.insertPlugin(plugin, 0, nullptr);
        if (auto* fourOsc = dynamic_cast<tracktion::engine::FourOscPlugin*>(plugin.get()))
        {
            for (int i = 0; i < fourOsc->oscParams.size(); ++i)
            {
                auto& osc = *fourOsc->oscParams[i];
                osc.waveShapeValue = 3;        // 3 = square
                osc.levelValue = (i == 0) ? 1.0f : 0.0f; // Only osc1 active
            }
            if (fourOsc->ampAttack)   fourOsc->ampAttack->setParameter(0.0f, juce::sendNotification);
            if (fourOsc->ampDecay)    fourOsc->ampDecay->setParameter(0.05f, juce::sendNotification);
            if (fourOsc->ampSustain)  fourOsc->ampSustain->setParameter(1.0f, juce::sendNotification);
            if (fourOsc->ampRelease)  fourOsc->ampRelease->setParameter(0.1f, juce::sendNotification);
        }
    }

    // Enable MIDI input monitoring on all available MIDI input devices
    for (auto& midiIn : engine.getDeviceManager().getMidiInDevices())
    {
        DBG("Set monitor mode");
        DBG(midiIn->getName());
        midiIn->setMonitorMode(tracktion_engine::InputDevice::MonitorMode::on);
        midiIn->setEnabled(true);
    }
    
    edit->getTransport().ensureContextAllocated();

    for (auto instance : edit->getAllInputDevices())
    {
        if (instance->getInputDevice().getDeviceType() == tracktion_engine::InputDevice::physicalMidiDevice)
        {
            [[ maybe_unused ]] auto res = instance->setTarget (track->itemID, true, &edit->getUndoManager(), 0);
            instance->setRecordingEnabled (track->itemID, true);
            DBG("midi in device record enabled");
            DBG(instance->getInputDevice().getName());
        }
    }
}

void MainComponent::showAudioDeviceSettings()
{
    juce::DialogWindow::LaunchOptions o;
    o.dialogTitle = TRANS("Audio Settings");
    o.dialogBackgroundColour = juce::LookAndFeel::getDefaultLookAndFeel()
        .findColour(juce::ResizableWindow::backgroundColourId);

    o.content.setOwned(new juce::AudioDeviceSelectorComponent(
        engine.getDeviceManager().deviceManager,
        0, 512, 1, 512,
        false, false, true, true));

    o.content->setSize(800, 600);
    o.launchAsync();
}

void MainComponent::playTestNote()
{
    DBG("Playing test note...");
    int midiNumber = 60;
    int velocity = 100;
    int durationMs = 500;

    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, midiNumber, (juce::uint8) velocity);
    noteOn.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);

    juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, midiNumber);
    noteOff.setTimeStamp(noteOn.getTimeStamp() + durationMs * 0.001);

    for (auto& midiIn : engine.getDeviceManager().getMidiInDevices())
    {
        if (midiIn->isEnabled())
        {
            midiIn->handleIncomingMidiMessage(nullptr, noteOn);

            juce::Timer::callAfterDelay(durationMs, [midiIn, noteOff]()
            {
                midiIn->handleIncomingMidiMessage(nullptr, noteOff);
                DBG("Playing test note off...");
            });
        }
    }
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{

}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    audioSettingsButton.setBounds(10, 10, 150, 30);
    playTestNoteButton.setBounds(10, 50, 150, 30);
}
