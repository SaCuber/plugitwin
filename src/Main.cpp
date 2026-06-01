#include <JuceHeader.h>

#include "TrayApplication.h"
#include "PluginScanWorker.h"

#if JUCE_WINDOWS
  #include <windows.h>
#endif

#if JUCE_WINDOWS

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    const auto cmdLine = juce::String(GetCommandLineW());

    auto tokens = juce::StringArray::fromTokens(cmdLine, true);

    juce::String vst3Path, outPath;

    for (int i = 0; i <tokens.size() - 1; ++i) {
        if (tokens[i] == "--scan-vst3") vst3Path = tokens[i+1].unquoted();
        if (tokens[i] == "--scan-out") outPath = tokens[i+1].unquoted();
    }

    if (vst3Path.isNotEmpty() && outPath.isNotEmpty()) {
        return plugitwin::runPluginScanWorker(vst3Path, outPath);
    }

    juce::JUCEApplicationBase::createInstance = 
        []() -> juce::JUCEApplicationBase* {return new plugitwin::TrayApplication();};
    
    return juce::JUCEApplicationBase::main();
}

#else
    START_JUCE_APPLICATION(plugitwin::TrayApplication)
#endif