#include "PluginHost.h"

namespace plugitwin
{
    PluginHost::PluginHost()
    {
        // Register the VST3 format. After this, formatManager.getNumFormats()
        // returns 1 and we can scan/load VST3 plugins.
        //
        // We deliberately don't call addDefaultFormats() (which would also
        // register VST2, AU, LADSPA depending on platform). That keeps our
        // dependency surface small and matches the MVP scope.
        formatManager.addFormat(std::make_unique<juce::VST3PluginFormat>());

        addDefaultFolders();
    }

    PluginHost::~PluginHost() = default;

    void PluginHost::addDefaultFolders()
    {
        // Standard Windows VST3 locations. JUCE's VST3PluginFormat also
        // knows about these internally, but we add them explicitly so they
        // show up in our FileSearchPath and we can display them in the UI.

        const auto programFiles = juce::File::getSpecialLocation(
            juce::File::globalApplicationsDirectory);

        searchPaths.add(programFiles.getChildFile("Common Files/VST3"));

        // Also check the user-local VST3 folder if it exists.
        const auto userVst3 = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory)
            .getChildFile("VST3");

        if (userVst3.isDirectory())
            searchPaths.add(userVst3);
    }

    void PluginHost::addCustomFolder(const juce::File& folder)
    {
        if (!folder.isDirectory()) return;
        
        // Avoid duplicates
        for (const auto& existing : customFolders) {
            if (existing == folder) return;
        }

        customFolders.add(folder);
        searchPaths.add(folder);
    }

    void PluginHost::removeCustomFolder(const juce::File& folder)
    {
        customFolders.removeAllInstancesOf(folder);

        searchPaths = juce::FileSearchPath();
        addDefaultFolders();
        for (const auto& f : customFolders) {
            searchPaths.add(f);
        }
    }

    juce::Array<juce::File> PluginHost::getSearchFolders() const
    {
        juce::Array<juce::File> out;
        for (int i=0; i < searchPaths.getNumPaths(); ++i) out.add(searchPaths[i]);
        return out;
    }

    void PluginHost::saveCustomFoldersTo(juce::PropertiesFile& props) const
    {
        juce::StringArray paths;
        for (const auto& f : customFolders) paths.add(f.getFullPathName());

        props.setValue("customPluginFolders", paths.joinIntoString("\n"));
    }

    void PluginHost::loadCustomFoldersFrom(const juce::PropertiesFile& props)
    {
        const auto joined = props.getValue("customPluginsFolder", {});
        if (joined.isEmpty()) return;

        juce::StringArray paths;
        paths.addLines(joined);

        for (const auto& p : paths) addCustomFolder(juce::File(p));
    }

    int PluginHost::scanFolderOnce(const juce::File& folder, ScanProgressFn onProgress)
    {
        if (! folder.isDirectory())
            return knownPlugins.getNumTypes();

        auto* vst3Format = formatManager.getFormat(0);
        if (vst3Format == nullptr)
            return knownPlugins.getNumTypes();

        // Build a local FileSearchPath containing only this folder.
        // We deliberately do NOT touch this->searchPaths.
        juce::FileSearchPath localPath;
        localPath.add(folder);

        const auto filesToScan = vst3Format->searchPathsForPlugins(
            localPath,
            /*recursive*/ true,
            /*allowPluginsWhichRequireAsynchronousInstantiation*/ false);

        const int total = filesToScan.size();

        juce::PluginDirectoryScanner scanner(
            knownPlugins,
            *vst3Format,
            localPath,
            /*recursive*/         true,
            /*deadMansPedalFile*/ deadMansPedal,
            /*allowAsync*/        true);

        juce::String pluginBeingScanned;
        int scanned = 0;

        if (onProgress) onProgress(0, total, {});

        while (scanner.scanNextFile(/*dontRescanIfAlreadyInList*/ true,
                                    pluginBeingScanned))
        {
            ++scanned;
            if (onProgress)
                onProgress(scanned, total, pluginBeingScanned);
        }

        if (onProgress) onProgress(total, total, {});
        return knownPlugins.getNumTypes();
    }

    int PluginHost::scanForPlugins(ScanProgressFn onProgress)
    {
        // The VST3 format object knows how to recognise a VST3 file and
        // extract its descriptions. We ask it to walk our search paths.
        auto* vst3Format = formatManager.getFormat(0);   // we only registered one
        if (vst3Format == nullptr)
            return knownPlugins.getNumTypes();
        

        // Collect candidate files up-front so we have a meaningful "total"
        // for the progress bar. JUCE's scanner walks lazily otherwise.
        const auto filesToScan = vst3Format->searchPathsForPlugins(
        searchPaths,
        /*recursive*/             true,
        /*allowPluginsWhichRequireAsynchronousInstantiation*/ false);
        
        const int total = filesToScan.size();
        
        juce::PluginDirectoryScanner scanner(
            knownPlugins,
            *vst3Format,
            searchPaths,
            /*recursive*/         true,
            /*deadMansPedalFile*/ deadMansPedal,
            /*allowAsync*/        true);

        juce::String pluginBeingScanned;
        int scanned = 0;

        // Report initial state so the UI can switch to "0 / N".
        if (onProgress) onProgress(0, total, {});


        while (scanner.scanNextFile(/*dontRescanIfAlreadyInList*/ true, pluginBeingScanned))
        {
            ++scanned;

            if (onProgress) onProgress(scanned, total, pluginBeingScanned);
        }

        if (onProgress) onProgress(total, total, {});
        return knownPlugins.getNumTypes();
    }

    juce::Array<juce::PluginDescription> PluginHost::getKnownPlugins() const
    {
        juce::Array<juce::PluginDescription> result;

        for (const auto& type : knownPlugins.getTypes())
            result.add(type);

        return result;
    }

    std::unique_ptr<juce::AudioPluginInstance> PluginHost::createPluginInstance(
        const juce::PluginDescription& description,
        double                         sampleRate,
        int                            blockSize)
    {
        // If audio hasn't been opened yet, JUCE wants a non-zero sample rate
        // anyway. Fall back to a sensible default so the plugin constructs
        // cleanly; the real value will arrive via prepareToPlay later.
        if (sampleRate <= 0.0) sampleRate = 48000.0;
        if (blockSize  <= 0)   blockSize  = 256;

        juce::String errorMessage;

        auto instance = formatManager.createPluginInstance(
            description,
            sampleRate,
            blockSize,
            errorMessage);

        if (instance == nullptr)
        {
            DBG("PluginHost: failed to load \"" << description.name
                << "\": " << errorMessage);
        }

        return instance;
    }
    juce::String PluginHost::getKnownPluginsAsXml() const
    {
        auto xml = knownPlugins.createXml();
        if (xml == nullptr) return {};
        return xml->toString();
    }

    void PluginHost::restoreKnownPluginsFromXml(const juce::String& xml)
    {
        if (xml.isEmpty()) return;
        if (auto parsed = juce::parseXML(xml)) knownPlugins.recreateFromXml(*parsed);
    }

}