#pragma once

#include <JuceHeader.h>

#include <memory>

namespace plugitwin
{
    class PluginChain;

    struct AudioDeviceSettings
    {
        juce::String inputDeviceName;     // empty = system default
        juce::String outputDeviceName;    // empty = system default
        double       sampleRate    = 48000.0;
        int          bufferSize    = 256;       // frames per callback
    };

    class AudioEngine : public juce::AudioIODeviceCallback
    {
    public:
        explicit AudioEngine(PluginChain& chain);
        ~AudioEngine() override;

        // Open devices and start audio.
        void start();

        // Stop audio and close devices.
        void stop();

        void applySettings(const AudioDeviceSettings& newSettings);

        void setSavedDeviceState(std::unique_ptr<juce::XmlElement> xml);

        AudioDeviceSettings getSettings() const;

        juce::StringArray getAvailableInputDevices()  const;
        juce::StringArray getAvailableOutputDevices() const;

        juce::AudioDeviceManager& getDeviceManager() noexcept { return deviceManager; }

        void audioDeviceIOCallbackWithContext(
            const float* const* inputChannelData,
            int                 numInputChannels,
            float* const*       outputChannelData,
            int                 numOutputChannels,
            int                 numSamples,
            const juce::AudioIODeviceCallbackContext& context) override;

        void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
        void audioDeviceStopped() override;
        void audioDeviceError(const juce::String& errorMessage) override;

    private:
        juce::String openDevices(const AudioDeviceSettings& s);

        PluginChain& pluginChain;

        juce::AudioDeviceManager deviceManager;

        juce::AudioBuffer<float> scratchBuffer;

        juce::MidiBuffer emptyMidi;

        AudioDeviceSettings settings;

        mutable juce::CriticalSection settingsLock;

        bool running = false;

        std::unique_ptr<juce::XmlElement> savedDeviceState;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
    };
}