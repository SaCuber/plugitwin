#include "Colours.h"

#include "PluginEditorWindow.h"

#include "PluginChainView.h"

namespace plugitwin
{
    class PluginEditorWindow::NoEditorPlaceholder : public juce::Component
    {
    public:
        NoEditorPlaceholder()
        {
            label.setText("This plugin has no editor of its own.",
                          juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            label.setColour(juce::Label::textColourId, Colours::pluginTextColour);
            label.setFont(juce::Font(juce::FontOptions(14.0f)));
            addAndMakeVisible(label);

            setSize(360, 120);
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(Colours::pluginBackgroundColour);
        }

        void resized() override
        {
            label.setBounds(getLocalBounds());
        }

    private:
        juce::Label label;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoEditorPlaceholder)
    };

    PluginEditorWindow::PluginEditorWindow(PluginRowComponent&        ownerRowRef,
                                           juce::AudioPluginInstance& pluginRef,
                                           const juce::String&        windowTitle)
        : juce::DocumentWindow(windowTitle,
                               Colours::mainUIColour,
                               juce::DocumentWindow::closeButton),
          ownerRow(ownerRowRef),
          plugin(pluginRef)
    {
        setUsingNativeTitleBar(true);

        juce::Component* contentComp = plugin.createEditorAndMakeActive();

        if (contentComp == nullptr)
        {
            contentComp = new NoEditorPlaceholder();
        }

        const int initialW = juce::jmax(contentComp->getWidth(),  200);
        const int initialH = juce::jmax(contentComp->getHeight(), 100);

        rawContent = contentComp;
        setContentOwned(contentComp, /*resizeToFitWhenContentChangesSize*/ true);

        bool editorResizable = false;

        if (auto* editor = dynamic_cast<juce::AudioProcessorEditor*>(contentComp))
            editorResizable = editor->isResizable();

        setResizable(editorResizable, false);

        centreWithSize(initialW, initialH);
        setVisible(true);
    }

    PluginEditorWindow::~PluginEditorWindow()
    {
        clearContentComponent();
    }

    void PluginEditorWindow::closeButtonPressed()
    {
        ownerRow.notifyEditorClosed();
    }
}