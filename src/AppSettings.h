#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace plugitwin
{
    class AppSettings
    {
        public:
            AppSettings()
            {
                juce::PropertiesFile::Options opts;
                opts.applicationName    = "PlugitWin";
                opts.filenameSuffix     = ".settings";
                opts.osxLibrarySubFolder= "Application Support";
                opts.folderName         = "PlugitWin";
                opts.storageFormat      = juce::PropertiesFile::storeAsXML;

                properties.setStorageParameters(opts);
            }

            juce::PropertiesFile& getProps() {return *properties.getUserSettings();}

            void saveIfNeeded() {properties.saveIfNeeded();}
        
        private:
            juce::ApplicationProperties properties;
    };
}