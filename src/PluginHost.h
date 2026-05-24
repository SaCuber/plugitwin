#pragma once

#include <JuceHeader.h>

#include <memory>

namespace plugitwin
{
    class PluginHost
    {
    public:
        PluginHost();
        ~PluginHost();

        int scanForPlugins();

        void addCustomFolder(const juce::File& folder);


        juce::Array<juce::PluginDescription> getKnownPlugins() const;

        std::unique_ptr<juce::AudioPluginInstance> createPluginInstance(
            const juce::PluginDescription& description,
            double                         sampleRate,
            int                            blockSize);

    private:
        void addDefaultFolders();

        juce::AudioPluginFormatManager formatManager;
        juce::KnownPluginList          knownPlugins;
        juce::FileSearchPath           searchPaths;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
    };
}