#pragma once

#include <JuceHeader.h>

namespace plugitwin::StartupShortcut
{
    inline juce::File getShortcutFile()
    {
        auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);

        auto startupFolder = appData.getChildFile("Microsoft").getChildFile("Windows").getChildFile("Start Menu").getChildFile("Programs").getChildFile("Startup");
        
        return startupFolder.getChildFile("PlugitWin.lnk");
    }

    inline bool shortcutExists() 
    {
        return getShortcutFile().existsAsFile();
    }

    inline bool enable()
    {
        const auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);

        const auto link = getShortcutFile();
        link.getParentDirectory().createDirectory();

        if (link.exists()) link.deleteFile();

        return exe.createShortcut("PlugitWin - VST3 Host", link);
    }

    inline bool disable()
    {
        const auto link = getShortcutFile();
        if (!link.existsAsFile()) return true;
        return link.deleteFile();
    }

    inline void apply(bool shouldRunOnStartup)
    {
        if (shouldRunOnStartup) enable();
        else disable();
    }
}