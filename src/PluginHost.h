#pragma once

#include <JuceHeader.h>

#include <memory>

namespace plugitwin
{
    class PluginHost
    {
    public:
        using ScanProgressFn = std::function<void(int current,
                                                int total,
                                                const juce::String& currentName)>;
        

        int scanFolderOnce(const juce::File& folder, ScanProgressFn onProgress = {});                            
        int scanForPlugins(ScanProgressFn onProgress = {});
        PluginHost();
        ~PluginHost();

        juce::Array<juce::File> getSearchFolders() const;

        void addCustomFolder(const juce::File& folder);

        // Remove a previously-added custom folder. No-op if not present.
        void removeCustomFolder(const juce::File& folder);

        // Persist / restore the custom folder list across runs.
        void saveCustomFoldersTo(juce::PropertiesFile& props) const;
        void loadCustomFoldersFrom(const juce::PropertiesFile& props);

        juce::String getKnownPluginsAsXml() const;
        void         restoreKnownPluginsFromXml(const juce::String& xml);
        
        juce::Array<juce::File> getCustomFolders() const { return customFolders; }

        juce::Array<juce::PluginDescription> getKnownPlugins() const;

        std::unique_ptr<juce::AudioPluginInstance> createPluginInstance(
            const juce::PluginDescription& description,
            double                         sampleRate,
            int                            blockSize);
        
        void setDeadMansPedalFile(const juce::File& f) { deadMansPedal = f; }
    private:
        void addDefaultFolders();

        juce::Array<juce::File> customFolders;

        juce::AudioPluginFormatManager formatManager;
        juce::KnownPluginList          knownPlugins;
        juce::FileSearchPath           searchPaths;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)

        juce::File deadMansPedal;
    };
}