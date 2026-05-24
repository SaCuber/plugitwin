# ==============================================================================
#  PlugitWin - top-level CMake build script
# ==============================================================================
#  This file tells CMake how to build PlugitWin. CMake reads it and generates
#  the actual build files for whatever build system you have installed on your
#  machine (Visual Studio solution, Ninja files, Makefiles, etc).
#
#  You do not run this file directly. You run `cmake` and point it at the
#  folder containing this file. See 05-build-and-run.md for the full workflow.
# ==============================================================================

cmake_minimum_required(VERSION 3.22)

# ------------------------------------------------------------------------------
#  Project declaration
# ------------------------------------------------------------------------------
#  This sets the project name and the languages we use. CXX = C++.
#  VERSION is informational; we can read it from C++ code if we want.
# ------------------------------------------------------------------------------
project(PlugitWin
    VERSION 0.1.0
    DESCRIPTION "System-tray VST host for Windows system audio"
    LANGUAGES CXX
)

# ------------------------------------------------------------------------------
#  C++ standard
# ------------------------------------------------------------------------------
#  JUCE 7+ requires C++17. We pin it explicitly so the build is reproducible
#  regardless of compiler defaults.
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------------------
#  Where to put built binaries
# ------------------------------------------------------------------------------
#  By default, CMake scatters .exe and .dll files around the build tree. We
#  collect them in one place so it's easy to find and run PlugitWin after
#  building.
# ------------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ------------------------------------------------------------------------------
#  JUCE
# ------------------------------------------------------------------------------
#  We bring JUCE in as a git submodule under `external/JUCE`. CMake's
#  `add_subdirectory` reads JUCE's own CMakeLists.txt, which defines all the
#  JUCE module targets (juce_core, juce_audio_processors, etc) that we link to
#  below.
#
#  Setup for this submodule is documented in 05-build-and-run.md.
# ------------------------------------------------------------------------------
add_subdirectory(external/JUCE)

# ------------------------------------------------------------------------------
#  The PlugitWin GUI application target
# ------------------------------------------------------------------------------
#  juce_add_gui_app is a helper from JUCE's CMake API. It creates an executable
#  target with all the platform glue (Windows resource files, the WinMain
#  entry point shim, app icon, etc) wired up correctly.
#
#  PRODUCT_NAME    - what shows in the .exe filename and in Task Manager
#  COMPANY_NAME    - used in version info; can be anything for now
#  VERSION         - string version, separate from project VERSION above
#  ICON_BIG        - we'll add an .ico file later; commented out for now
# ------------------------------------------------------------------------------
juce_add_gui_app(PlugitWin
    PRODUCT_NAME "PlugitWin"
    COMPANY_NAME "Santiago Roldão"
    VERSION "0.1.0"
    # ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.png"
)

# ------------------------------------------------------------------------------
#  JUCE generated header
# ------------------------------------------------------------------------------
#  JUCE auto-generates a `JuceHeader.h` that includes all the JUCE modules
#  you've linked. We could include each module header manually instead; using
#  the generated header is simpler for a small project.
# ------------------------------------------------------------------------------
juce_generate_juce_header(PlugitWin)

# ------------------------------------------------------------------------------
#  Source files
# ------------------------------------------------------------------------------
#  We list every .cpp and .h file in the project here. CMake doesn't auto-
#  discover files (and there are good reasons for that: explicit lists make
#  builds reproducible and force you to think about what's in your project).
#
#  As we add new files, we add them to this list.
# ------------------------------------------------------------------------------
target_sources(PlugitWin PRIVATE
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
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/PluginChainView.h
    src/ui/PluginChainView.cpp
    src/ui/PluginEditorWindow.h
    src/ui/PluginEditorWindow.cpp
)

# ------------------------------------------------------------------------------
#  Compile-time definitions
# ------------------------------------------------------------------------------
#  These #defines get baked into the compile. They configure JUCE features
#  without needing to edit JUCE's own files.
#
#  JUCE_PLUGINHOST_VST3=1      -> compile VST3 hosting support
#  JUCE_PLUGINHOST_VST=0       -> we do NOT enable VST2; the SDK is no longer
#                                 available from Steinberg and we don't want
#                                 the legal/licensing complication. Can revisit.
#  JUCE_WASAPI=1               -> enable Windows WASAPI audio backend
#  JUCE_DIRECTSOUND=0          -> we don't need legacy DirectSound
#  JUCE_ASIO=0                 -> not in MVP, per our spec
#  JUCE_USE_CURL=0             -> we don't make HTTP requests
#  JUCE_WEB_BROWSER=0          -> we don't embed a web browser
#  JUCE_DISPLAY_SPLASH_SCREEN  -> hide JUCE's free-license splash on launch
# ------------------------------------------------------------------------------
target_compile_definitions(PlugitWin PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=0
    JUCE_ASIO=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

# ------------------------------------------------------------------------------
#  Linked JUCE modules
# ------------------------------------------------------------------------------
#  Each `juce::juce_xxx` is a CMake target representing one JUCE module.
#  PRIVATE means: PlugitWin uses these but doesn't re-expose them as part of
#  its own public API. For an application (as opposed to a library), PRIVATE
#  is always correct.
#
#  Modules we need:
#    juce_audio_basics       - audio buffer types, MIDI buffers
#    juce_audio_devices      - WASAPI device enumeration + I/O
#    juce_audio_processors   - the plugin hosting machinery
#    juce_audio_utils        - small audio helpers
#    juce_core               - strings, files, threads, the foundation
#    juce_data_structures    - ValueTree, etc (we may use for future config)
#    juce_events             - the message loop, timers
#    juce_graphics           - low-level drawing
#    juce_gui_basics         - windows, components, the tray icon
#    juce_gui_extra          - extras like the document window
# ------------------------------------------------------------------------------
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

# ------------------------------------------------------------------------------
#  Include directories
# ------------------------------------------------------------------------------
#  Lets us write `#include "AudioEngine.h"` from any source file without
#  worrying about relative paths.
# ------------------------------------------------------------------------------
target_include_directories(PlugitWin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)