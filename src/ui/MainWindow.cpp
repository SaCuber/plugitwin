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

            settingsButton.setButtonText("Settings");
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
            menu.addItem(3, "Add default VST3 folders");

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
                else if (result == 3)
                {
                    host.addDefaultFolders(true);
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
                        safeThis->setStatusText("Added folder: " + folder.getFullPathName());
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

            repaint();
        }

        void setStatusText(const juce::String& text)
        {
            statusLabel.setText(text, juce::dontSendNotification);
        }

    private:

        struct PluginPickerPopup : public juce::Component, private juce::TextEditor::Listener, private juce::ListBoxModel
        {
            PluginPickerPopup(juce::Array<juce::PluginDescription> descs, std::function<void(const juce::PluginDescription&)> onPick)
                : allDescriptions(std::move(descs)),
                onPicked(std::move(onPick))
            {
                searchBox.setTextToShowWhenEmpty("Search Plugins...", Colours::searchBarTextColour);
                searchBox.addListener(this);
                searchBox.onReturnKey = [this] {pickCurrent();};
                searchBox.onEscapeKey = [this] {dismiss();};
                searchBox.setWantsKeyboardFocus(true);
                addAndMakeVisible(searchBox);

                list.setModel(this);
                list.setRowHeight(22);
                list.setColour(juce::ListBox::backgroundColourId, Colours::mainUIColour);
                list.setMouseCursor(juce::MouseCursor::PointingHandCursor);
                addAndMakeVisible(list);

                rebuildFiltered();
                setSize(320, 360);
            }

            void resized() override
            {
                auto area = getLocalBounds().reduced(4);
                searchBox.setBounds(area.removeFromTop(26));
                area.removeFromTop(4);
                list.setBounds(area);
            }

            void parentHierarchyChanged() override
            {
                if (isShowing() && !searchBox.hasKeyboardFocus(true)) searchBox.grabKeyboardFocus();
            }

            bool keyPressed(const juce::KeyPress& key) override
            {
                if (key == juce::KeyPress::downKey) {moveCurrent(+1); return true;}
                if (key == juce::KeyPress::upKey) {moveCurrent(-1); return true;}
                if (key == juce::KeyPress::pageDownKey) {moveCurrent(+8); return true;}
                if (key == juce::KeyPress::pageUpKey) {moveCurrent(-8); return true;}
                return false;
            }

            void mouseMove(const juce::MouseEvent& e) override
            {
                updateHoverFromScreenPos(e.getScreenPosition());
            }

            void mouseExit(const juce::MouseEvent&) override {};

            void textEditorTextChanged(juce::TextEditor&) override
            {
                rebuildFiltered();
            }

            int getNumRows() override {return filtered.size();}

            void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
            {
                if (rowNumber<0 || rowNumber > filtered.size()) return;

                if (rowIsSelected) g.fillAll(Colours::searchHighlightColour);

                g.setColour(Colours::addListItemTextColour);
                g.setFont((float) height * 0.6f);
                g.drawText(filtered.getReference(rowNumber).name, 8, 0, width-16, height, juce::Justification::centredLeft, true);
            }

            void listBoxItemClicked(int row, const juce::MouseEvent&) override {setCurrentRow(row); pickCurrent();}

        private:

            void rebuildFiltered()
            {
                const auto query = searchBox.getText().trim().toLowerCase();
                filtered.clearQuick();

                if (query.isEmpty()) filtered = allDescriptions;
                else {
                    for (const auto& d : allDescriptions) {
                        if (d.name.toLowerCase().contains(query)) filtered.add(d);
                    }
                }
                list.updateContent();
                setCurrentRow(filtered.isEmpty() ? -1 : 0);
            }

            void setCurrentRow(int row)
            {
                if (row == currentRow) return;
                currentRow = row;
                list.selectRow(row, false, true);
                list.repaint();
            }

            void moveCurrent(int delta)
            {
                if (filtered.isEmpty()) return;
                int next = juce::jlimit(0, filtered.size() -1, (currentRow < 0 ? 0: currentRow) + delta);
                setCurrentRow(next);
            }

            void updateHoverFromScreenPos(juce::Point<int> screenPos)
            {
                const auto listLocal = list.getLocalPoint(nullptr, screenPos);
                if (!list.getLocalBounds().contains(listLocal)) return;

                const int row = list.getRowContainingPosition(listLocal.x, listLocal.y);
                if (row >= 0) setCurrentRow(row);
            }

            void pickCurrent()
            {
                if (currentRow < 0 || currentRow >= filtered.size()) return;
                const auto chosen = filtered.getReference(currentRow);
                auto cb = onPicked;
                dismiss();
                if (cb) cb(chosen);
            }

            void dismiss()
            {
                if (auto* box = findParentComponentOfClass<juce::CallOutBox>()) box->dismiss();
            }

            juce::Array<juce::PluginDescription> allDescriptions;
            juce::Array<juce::PluginDescription> filtered;
            std::function<void(const juce::PluginDescription&)> onPicked;

            juce::TextEditor searchBox;
            juce::ListBox list;
            int currentRow {-1};
        };


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
            scanInProgress = true;
            scanButton.setButtonText("Stop Scanning");
            scanButton.setEnabled(true);
            addButton.setEnabled(false);
            //settingsButton.setEnabled(false); MAYBE

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

                    const bool wasCancelled = safeThis->owner.getPluginHost().isScanCancelled();

                    safeThis->scanProgress = 1.0;
                    safeThis->progressBar.setVisible(false);
                    safeThis->scanInProgress = false;
                    safeThis->scanButton.setButtonText("Scan plugins");
                    safeThis->scanButton.setEnabled(true);
                    safeThis->addButton.setEnabled(true);
                    safeThis->settingsButton.setEnabled(true);

                    if (wasCancelled) {
                        safeThis->setStatusText("Scan stopped. " + juce::String(count)
                            + " plugin(s) registered so far.");
                    } else if (count > 0) {
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

            if (scanInProgress)
            {
                //Stop request
                host.requestScanStop();
                scanButton.setEnabled(false);
                scanButton.setButtonText("Stopping...");
                setStatusText("Stop requested - finishing current plugin...");
                return;
            }

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

            juce::Component::SafePointer<Content> safeThis(this);

            auto picker = std::make_unique<PluginPickerPopup>(descriptions, 
                [safeThis] (const juce::PluginDescription& d)
                {
                    if (safeThis == nullptr) return;
                    
                    const auto id = safeThis->owner.getPluginChain().addPlugin(d);

                    if (id.isNull()) safeThis->setStatusText("Failed to load \"" + d.name + "\".");
                    else safeThis->setStatusText("Added \"" + d.name + "\".");

                    safeThis->owner.savePersistedState(); // Save :)
                });

            auto& box = juce::CallOutBox::launchAsynchronously(std::move(picker), addButton.getScreenBounds(), nullptr);

            juce::ignoreUnused(box);
        }

        void handleSettingsClicked()
        {
            auto& deviceManager = owner.getAudioEngine().getDeviceManager();

            struct SettingsContent : public juce::Component
            {
                SettingsContent(juce::AudioDeviceManager& dm, TrayApplication& app, SafePointer<Content> safeThis)
                    : owner(app),
                      selector(dm,
                            /*minInputChan*/ 2,   /*maxInputChan*/2,
                            /*minOutChan*/ 2,     /*maxOutChan*/ 2,
                            /*midiIn*/ false,     /*midiOut*/ false,
                            /*stereoPairs*/ true, /*hideAdvanced*/ false)
                {
                    addAndMakeVisible(selector);

                    resetButton.setButtonText("Reset config");
                    resetButton.setTooltip("Erase EVERYTHING, BE CAREFUL!");
                    resetButton.setColour(juce::TextButton::buttonColourId, Colours::resetButtonColour);
                    resetButton.setColour(juce::TextButton::textColourOffId, Colours::resetButtonOffColour);
                    resetButton.onClick = [this, safeThis] {owner.deleteAllConfigs(); safeThis->setStatusText("Reset Configs.");};
                    addAndMakeVisible(resetButton);

                    setSize(480, 410);
                }

                void resized() override
                {
                    auto area = getLocalBounds();
                    auto bottom = area.removeFromBottom(40).reduced(12,6);
                    resetButton.setBounds(bottom.removeFromRight(130));
                    selector.setBounds(area);
                }

                TrayApplication&                    owner;
                juce::AudioDeviceSelectorComponent  selector;
                juce::TextButton                    resetButton;
            };
            
            juce::Component::SafePointer<Content> safeThis(this);
            auto content = std::make_unique<SettingsContent>(deviceManager, owner, safeThis);

            juce::DialogWindow::LaunchOptions opts;
            opts.dialogTitle                  = "Settings";
            opts.dialogBackgroundColour       = Colours::settingsDialogBackgroundColour;
            opts.content.setOwned(content.release());
            opts.componentToCentreAround      = this;
            opts.escapeKeyTriggersCloseButton = true;
            opts.useNativeTitleBar            = true;
            opts.resizable                    = true;

            auto* dlg = opts.launchAsync();

            if (dlg != nullptr) {
                dlg->enterModalState(true, juce::ModalCallbackFunction::create(
                    [safeThis](int){
                        if (safeThis != nullptr) safeThis->owner.savePersistedState();
                    }),
                true);
            }
        }

        TrayApplication& owner;

        juce::Label       titleLabel;
        juce::TextButton  scanButton;
        juce::TextButton  addButton;
        juce::TextButton  settingsButton;
        PluginChainView   chainView;
        juce::Label       statusLabel;

        bool scanInProgress {false};
        double            scanProgress {0.0};  // 0.0 .. 1.0
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

        if (auto* peer = getPeer()) peer->setCurrentRenderingEngine(0);
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

    void MainWindow::resized()
    {
        juce::DocumentWindow::resized();

        if (auto* c = getContentComponent()) c->repaint();
    }
}