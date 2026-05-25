#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <memory>
#include <vector>

namespace plugitwin
{
    class PluginHost;

    struct PluginSlot
    {
        std::unique_ptr<juce::AudioPluginInstance> plugin;

        juce::String displayName;

        std::atomic<bool> muted { false };

        juce::Uuid id;
    };

    class PluginChain
    {
    public:
        explicit PluginChain(PluginHost& host);
        ~PluginChain();

        void prepareToPlay(double sampleRate, int blockSize);

        // Called from AudioEngine::audioDeviceStopped.
        void releaseResources();

        void processBlock(juce::AudioBuffer<float>& buffer,
                          juce::MidiBuffer&         midi,
                          int                       numSamples) noexcept;


        juce::Uuid addPlugin(const juce::PluginDescription& description);

        void removePlugin(const juce::Uuid& slotId);

        void movePlugin(const juce::Uuid& slotId, int newIndex);

        void setPluginMuted(const juce::Uuid& slotId, bool shouldBeMuted);


        int getPluginCount() const;

        struct SlotInfo
        {
            juce::Uuid   id;
            juce::String displayName;
            bool         muted = false;
        };

        SlotInfo getSlotInfo(int index) const;

        juce::AudioPluginInstance* getPluginInstance(const juce::Uuid& slotId) const;

        juce::String getStateAsXml() const;
        void restoreStateFromXml(const juce::String& xml);

        // Subscribe to chain-changed notifications. The UI uses this to
        // rebuild its plugin list when slots are added/removed/reordered.
        // Callback fires on the GUI thread.
        struct Listener
        {
            virtual ~Listener() = default;
            virtual void pluginChainChanged() = 0;
        };

        void addListener(Listener* listener);
        void removeListener(Listener* listener);

    private:
        void publishSnapshot();

        void notifyListeners();

        int indexOf(const juce::Uuid& slotId) const;

        PluginHost& host;

        std::vector<std::unique_ptr<PluginSlot>> slots;

        struct Snapshot
        {
            struct Entry
            {
                juce::AudioPluginInstance* plugin = nullptr;
                std::atomic<bool>*         mutedFlag = nullptr;
            };
            std::vector<Entry> entries;
        };

        std::atomic<Snapshot*> currentSnapshot { nullptr };

        std::vector<std::unique_ptr<Snapshot>> retiredSnapshots;

        double currentSampleRate = 0.0;
        int    currentBlockSize  = 0;
        bool   prepared          = false;

        juce::ListenerList<Listener> listeners;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChain)
    };
}