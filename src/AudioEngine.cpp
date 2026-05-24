#include "AudioEngine.h"

#include "PluginChain.h"

namespace plugitwin
{
    static constexpr int kNumChannels = 2;

    AudioEngine::AudioEngine(PluginChain& chain)
        : pluginChain(chain)
    {
        // We don't open devices yet; that happens in start(). Keeping the
        // constructor cheap means TrayApplication can build us early without
        // any audio side effects.
    }

    AudioEngine::~AudioEngine()
    {
        stop();
    }

    void AudioEngine::start()
    {
        if (running)
            return;

        const auto error = openDevices(settings);

        if (error.isNotEmpty())
        {
            // If devices can't be opened, the
            // user just gets silence until they pick valid devices from the
            // UI. The UI will show this error string.
            DBG("AudioEngine: failed to open devices: " << error);
            return;
        }

        deviceManager.addAudioCallback(this);
        running = true;
    }

    void AudioEngine::stop()
    {
        if (! running)
            return;

        // removeAudioCallback BLOCKS until any in-flight callback finishes.
        // After it returns, our audioDeviceIOCallbackWithContext is
        // guaranteed not to be called again. Safe to mutate state now.
        deviceManager.removeAudioCallback(this);
        deviceManager.closeAudioDevice();

        running = false;
    }

    void AudioEngine::applySettings(const AudioDeviceSettings& newSettings)
    {
        const bool wasRunning = running;

        if (wasRunning)
            stop();

        {
            const juce::ScopedLock sl(settingsLock);
            settings = newSettings;
        }

        if (wasRunning)
            start();
    }

    AudioDeviceSettings AudioEngine::getSettings() const
    {
        const juce::ScopedLock sl(settingsLock);
        return settings;  // returns a copy; caller can do what it wants with it
    }

    juce::StringArray AudioEngine::getAvailableInputDevices() const
    {
        juce::StringArray names;

        if (auto* type = deviceManager.getCurrentDeviceTypeObject())
        {
            // `true` here = input devices (false = output).
            type->scanForDevices();
            names = type->getDeviceNames(true);
        }

        return names;
    }

    juce::StringArray AudioEngine::getAvailableOutputDevices() const
    {
        juce::StringArray names;

        if (auto* type = deviceManager.getCurrentDeviceTypeObject())
        {
            type->scanForDevices();
            names = type->getDeviceNames(false);
        }

        return names;
    }

    juce::String AudioEngine::openDevices(const AudioDeviceSettings& s)
    {
        //Only allow Windows Audio, no support for ASIO or others
        deviceManager.setCurrentAudioDeviceType("Windows Audio", /*treatAsChosen*/ true);

        juce::AudioDeviceManager::AudioDeviceSetup setup;
        setup.inputDeviceName    = s.inputDeviceName;
        setup.outputDeviceName   = s.outputDeviceName;
        setup.sampleRate         = s.sampleRate;
        setup.bufferSize         = s.bufferSize;
        setup.useDefaultInputChannels  = false;
        setup.useDefaultOutputChannels = false;
        setup.inputChannels .setRange(0, kNumChannels, true);
        setup.outputChannels.setRange(0, kNumChannels, true);

        return deviceManager.initialise(
            /*numInputChannelsNeeded*/  kNumChannels,
            /*numOutputChannelsNeeded*/ kNumChannels,
            /*savedState*/              nullptr,
            /*selectDefaultDeviceOnFailure*/ true,
            /*preferredDefaultDeviceName*/   {},
            /*preferredSetupOptions*/        &setup);
    }

    void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
    {
        const auto sampleRate = device->getCurrentSampleRate();
        const auto bufferSize = device->getCurrentBufferSizeSamples();

        scratchBuffer.setSize(kNumChannels, bufferSize,
                              /*keepExistingContent*/ false,
                              /*clearExtraSpace*/     true,
                              /*avoidReallocating*/   false);

        emptyMidi.ensureSize(256);
        emptyMidi.clear();

        pluginChain.prepareToPlay(sampleRate, bufferSize);
    }

    void AudioEngine::audioDeviceStopped()
    {
        pluginChain.releaseResources();
    }

    void AudioEngine::audioDeviceError(const juce::String& errorMessage)
    {
        DBG("AudioEngine: device error: " << errorMessage);
    }

    void AudioEngine::audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,
        int                 numInputChannels,
        float* const*       outputChannelData,
        int                 numOutputChannels,
        int                 numSamples,
        const juce::AudioIODeviceCallbackContext& context)
    {
        juce::ignoreUnused(context);

        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            auto* dest = scratchBuffer.getWritePointer(ch);

            if (numInputChannels >= kNumChannels)
            {
                // Standard stereo path. memcpy of numSamples floats.
                std::memcpy(dest, inputChannelData[ch], sizeof(float) * (size_t) numSamples);
            }
            else if (numInputChannels == 1)
            {
                // Mono input: duplicate to both output channels.
                std::memcpy(dest, inputChannelData[0], sizeof(float) * (size_t) numSamples);
            }
            else
            {
                // No input at all: silence.
                std::memset(dest, 0, sizeof(float) * (size_t) numSamples);
            }
        }

        emptyMidi.clear();
        pluginChain.processBlock(scratchBuffer, emptyMidi, numSamples);


        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            auto* dest = outputChannelData[ch];

            if (dest == nullptr)
                continue;   // JUCE sometimes passes null for unused channels

            if (ch < kNumChannels)
            {
                // We have a processed channel for this output index.
                std::memcpy(dest, scratchBuffer.getReadPointer(ch),
                            sizeof(float) * (size_t) numSamples);
            }
            else
            {
                // Output device has more channels than we filled (e.g. 5.1
                // speakers but we only produced stereo). Silence the rest.
                std::memset(dest, 0, sizeof(float) * (size_t) numSamples);
            }
        }

        juce::ignoreUnused(numInputChannels);
    }
}