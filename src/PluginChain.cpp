#include "PluginChain.h"

#include "PluginHost.h"

namespace plugitwin
{
    PluginChain::PluginChain(PluginHost& hostRef)
        : host(hostRef)
    {
        // Publish an initial empty snapshot so the RT thread, if it somehow
        // runs before any plugin is added, sees a valid (empty) chain rather
        // than a null pointer.
        publishSnapshot();
    }

    PluginChain::~PluginChain()
    {
        // Delete the current snapshot. unique_ptr in retiredSnapshots will
        // delete the rest automatically when the vector goes out of scope.
        delete currentSnapshot.exchange(nullptr);
    }


    void PluginChain::prepareToPlay(double sampleRate, int blockSize)
    {
        currentSampleRate = sampleRate;
        currentBlockSize  = blockSize;
        prepared          = true;

        for (auto& slot : slots)
        {
            if (slot->plugin != nullptr)
                slot->plugin->prepareToPlay(sampleRate, blockSize);
        }
    }

    void PluginChain::releaseResources()
    {
        for (auto& slot : slots)
        {
            if (slot->plugin != nullptr)
                slot->plugin->releaseResources();
        }

        prepared = false;
    }


    void PluginChain::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer&         midi,
                                   int                       numSamples) noexcept
    {
        juce::ignoreUnused(numSamples);

        auto* snap = currentSnapshot.load(std::memory_order_acquire);

        if (snap == nullptr)
            return;   // chain not yet initialised; produce silence (buffer
                      // is already filled by AudioEngine from the input)

        // Walk the snapshot in order. Each entry is just two pointers
        for (const auto& entry : snap->entries)
        {
            if (entry.plugin == nullptr)
                continue;

            if (entry.mutedFlag != nullptr
                && entry.mutedFlag->load(std::memory_order_relaxed))
                continue;

            entry.plugin->processBlock(buffer, midi);
        }
    }

    juce::Uuid PluginChain::addPlugin(const juce::PluginDescription& description)
    {
        // Instantiate the plugin via the host
        auto instance = host.createPluginInstance(description, currentSampleRate, currentBlockSize);

        if (instance == nullptr)
            return {};   // null UUID signals failure to the caller

        // If audio is already running, prepare the plugin now so the RT
        // thread can call processBlock on it immediately.
        if (prepared)
            instance->prepareToPlay(currentSampleRate, currentBlockSize);

        auto slot = std::make_unique<PluginSlot>();
        slot->plugin      = std::move(instance);
        slot->displayName = description.name;
        slot->id          = juce::Uuid();   // generates a random UUID

        const auto resultId = slot->id;

        slots.push_back(std::move(slot));

        publishSnapshot();
        notifyListeners();

        return resultId;
    }

    void PluginChain::removePlugin(const juce::Uuid& slotId)
    {
        const auto idx = indexOf(slotId);
        if (idx < 0)
            return;

        auto removed = std::move(slots[(size_t) idx]);
        slots.erase(slots.begin() + idx);

        publishSnapshot();

        notifyListeners();
    }

    void PluginChain::movePlugin(const juce::Uuid& slotId, int newIndex)
    {
        const auto idx = indexOf(slotId);
        if (idx < 0)
            return;

        newIndex = juce::jlimit(0, (int) slots.size() - 1, newIndex);

        if (newIndex == idx)
            return;

        auto slot = std::move(slots[(size_t) idx]);
        slots.erase(slots.begin() + idx);
        slots.insert(slots.begin() + newIndex, std::move(slot));

        publishSnapshot();
        notifyListeners();
    }

    void PluginChain::setPluginMuted(const juce::Uuid& slotId, bool shouldBeMuted)
    {
        const auto idx = indexOf(slotId);
        if (idx < 0)
            return;

        slots[(size_t) idx]->muted.store(shouldBeMuted, std::memory_order_relaxed);
    }

    int PluginChain::getPluginCount() const
    {
        return (int) slots.size();
    }

    PluginChain::SlotInfo PluginChain::getSlotInfo(int index) const
    {
        SlotInfo info;

        if (index < 0 || index >= (int) slots.size())
            return info;   // default: null UUID, empty name

        const auto& slot = *slots[(size_t) index];
        info.id          = slot.id;
        info.displayName = slot.displayName;
        info.muted       = slot.muted.load(std::memory_order_relaxed);

        return info;
    }

    juce::AudioPluginInstance* PluginChain::getPluginInstance(const juce::Uuid& slotId) const
    {
        const auto idx = indexOf(slotId);
        if (idx < 0)
            return nullptr;

        return slots[(size_t) idx]->plugin.get();
    }

    void PluginChain::addListener(Listener* listener)
    {
        listeners.add(listener);
    }

    void PluginChain::removeListener(Listener* listener)
    {
        listeners.remove(listener);
    }


    void PluginChain::publishSnapshot()
    {
        auto newSnap = std::make_unique<Snapshot>();
        newSnap->entries.reserve(slots.size());

        for (auto& slot : slots)
        {
            Snapshot::Entry e;
            e.plugin    = slot->plugin.get();
            e.mutedFlag = &slot->muted;
            newSnap->entries.push_back(e);
        }

        auto* oldSnap = currentSnapshot.exchange(newSnap.release(),
                                                 std::memory_order_release);
        retiredSnapshots.clear();
        if (oldSnap != nullptr)
            retiredSnapshots.emplace_back(oldSnap);
    }

    void PluginChain::notifyListeners()
    {
        listeners.call([](Listener& l) { l.pluginChainChanged(); });
    }

    int PluginChain::indexOf(const juce::Uuid& slotId) const
    {
        for (int i = 0; i < (int) slots.size(); ++i)
            if (slots[(size_t) i]->id == slotId)
                return i;

        return -1;
    }
}