#include "PluginScanWorker.h"

namespace plugitwin
{
    int runPluginScanWorker(const juce::String& vst3Path,
                            const juce::String& outputFilePath)
    {
        if (vst3Path.isEmpty() || outputFilePath.isEmpty()) return 1;

        juce::AudioPluginFormatManager fm;
        fm.addFormat(std::make_unique<juce::VST3PluginFormat>());

        auto* vst3 = fm.getFormat(0);
        if (vst3 == nullptr) return 1;

        juce::OwnedArray<juce::PluginDescription> found;
        vst3->findAllTypesForFile(found, vst3Path);

        if (found.isEmpty()) return 1;

        juce::XmlElement root("PLUGIN_SCAN_RESULT");
        for (auto* desc : found) {
            if (desc->isInstrument) continue;
            auto child = desc->createXml();
            if (child != nullptr) root.addChildElement(child.release());
        }

        juce::File outFile(outputFilePath);
        outFile.getParentDirectory().createDirectory();

        if (!outFile.replaceWithText(root.toString())) return 1;

        return 0;
    }
}