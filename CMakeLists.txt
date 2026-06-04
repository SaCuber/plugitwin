cmake_minimum_required(VERSION 3.22)

project(PlugitWin
    VERSION 0.1.1
    DESCRIPTION "System-tray VST host for Windows system audio"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(external/JUCE)

juce_add_gui_app(PlugitWin
    PRODUCT_NAME "PlugitWin"
    COMPANY_NAME "SantiagoRoldao"
    VERSION "0.2.0"
    # TODO: Icon
    # ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.png"
)

juce_generate_juce_header(PlugitWin)

target_sources(PlugitWin PRIVATE
    src/PluginScanWorker.h
    src/PluginScanWorker.cpp
    src/Main.cpp
    src/TrayApplication.h
    src/TrayApplication.cpp
    src/AudioEngine.h
    src/AudioEngine.cpp
    src/PluginChain.h
    src/PluginChain.cpp
    src/PluginHost.h
    src/PluginHost.cpp
    src/ui/Colours.h
    src/ui/StartupShortcut.h
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/PluginChainView.h
    src/ui/PluginChainView.cpp
    src/ui/PluginEditorWindow.h
    src/ui/PluginEditorWindow.cpp
)

target_compile_definitions(PlugitWin PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=0
    JUCE_ASIO=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(PlugitWin PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    # JUCE's recommended interface targets for warnings & flags:
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

target_include_directories(PlugitWin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)