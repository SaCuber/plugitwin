#pragma once

#include <JuceHeader.h>

#include <memory>

#include "AppSettings.h"

namespace plugitwin
{
    class AudioEngine;
    class PluginChain;
    class PluginHost;
    class MainWindow;
    class TrayIcon;

    class TrayApplication : public juce::JUCEApplication
    {
    public:
        TrayApplication();
        ~TrayApplication() override;

        const juce::String getApplicationName() override;
        const juce::String getApplicationVersion() override;
        bool moreThanOneInstanceAllowed() override;

        void initialise(const juce::String& commandLine) override;
        void shutdown() override;

        void systemRequestedQuit() override;
        void anotherInstanceStarted(const juce::String& commandLine) override;

        void showMainWindow();

        void hideMainWindow();

        void deleteAllConfigs();

        void requestQuit();

        AudioEngine&  getAudioEngine()  noexcept;
        PluginChain&  getPluginChain()  noexcept;
        PluginHost&   getPluginHost()   noexcept;

        void loadPersistedState();
        void savePersistedState();
        AppSettings& getSettings() noexcept {return settings;}

    private:
        std::unique_ptr<PluginHost>  pluginHost;
        std::unique_ptr<PluginChain> pluginChain;
        std::unique_ptr<AudioEngine> audioEngine;
        std::unique_ptr<TrayIcon>    trayIcon;
        std::unique_ptr<MainWindow>  mainWindow;

        AppSettings settings;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrayApplication)
    };
}