#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <memory>

namespace plugitwin
{
    class PluginHost
    {
    public:
        using ScanProgressFn = std::function<void(int current,
                                                int total,
                                                const juce::String& currentName)>;
        
        
        void setScanScratchDirectory(const juce::File& dir) {scanScratchDir = dir;}

        void addDefaultFolders(bool showInCustom = false);

        int scanFolderOnce(const juce::File& folder, ScanProgressFn onProgress = {});                            
        int scanForPlugins(ScanProgressFn onProgress = {});
        PluginHost();
        ~PluginHost();

        void requestScanStop() noexcept {scanCancelled.store(true);}
        bool isScanCancelled() const noexcept {return scanCancelled.load();}

        void clearKnownPlugins();

        juce::Array<juce::File> getSearchFolders() const;

        void addCustomFolder(const juce::File& folder);
        void removeCustomFolder(const juce::File& folder);

        // Persist / restore the custom folder list across runs.
        void saveCustomFoldersTo(juce::PropertiesFile& props) const;
        bool loadCustomFoldersFrom(const juce::PropertiesFile& props);

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
        int runScanLoop(juce::FileSearchPath& paths,
                        ScanProgressFn onProgress);

        juce::Array<juce::File> customFolders;

        juce::AudioPluginFormatManager formatManager;
        juce::KnownPluginList          knownPlugins;
        juce::FileSearchPath           searchPaths;

        std::atomic<bool> scanCancelled {false};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)

        juce::File deadMansPedal;
        juce::File scanScratchDir;
    };
}