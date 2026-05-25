#include "TrayApplication.h"

#include "AudioEngine.h"
#include "PluginChain.h"
#include "PluginHost.h"
#include "ui/MainWindow.h"

namespace plugitwin
{
    class TrayIcon : public juce::SystemTrayIconComponent
    {
    public:
        explicit TrayIcon(TrayApplication& ownerApp)
            : owner(ownerApp)
        {
            // TODO: Add Image
            juce::Image placeholder(juce::Image::ARGB, 16, 16, true);
            juce::Graphics g(placeholder);
            g.fillAll(juce::Colour::fromRGB(0, 255, 0));

            setIconImage(placeholder, placeholder);
            setIconTooltip("PlugitWin");
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            juce::ignoreUnused(e);

            // Left click: toggle the main window.
            // Right click: show a menu with Open / Quit.
            if (e.mods.isPopupMenu())
            {
                showContextMenu();
            }
            else
            {
                owner.showMainWindow();
            }
        }

    private:
        void showContextMenu()
        {
            juce::PopupMenu menu;
            menu.addItem(1, "Open PlugitWin");
            menu.addSeparator();
            menu.addItem(2, "Quit");

            // The lambda is the callback invoked when the user picks an item.
            // We capture `this` so we can reach `owner`. The menu manages its
            // own lifetime; we don't hold onto it.
            menu.showMenuAsync(juce::PopupMenu::Options(),
                [this](int result)
                {
                    switch (result)
                    {
                        case 1: owner.showMainWindow(); break;
                        case 2: owner.requestQuit();    break;
                        default: break;   // 0 = user dismissed the menu
                    }
                });
        }

        TrayApplication& owner;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrayIcon)
    };

    TrayApplication::TrayApplication()  = default;
    TrayApplication::~TrayApplication() = default;

    const juce::String TrayApplication::getApplicationName()    { return "PlugitWin"; }
    const juce::String TrayApplication::getApplicationVersion() { return "0.1.0";     }
    bool TrayApplication::moreThanOneInstanceAllowed()          { return false;       }

    void TrayApplication::initialise(const juce::String& commandLine)
    {
        juce::ignoreUnused(commandLine);

        pluginHost = std::make_unique<PluginHost>();

        pluginChain = std::make_unique<PluginChain>(*pluginHost);

        audioEngine = std::make_unique<AudioEngine>(*pluginChain);

        loadPersistedState();
        audioEngine->start();

        trayIcon = std::make_unique<TrayIcon>(*this);

        // TODO: Remove this so it doesn't show on startup
        showMainWindow();
    }

    void TrayApplication::shutdown()
    {
        savePersistedState();

        mainWindow.reset();

        trayIcon.reset();

        if (audioEngine != nullptr)
            audioEngine->stop();
        audioEngine.reset();

        pluginChain.reset();

        pluginHost.reset();
    }

    // OS asks us to quit, (e.g. Windows shutdown). We comply
    void TrayApplication::systemRequestedQuit()
    {
        requestQuit();
    }

    void TrayApplication::anotherInstanceStarted(const juce::String& commandLine)
    {
        juce::ignoreUnused(commandLine);
        // if the user launches PlugitWin a second time, JUCE forwards the launch to us
        showMainWindow();
    }

    void TrayApplication::loadPersistedState()
    {
        auto& props = settings.getProps();

        {
            const auto recoveryFile = juce::File::getSpecialLocation(
                    juce::File::userApplicationDataDirectory).getChildFile("PlugitWin").getChildFile("scanRecovery.txt");
            
            recoveryFile.getParentDirectory().createDirectory();
            pluginHost->setDeadMansPedalFile(recoveryFile);

            pluginHost->loadCustomFoldersFrom(props);
            pluginHost->restoreKnownPluginsFromXml(props.getValue("knownPlugins", {}));
        }

        {
            const auto deviceXml = props.getValue("audioDeviceState", {});

            if (deviceXml.isNotEmpty())
            {
                if (auto parsed = juce::parseXML(deviceXml)) {
                    audioEngine->setSavedDeviceState(std::move(parsed));
                }
            }
        }

        {
            const auto chainXml = props.getValue("pluginChain", {});
            if (chainXml.isNotEmpty()) {
                pluginChain->restoreStateFromXml(chainXml);
            }
        }
    }

    void TrayApplication::savePersistedState()
    {
        if (pluginHost == nullptr || pluginChain == nullptr || audioEngine == nullptr) return;

        auto& props = settings.getProps();

        if (auto deviceXml = audioEngine->getDeviceManager().createStateXml()) {
            props.setValue("audioDeviceState", deviceXml->toString());
        }

        props.setValue("knownPlugins", pluginHost->getKnownPluginsAsXml());
        pluginHost->saveCustomFoldersTo(props);

        props.setValue("pluginChain", pluginChain->getStateAsXml());

        settings.saveIfNeeded();
    }

    void TrayApplication::showMainWindow()
    {
        if (mainWindow == nullptr)
            mainWindow = std::make_unique<MainWindow>(*this);

        mainWindow->setVisible(true);
        mainWindow->toFront(true);
    }

    void TrayApplication::hideMainWindow()
    {
        // Destroying the window (rather than just hiding it) keeps things
        // simple: each time the user opens the window it's a fresh instance
        // that reflects current state. Cheap because the window is small
        //
        // If we later find rebuild cost is annoying, we can switch to
        // setVisible(false) and keep it around
        mainWindow.reset();
    }

    void TrayApplication::requestQuit()
    {
        quit();
    }

    AudioEngine& TrayApplication::getAudioEngine() noexcept  { return *audioEngine;  }
    PluginChain& TrayApplication::getPluginChain() noexcept  { return *pluginChain;  }
    PluginHost&  TrayApplication::getPluginHost()  noexcept  { return *pluginHost;   }
}