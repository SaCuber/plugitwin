#pragma once

#include <JuceHeader.h>

namespace plugitwin
{
    int runPluginScanWorker(const juce::String& vst3Path, const juce::String& outputFilePath);
}