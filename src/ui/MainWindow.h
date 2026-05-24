#pragma once

#include <JuceHeader.h>

#include <memory>

namespace plugitwin
{
    class TrayApplication;
    class PluginChainView;

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(TrayApplication& owner);
        ~MainWindow() override;

        void closeButtonPressed() override;

    private:
        TrayApplication& owner;

        class Content;
        Content* rawContent = nullptr;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };
}