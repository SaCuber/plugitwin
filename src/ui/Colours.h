#pragma once

#include <JuceHeader.h>

namespace plugitwin
{   
    class Colours 
    {
        public:
        // MainUIColour = RGB(28,30,34)
        static inline const juce::Colour mainUIColour = juce::Colour::fromRGB(28, 30, 34);

        static inline const juce::Colour pluginDockColour = mainUIColour;

        // SeparatorBarColour = #3C4048
        static inline const juce::Colour separatorBarColour = juce::Colour::fromRGB(60, 64, 72);

        // Add Menu
        static inline const juce::Colour searchBarTextColour = juce::Colours::white.withAlpha(0.65f);
        static inline const juce::Colour searchHighlightColour = juce::Colours::white.withAlpha(0.3f);
        static inline const juce::Colour addListItemTextColour = searchBarTextColour;

        // Settings
        static inline const juce::Colour settingsDialogBackgroundColour = mainUIColour;

        static inline const juce::Colour titleColour = juce::Colours::white;

        static inline const juce::Colour statusLabelColour = juce::Colours::lightgrey;

        static inline const juce::Colour pluginTextColour = juce::Colours::white;

        static inline const juce::Colour pluginBackgroundColour = juce::Colour::fromRGB(42, 46, 52);

        static inline const juce::Colour pluginBorderColour = separatorBarColour;
    
        static inline const juce::Colour pluginChainColour = juce::Colours::grey;

        static inline const juce::Colour pluginChainInnerColour = juce::Colour::fromRGB(34, 36, 40);

        // Reset Configs
        static inline const juce::Colour resetButtonColour = juce::Colour::fromRGB(160, 30, 30);
        static inline const juce::Colour resetButtonOffColour = juce::Colours::white;
    };
}