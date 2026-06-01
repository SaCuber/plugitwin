#include "PluginHost.h"

namespace plugitwin
{   
    namespace {
        constexpr int kPluginScanTimeoutMs = 30000;
    }

    PluginHost::PluginHost()
    {
        // Register the VST3 format. After this, formatManager.getNumFormats()
        // returns 1 and we can scan/load VST3 plugins.
        //
        // We deliberately don't call addDefaultFormats() (which would also
        // register VST2, AU, LADSPA depending on platform)
        formatManager.addFormat(std::make_unique<juce::VST3PluginFormat>());
    }

    PluginHost::~PluginHost() = default;

    void PluginHost::addDefaultFolders(bool showInCustom = false)
    {
        // Standard Windows VST3 locations. JUCE's VST3PluginFormat also
        // knows about these internally, but we add them explicitly so they
        // show up in our FileSearchPath and we can display them in the UI.

        const auto programFiles = juce::File::getSpecialLocation(
            juce::File::globalApplicationsDirectory);
        
        const auto commonFiles = programFiles.getChildFile("Common Files/VST3");

        if (showInCustom) {
            addCustomFolder(commonFiles);
        } else {
            searchPaths.add(commonFiles);
        }

        // Also check the user-local VST3 folder if it exists.
        const auto userVst3 = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory)
            .getChildFile("VST3");

        if (userVst3.isDirectory())
            if (showInCustom) {
                addCustomFolder(userVst3);
            } else {
                searchPaths.add(userVst3);
            }
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

        if (customFolders.size() == 0) {
            addDefaultFolders();
        } else {
            for (const auto& f : customFolders) {
                searchPaths.add(f);
            }
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

    bool PluginHost::loadCustomFoldersFrom(const juce::PropertiesFile& props)
    {
        const auto joined = props.getValue("customPluginFolders", {});
        if (joined.isEmpty()) return false;

        juce::StringArray paths;
        paths.addLines(joined);

        for (const auto& p : paths) addCustomFolder(juce::File(p));
        return true;
    }

    void PluginHost::clearKnownPlugins()
    {
        knownPlugins.clear();
    }

    int PluginHost::runScanLoop(juce::FileSearchPath& paths,
                                ScanProgressFn onProgress)
    {
        auto* vst3Format = formatManager.getFormat(0);
        if (vst3Format == nullptr) return knownPlugins.getNumTypes();

        const auto filesToScan = vst3Format->searchPathsForPlugins(
            paths,
            /*recursive*/ true,
            /*allowPluginsWhichRequireAsynchornousInstantiation*/ false);
        
        const int total = filesToScan.size();
        if (onProgress) onProgress(0, total, {});

        const auto selfExe = juce::File::getSpecialLocation(
            juce::File::currentExecutableFile);
        
        juce::File scratchDir = scanScratchDir;
        if (scratchDir == juce::File{} || !scratchDir.isDirectory()) {
            scratchDir.createDirectory();
            if (!scratchDir.isDirectory()) scratchDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        }

        int scanned = 0;

        for (const auto& vst3Path : filesToScan)
        {
            if (scanCancelled.load()) break;

            ++scanned;
            if (onProgress) onProgress(scanned, total, vst3Path);

            bool alreadyKnown = false;
            for (const auto& known : knownPlugins.getTypes())
            {
                if (juce::File(known.fileOrIdentifier) == juce::File(vst3Path)) {
                    alreadyKnown = true;
                    break;
                }
            }
            if (alreadyKnown) continue;

            if (deadMansPedal != juce::File{}) deadMansPedal.replaceWithText(vst3Path);

            const auto outFile = scratchDir.getChildFile(
                "scan_" + juce::String(juce::Uuid().toString()) + ".xml");
            
            juce::StringArray args;
            args.add(selfExe.getFullPathName());
            args.add("--scan-vst3");
            args.add(vst3Path);
            args.add("--scan-out");
            args.add(outFile.getFullPathName());

            juce::ChildProcess child;
            if (!child.start(args, 0)) {
                DBG("PluginHost: failed to spawn a scan worker oops");
                outFile.deleteFile();
                continue;
            }

            bool finished = false;
            const auto startTime = juce::Time::getMillisecondCounter();

            while (!finished) {
                if (child.waitForProcessToFinish(100)) {finished = true; break;}
                if (scanCancelled.load()) {child.kill(); child.waitForProcessToFinish(2000); break;}
                if ((int) (juce::Time::getMillisecondCounter() - startTime) >= kPluginScanTimeoutMs) {
                    DBG("PluginHost: scan worker timed out");
                    child.kill();
                    child.waitForProcessToFinish(2000);
                    break;
                }
            }

            if (!finished) {
                outFile.deleteFile();
                continue;
            }
            if (child.getExitCode() != 0) {
                outFile.deleteFile();
                continue;
            }

            const auto output = outFile.loadFileAsString();
            outFile.deleteFile();

            if (output.isEmpty()) continue;

            auto xml = juce::parseXML(output);
            if (xml == nullptr || !xml->hasTagName("PLUGIN_SCAN_RESULT")) continue;

            for (auto* descXml : xml->getChildIterator()) {
                juce::PluginDescription desc;
                if (desc.loadFromXml(*descXml)) knownPlugins.addType(desc);
            }
        }

        if (deadMansPedal != juce::File{} && deadMansPedal.existsAsFile()) deadMansPedal.deleteFile();

        if (onProgress) onProgress(total, total, {});
        return knownPlugins.getNumTypes();
    }

    int PluginHost::scanFolderOnce(const juce::File& folder, ScanProgressFn onProgress)
    {
        scanCancelled.store(false);

        if (!folder.isDirectory()) return knownPlugins.getNumTypes();
        
        juce::FileSearchPath localPath;
        localPath.add(folder);

        return runScanLoop(searchPaths, std::move(onProgress));
    }

    int PluginHost::scanForPlugins(ScanProgressFn onProgress)
    {
        scanCancelled.store(false);
        return runScanLoop(searchPaths, std::move(onProgress));
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