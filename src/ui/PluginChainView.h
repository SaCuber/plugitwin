#pragma once

#include <JuceHeader.h>

#include "../PluginChain.h"

#include <memory>
#include <vector>

namespace plugitwin
{
    class PluginEditorWindow;

    class PluginRowComponent : public juce::Component
    {
    public:
        PluginRowComponent(PluginChain& chain, const juce::Uuid& slotId);
        ~PluginRowComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        void refreshFromChain();

        const juce::Uuid& getSlotId() const noexcept { return slotId; }

        void setRowIndex(int newIndex, int totalRows);

    private:
        void handleMuteClicked();
        void handleMoveUpClicked();
        void handleMoveDownClicked();
        void handleShowEditorClicked();
        void handleRemoveClicked();

        PluginChain& chain;
        juce::Uuid   slotId;

        int rowIndex  = 0;
        int totalRows = 0;

        juce::Label      nameLabel;
        juce::TextButton muteButton;
        juce::TextButton moveUpButton;
        juce::TextButton moveDownButton;
        juce::TextButton showEditorButton;
        juce::TextButton removeButton;

        std::unique_ptr<PluginEditorWindow> editorWindow;

        friend class PluginEditorWindow;
        void notifyEditorClosed();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginRowComponent)
    };

    class PluginChainView
        : public juce::Component,
          public PluginChain::Listener
    {
    public:
        explicit PluginChainView(PluginChain& chain);
        ~PluginChainView() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        // PluginChain::Listener
        void pluginChainChanged() override;

    private:
        void rebuildRows();

        class RowContainer : public juce::Component
        {
        public:
            void resized() override;

            std::vector<std::unique_ptr<PluginRowComponent>> rows;
        };

        PluginChain& chain;
        juce::Viewport viewport;
        RowContainer   rowContainer;

        juce::Label emptyLabel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChainView)
    };
}