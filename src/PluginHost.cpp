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
        formatManager.addFormat(new juce::VST3PluginFormat());

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
        if (folder.isDirectory())
            searchPaths.add(folder);
    }

    int PluginHost::scanForPlugins()
    {
        // The VST3 format object knows how to recognise a VST3 file and
        // extract its descriptions. We ask it to walk our search paths.
        auto* vst3Format = formatManager.getFormat(0);   // we only registered one
        if (vst3Format == nullptr)
            return knownPlugins.getNumTypes();

        juce::PluginDirectoryScanner scanner(
            knownPlugins,
            *vst3Format,
            searchPaths,
            /*recursive*/         true,
            /*deadMansPedalFile*/ {},
            /*allowAsync*/        false);

        juce::String pluginBeingScanned;
        while (scanner.scanNextFile(/*dontRescanIfAlreadyInList*/ true,
                                    pluginBeingScanned))
        {
            // TODO: Add visual progress indicator
        }

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
}