#include "Colours.h"

#include "PluginChainView.h"

#include "PluginEditorWindow.h"

namespace plugitwin
{
    static constexpr int kRowHeight     = 56;
    static constexpr int kRowGap        = 6;
    static constexpr int kRowInnerPad   = 10;
    static constexpr int kSmallButtonW  = 32;
    static constexpr int kMediumButtonW = 64;
    static constexpr int kLargeButtonW  = 110;

    // ========================================================================
    //  PluginRowComponent
    // ========================================================================

    PluginRowComponent::PluginRowComponent(PluginChain& chainRef,
                                           const juce::Uuid& slotIdRef)
        : chain(chainRef),
          slotId(slotIdRef)
    {
        nameLabel.setColour(juce::Label::textColourId, Colours::pluginTextColour);
        nameLabel.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
        addAndMakeVisible(nameLabel);

        muteButton.setButtonText("Bypass");
        muteButton.setColour(juce::Label::textColourId, Colours::pluginTextColour);
        muteButton.setClickingTogglesState(true);
        muteButton.onClick = [this] { handleMuteClicked(); };
        addAndMakeVisible(muteButton);

        moveUpButton.setButtonText(juce::String::charToString(0x2191));   // ↑
        moveUpButton.setColour(juce::Label::textColourId, Colours::pluginTextColour);
        moveUpButton.onClick = [this] { handleMoveUpClicked(); };
        addAndMakeVisible(moveUpButton);

        moveDownButton.setButtonText(juce::String::charToString(0x2193)); // ↓
        moveDownButton.setColour(juce::Label::textColourId, Colours::pluginTextColour);
        moveDownButton.onClick = [this] { handleMoveDownClicked(); };
        addAndMakeVisible(moveDownButton);

        showEditorButton.setButtonText("Show editor");
        showEditorButton.setColour(juce::Label::textColourId, Colours::pluginTextColour);
        showEditorButton.onClick = [this] { handleShowEditorClicked(); };
        addAndMakeVisible(showEditorButton);

        removeButton.setButtonText("Remove");
        removeButton.setColour(juce::Label::textColourId, Colours::pluginTextColour);
        removeButton.onClick = [this] { handleRemoveClicked(); };
        addAndMakeVisible(removeButton);

        refreshFromChain();
    }

    PluginRowComponent::~PluginRowComponent()
    {
        editorWindow.reset();
    }

    void PluginRowComponent::paint(juce::Graphics& g)
    {
        // Rounded card background. Darker than the window backdrop so rows
        // pop visually.
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Colours::pluginBackgroundColour);
        g.fillRoundedRectangle(r, 6.0f);

        // Subtle border.
        g.setColour(Colours::pluginBorderColour);
        g.drawRoundedRectangle(r, 6.0f, 1.0f);
    }

    void PluginRowComponent::resized()
    {
        auto area = getLocalBounds().reduced(kRowInnerPad, 8);

        // Right-anchored controls, laid out from the right edge.
        removeButton    .setBounds(area.removeFromRight(kMediumButtonW));  area.removeFromRight(6);
        showEditorButton.setBounds(area.removeFromRight(kLargeButtonW));   area.removeFromRight(6);
        muteButton      .setBounds(area.removeFromRight(kMediumButtonW));  area.removeFromRight(6);
        moveDownButton  .setBounds(area.removeFromRight(kSmallButtonW));   area.removeFromRight(2);
        moveUpButton    .setBounds(area.removeFromRight(kSmallButtonW));   area.removeFromRight(10);

        // Whatever's left on the left
        nameLabel.setBounds(area);
    }

    void PluginRowComponent::refreshFromChain()
    {
        // Look up the slot by UUID. If it's gone (slot was removed under us),
        // there's nothing to refresh; the parent view will destroy us shortly.
        for (int i = 0; i < chain.getPluginCount(); ++i)
        {
            const auto info = chain.getSlotInfo(i);

            if (info.id == slotId)
            {
                nameLabel.setText(info.displayName, juce::dontSendNotification);
                muteButton.setToggleState(info.muted, juce::dontSendNotification);
                muteButton.setButtonText(info.muted ? "Bypassed" : "Bypass");
                return;
            }
        }
    }

    void PluginRowComponent::setRowIndex(int newIndex, int newTotalRows)
    {
        rowIndex  = newIndex;
        totalRows = newTotalRows;

        moveUpButton  .setEnabled(rowIndex > 0);
        moveDownButton.setEnabled(rowIndex < totalRows - 1);
    }

    void PluginRowComponent::handleMuteClicked()
    {
        // TODO: Check this out
        // setClickingTogglesState already toggled muteButton's visual state.
        chain.setPluginMuted(slotId, muteButton.getToggleState());
        muteButton.setButtonText(muteButton.getToggleState() ? "Bypassed" : "Bypass");
    }

    void PluginRowComponent::handleMoveUpClicked()
    {
        chain.movePlugin(slotId, rowIndex - 1);
    }

    void PluginRowComponent::handleMoveDownClicked()
    {
        chain.movePlugin(slotId, rowIndex + 1);
    }

    void PluginRowComponent::handleShowEditorClicked()
    {
        // If already open, just bring to front.
        if (editorWindow != nullptr)
        {
            editorWindow->setVisible(true);
            editorWindow->toFront(true);
            return;
        }

        auto* instance = chain.getPluginInstance(slotId);

        if (instance == nullptr)
            return;

        editorWindow = std::make_unique<PluginEditorWindow>(*this, *instance,
                                                            nameLabel.getText());
    }

    void PluginRowComponent::handleRemoveClicked()
    {
        editorWindow.reset();
        chain.removePlugin(slotId);
        // After this call, PluginChain notifies its listeners
    }

    void PluginRowComponent::notifyEditorClosed()
    {
        juce::MessageManager::callAsync([this]
        {
            editorWindow.reset();
        });
    }

    void PluginChainView::RowContainer::resized()
    {
        auto area = getLocalBounds();

        for (auto& row : rows)
        {
            row->setBounds(area.removeFromTop(kRowHeight));
            area.removeFromTop(kRowGap);
        }
    }

    PluginChainView::PluginChainView(PluginChain& chainRef)
        : chain(chainRef)
    {
        viewport.setViewedComponent(&rowContainer, /*deleteOnRemoval*/ false);
        viewport.setScrollBarsShown(/*vertical*/ true, /*horizontal*/ false);
        addAndMakeVisible(viewport);

        emptyLabel.setText("No plugins in the chain yet.\nClick \"Add plugin\" "
                           "to load one.",
                           juce::dontSendNotification);
        emptyLabel.setJustificationType(juce::Justification::centred);
        emptyLabel.setColour(juce::Label::textColourId, Colours::pluginChainColour);
        emptyLabel.setFont(juce::Font(juce::FontOptions(14.0f)));
        addAndMakeVisible(emptyLabel);

        chain.addListener(this);

        rebuildRows();
    }

    PluginChainView::~PluginChainView()
    {
        chain.removeListener(this);
    }

    void PluginChainView::paint(juce::Graphics& g)
    {
        // Slight inner background tint to differentiate the chain area from
        // the surrounding window.
        g.fillAll(Colours::pluginChainInnerColour);
    }

    void PluginChainView::resized()
    {
        viewport.setBounds(getLocalBounds());

        const int rowCount = (int) rowContainer.rows.size();
        const int contentHeight = rowCount > 0
            ? rowCount * kRowHeight + (rowCount - 1) * kRowGap
            : 0;

        rowContainer.setSize(viewport.getMaximumVisibleWidth(),
                             juce::jmax(contentHeight, viewport.getHeight()));

        emptyLabel.setBounds(getLocalBounds());
        emptyLabel.setVisible(rowCount == 0);
    }

    void PluginChainView::pluginChainChanged()
    {
        juce::MessageManager::callAsync([this]
        {
            rebuildRows();
        });
    }

    void PluginChainView::rebuildRows()
    {
        // Snapshot of slot IDs we want to display.
        std::vector<juce::Uuid> ids;
        ids.reserve((size_t) chain.getPluginCount());
        for (int i = 0; i < chain.getPluginCount(); ++i)
            ids.push_back(chain.getSlotInfo(i).id);

        rowContainer.rows.clear();

        rowContainer.rows.reserve(ids.size());
        for (const auto& id : ids)
        {
            auto row = std::make_unique<PluginRowComponent>(chain, id);
            rowContainer.addAndMakeVisible(*row);
            rowContainer.rows.push_back(std::move(row));
        }

        // Tell each row its position so move-up/down buttons enable correctly.
        for (int i = 0; i < (int) rowContainer.rows.size(); ++i)
            rowContainer.rows[(size_t) i]->setRowIndex(i, (int) rowContainer.rows.size());

        // Trigger a layout pass to position everything.
        resized();
        rowContainer.resized();
    }
}