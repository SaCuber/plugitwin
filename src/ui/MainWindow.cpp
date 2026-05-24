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
            scanButton.onClick = [this] { handleScanClicked(); };
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
            const auto statusTop = statusArea.getY();
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

            // Middle: whatever's left goes to the chain view.
            chainView.setBounds(area.reduced(12, 8));
        }

        void setStatusText(const juce::String& text)
        {
            statusLabel.setText(text, juce::dontSendNotification);
        }

    private:
        void handleScanClicked()
        {
            scanButton.setEnabled(false);
            setStatusText("Scanning...");

            // Run the scan on a background thread so the UI doesn't freeze
            // for the (potentially several-second) duration. When done,
            // hop back to the message thread to update the UI.
            juce::Thread::launch([this]
            {
                auto& host = owner.getPluginHost();
                const auto count = host.scanForPlugins();

                juce::MessageManager::callAsync([this, count]
                {
                    scanButton.setEnabled(true);
                    setStatusText("Scan complete. " + juce::String(count)
                                  + " plugin(s) registered and ready to be added.");
                });
            });
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

        // Cached layout regions so paint() can draw separators at the
        // boundaries computed by resized(). Plain rectangles, no ownership.
        juce::Rectangle<int> headerArea;
        juce::Rectangle<int> statusArea;

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