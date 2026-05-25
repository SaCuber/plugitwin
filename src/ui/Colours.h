#pragma once

#include <JuceHeader.h>

namespace plugitwin
{   
    class Colours 
    {
        public:
        // MainUIColour = RGB(28,30,34)
        static inline const juce::Colour mainUIColour = juce::Colour::fromRGB(28, 30, 34);

        static inline const juce::Colour pluginDockColour = juce::Colour::fromRGB(28, 30, 34);

        // SeparatorBarColour = #3C4048
        static inline const juce::Colour separatorBarColour = juce::Colour::fromRGB(60, 64, 72);

        //Settings
        static inline const juce::Colour settingsDialogBackgroundColour = juce::Colour::fromRGB(28, 30, 34);

        static inline const juce::Colour titleColour = juce::Colours::white;

        static inline const juce::Colour statusLabelColour = juce::Colours::lightgrey;

        static inline const juce::Colour pluginTextColour = juce::Colours::white;

        static inline const juce::Colour pluginBackgroundColour = juce::Colour::fromRGB(42, 46, 52);

        static inline const juce::Colour pluginBorderColour = juce::Colour::fromRGB(60, 64, 72);
    
        static inline const juce::Colour pluginChainColour = juce::Colours::grey;

        static inline const juce::Colour pluginChainInnerColour = juce::Colour::fromRGB(34, 36, 40);
    };
}