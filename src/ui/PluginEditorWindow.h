#pragma once

#include <JuceHeader.h>

namespace plugitwin
{
    class PluginRowComponent;

    class PluginEditorWindow : public juce::DocumentWindow
    {
    public:
        PluginEditorWindow(PluginRowComponent&         ownerRow,
                           juce::AudioPluginInstance&  pluginInstance,
                           const juce::String&         windowTitle);

        ~PluginEditorWindow() override;

        void closeButtonPressed() override;

    private:
        class NoEditorPlaceholder;

        PluginRowComponent&        ownerRow;
        juce::AudioPluginInstance& plugin;

        juce::Component* rawContent = nullptr;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
    };
}