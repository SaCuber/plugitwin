#include "MainWindow.h"
#include "Colours.h"

#include "../TrayApplication.h"
#include "../AudioEngine.h"
#include "../PluginChain.h"
#include "../PluginHost.h"
#include "PluginChainView.h"

namespace plugitwin
{
    class MainWindow::Content : public juce::Component
    {
    public:
        explicit Content(TrayApplication& ownerApp)
            : owner(ownerApp),
              chainView(ownerApp.getPluginChain())
        {
            titleLabel.setText("PlugitWin", juce::dontSendNotification);
            titleLabel.setFont(juce::Font(juce::FontOptions(22.0f, juce::Font::bold)));
            titleLabel.setColour(juce::Label::textColourId, Colours::titleColour);
            addAndMakeVisible(titleLabel);
            

            scanButton.setButtonText("Scan plugins");
            scanButton.setTooltip("Click: scan default VST3 folders.\n"
                                "Right-click: manage custom folders.");
            scanButton.onClick = [this] { handleScanClicked(); };
            // Enable right-click handling on the button.
            scanButton.addMouseListener(this, false);
            addAndMakeVisible(scanButton);

            addButton.setButtonText("Add plugin");
            addButton.onClick = [this] { handleAddClicked(); };
            addAndMakeVisible(addButton);

            settingsButton.setButtonText("Audio settings");
            settingsButton.onClick = [this] { handleSettingsClicked(); };
            addAndMakeVisible(settingsButton);

            addAndMakeVisible(chainView);

            statusLabel.setText("Ready. Click \"Scan plugins\" to discover VST3s.",
                                juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, Colours::statusLabelColour);
            statusLabel.setFont(juce::Font(juce::FontOptions(13.0f)));
            addAndMakeVisible(statusLabel);

            progressBar.setPercentageDisplay(true);
            progressBar.setVisible(false); // hidden until a scan starts
            addAndMakeVisible(progressBar);
        }

        void showScanOptionsMenu()
        {
            auto& host = owner.getPluginHost();

            juce::PopupMenu menu;
            menu.addSectionHeader("Custom plugin folders");

            const auto current = host.getSearchFolders();

            const auto customs = host.getCustomFolders();

            if (customs.isEmpty())
            {
                menu.addItem(juce::PopupMenu::Item("(none added)").setEnabled(false));
            }
            else
            {
                for (int i = 0; i < customs.size(); ++i)
                {
                    const auto& f = customs.getReference(i);
                    // IDs 1000+ = "remove folder at index i"
                    menu.addItem(1000 + i,
                                "Remove: " + f.getFullPathName(),
                                /*isEnabled*/ true,
                                /*isTicked*/ true);
                }
            }

            menu.addSeparator();
            menu.addItem(1, "Add custom folder...");
            menu.addItem(2, "Scan a folder once (don't save)...");

            const auto options = juce::PopupMenu::Options()
                .withTargetComponent(&scanButton);

            juce::Component::SafePointer<Content> safeThis(this);

            menu.showMenuAsync(options, [safeThis, customs](int result)
            {
                if (safeThis == nullptr || result == 0) return;

                auto& host = safeThis->owner.getPluginHost();

                if (result == 1)
                {
                    safeThis->pickFolderAndAdd(/*persist*/ true);
                }
                else if (result == 2)
                {
                    safeThis->pickFolderAndAdd(/*persist*/ false);
                }
                else if (result >= 1000)
                {
                    const int idx = result - 1000;
                    if (idx >= 0 && idx < customs.size())
                    {
                        host.removeCustomFolder(customs.getReference(idx));
                        safeThis->setStatusText("Removed folder: "
                            + customs.getReference(idx).getFullPathName());
                    }
                }
            });
        }

        void pickFolderAndAdd(bool persist)
        {
            // FileChooser must outlive the async call, so keep it as a member.
            folderChooser = std::make_unique<juce::FileChooser>(
                "Select a VST3 folder",
                juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                juce::String{});

            const auto FileFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectDirectories;

            juce::Component::SafePointer<Content> safeThis(this);

            folderChooser->launchAsync(FileFlags,
                [safeThis, persist](const juce::FileChooser& fc)
                {
                    const auto folder = fc.getResult();
                    if (folder == juce::File{} || ! folder.isDirectory())
                        return;

                    auto& host = safeThis->owner.getPluginHost();

                    if (persist)
                    {
                        host.addCustomFolder(folder);
                        // TODO: Save Custom Folders
                        // safeThis->owner.savePluginHostSettings();

                        safeThis->runScan([&host](PluginHost::ScanProgressFn cb){return host.scanForPlugins(cb); },
                            "Added folder: " + folder.getFullPathName()
                            + " — starting full scan...");
                    }
                    else
                    {
                        safeThis->runScan(
                            [&host, folder](PluginHost::ScanProgressFn cb){return host.scanFolderOnce(folder, cb); },
                            "Scanning (one-time): " + folder.getFullPathName());
                    }
                });
        }

        // Component overrides

        void paint(juce::Graphics& g) override
        {
            g.fillAll(Colours::mainUIColour);

            // Thin separator line between header and content.
            g.setColour(Colours::separatorBarColour);
            const auto sep = headerArea.getBottom();
            g.drawHorizontalLine(sep, 0.0f, (float) getWidth());

            // Thin separator above the status bar.
            const auto statusTop = progressArea.getY() - 6;
            g.drawHorizontalLine(statusTop, 0.0f, (float) getWidth());
        }

        void resized() override
        {
            auto area = getLocalBounds();

           // Header: 56 px tall
            headerArea = area.removeFromTop(56);
            {
                // Header Items Padding
                auto h = headerArea.reduced(12, 10);
                // Title indent: 160px
                titleLabel.setBounds(h.removeFromLeft(160));

                h.removeFromRight(0);                            // right-align
                addButton     .setBounds(h.removeFromRight(110).reduced(0, 2));
                h.removeFromRight(6); // Spacing
                scanButton    .setBounds(h.removeFromRight(110).reduced(0, 2));
                h.removeFromRight(6); // Spacing
                settingsButton.setBounds(h.removeFromRight(120).reduced(0, 2));
            }

            // Status: 28 px tall at the bottom.
            statusArea = area.removeFromBottom(28);
            statusLabel.setBounds(statusArea.reduced(12, 4));

            // Progress bar: 18 px tall, just above the status bar.
            // Always laid out, but only visible during a scan.
            progressArea = area.removeFromBottom(24);
            progressBar.setBounds(progressArea.reduced(12, 4));
            progressArea.removeFromBottom(-6);
            area.removeFromBottom(6);

            // Middle: whatever's left goes to the chain view.
            chainView.setBounds(area.reduced(12, 8));
        }

        void setStatusText(const juce::String& text)
        {
            statusLabel.setText(text, juce::dontSendNotification);
        }

    private:
        std::unique_ptr<juce::FileChooser> folderChooser;

        void mouseDown(const juce::MouseEvent& e) override
        {
            // We only care about right-clicks on the scan button.
            if (e.eventComponent == &scanButton
                && e.mods.isPopupMenu())
            {
                showScanOptionsMenu();
            }
        }
        
        void runScan(std::function<int(PluginHost::ScanProgressFn)> scanFn, const juce::String& startingMessage)
        {
            scanButton.setEnabled(false);
            addButton.setEnabled(false);
            scanProgress = 0.0;
            progressBar.setVisible(true);
            setStatusText(startingMessage);
            resized();

            juce::Component::SafePointer<Content> safeThis(this);

            juce::Thread::launch([safeThis, scanFn = std::move(scanFn)]
            {
                if (safeThis == nullptr) return;

                const auto count = scanFn(
                    [safeThis](int current, int total, const juce::String& currentName)
                    {
                        juce::MessageManager::callAsync(
                            [safeThis, current, total, currentName]
                            {
                                if (safeThis == nullptr) return;

                                safeThis->scanProgress = (total > 0)
                                    ? (double) current / (double) total
                                    : 0.0;

                                if (currentName.isNotEmpty())
                                {
                                    const auto shortName =
                                        juce::File(currentName).getFileNameWithoutExtension();

                                    safeThis->setStatusText(
                                        "Scanning (" + juce::String(current) + " / "
                                        + juce::String(total) + "): " + shortName);
                                }
                            });
                    });

                juce::MessageManager::callAsync([safeThis, count]
                {
                    if (safeThis == nullptr) return;

                    safeThis->scanProgress = 1.0;
                    safeThis->progressBar.setVisible(false);
                    safeThis->scanButton.setEnabled(true);
                    safeThis->addButton.setEnabled(true);
                    if (count > 0) {
                        safeThis->setStatusText("Scan complete. " + juce::String(count)
                            + " plugin(s) registered and ready to be added.");
                    } else {
                        safeThis->setStatusText("Scan complete. No plugins found in the directory(s)");
                    }
                    safeThis->resized();
                });
            });
        }

        void handleScanClicked()
        {
            auto& host = owner.getPluginHost();
            runScan([&host](PluginHost::ScanProgressFn cb) { return host.scanForPlugins(cb); }, "Scanning...");
        }

        void handleAddClicked()
        {
            auto& host = owner.getPluginHost();
            const auto descriptions = host.getKnownPlugins();

            if (descriptions.isEmpty())
            {
                setStatusText("No plugins known yet. Click \"Scan plugins\" first.");
                return;
            }

            juce::PopupMenu menu;

            for (int i = 0; i < descriptions.size(); ++i)
            {
                const auto& d = descriptions.getReference(i);
                menu.addItem(i + 1, d.name);
            }

            const auto options = juce::PopupMenu::Options()
                .withTargetComponent(&addButton);

            menu.showMenuAsync(options,
                [this, descriptions](int result)
                {
                    if (result <= 0)
                        return;

                    const auto& d = descriptions.getReference(result - 1);
                    const auto id = owner.getPluginChain().addPlugin(d);

                    if (id.isNull())
                        setStatusText("Failed to load \"" + d.name + "\".");
                    else
                        setStatusText("Added \"" + d.name + "\".");
                });
        }

        void handleSettingsClicked()
        {
            auto& deviceManager = owner.getAudioEngine().getDeviceManager();

            auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(
                deviceManager,
                /*minInputChannels*/  2,
                /*maxInputChannels*/  2,
                /*minOutputChannels*/ 2,
                /*maxOutputChannels*/ 2,
                /*showMidiInputOptions*/  false,
                /*showMidiOutputSelector*/ false,
                /*showChannelsAsStereoPairs*/ true,
                /*hideAdvancedOptions*/    false);

            selector->setSize(480, 360);

            juce::DialogWindow::LaunchOptions opts;
            opts.dialogTitle                  = "Audio settings";
            opts.dialogBackgroundColour       = Colours::settingsDialogBackgroundColour;
            opts.content.setOwned(selector.release());
            opts.componentToCentreAround      = this;
            opts.escapeKeyTriggersCloseButton = true;
            opts.useNativeTitleBar            = true;
            opts.resizable                    = true;

            opts.launchAsync();

            setStatusText("Adjust input/output devices. Close the dialog when done.");
        }

        TrayApplication& owner;

        juce::Label       titleLabel;
        juce::TextButton  scanButton;
        juce::TextButton  addButton;
        juce::TextButton  settingsButton;
        PluginChainView   chainView;
        juce::Label       statusLabel;

        double            scanProgress { 0.0 };  // 0.0 .. 1.0
        juce::ProgressBar progressBar { scanProgress };

        // Cached layout regions so paint() can draw separators at the
        // boundaries computed by resized(). Plain rectangles, no ownership.
        juce::Rectangle<int> headerArea;
        juce::Rectangle<int> statusArea;
        juce::Rectangle<int> progressArea;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Content)
    };

    // ========================================================================
    //  MainWindow
    // ========================================================================

    MainWindow::MainWindow(TrayApplication& ownerApp)
        : juce::DocumentWindow("PlugitWin",
                               Colours::pluginDockColour, // Colour for the main plugin dock
                               juce::DocumentWindow::closeButton),
          owner(ownerApp)
    {

        auto contentPtr = std::make_unique<Content>(ownerApp);
        rawContent = contentPtr.get();
        setContentOwned(contentPtr.release(),  /*resizeToFit*/ false);

        setUsingNativeTitleBar(true);
        setResizable(true, /*useBottomRightCornerResizer*/ false);
        setResizeLimits(560, 320, 1600, 1200);

        // Initial size. Centred on the user's primary display.
        centreWithSize(680, 480);
    }

    MainWindow::~MainWindow()
    {
        clearContentComponent();
    }

    void MainWindow::closeButtonPressed()
    {
        owner.hideMainWindow();
    }
}