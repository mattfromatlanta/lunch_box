cmake_minimum_required(VERSION 3.22)

project(chompi_pack VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE
# Set JUCE_DIR via: cmake .. -DJUCE_DIR=/path/to/JUCE
# Or clone JUCE alongside this repo: git clone https://github.com/juce-framework/JUCE.git ../JUCE
set(JUCE_DIR "" CACHE PATH "Path to JUCE framework source")
if(NOT JUCE_DIR)
    if(EXISTS "${CMAKE_SOURCE_DIR}/../JUCE/CMakeLists.txt")
        set(JUCE_DIR "${CMAKE_SOURCE_DIR}/../JUCE")
    else()
        message(FATAL_ERROR
            "JUCE not found. Either:\n"
            "  1. Clone JUCE next to this repo: git clone https://github.com/juce-framework/JUCE.git ../JUCE\n"
            "  2. Pass the path: cmake .. -DJUCE_DIR=/path/to/JUCE")
    endif()
endif()
add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)

# Optional dev tool: melatonin_inspector (JUCE component inspector)
# Enable with: cmake .. -DCHOMPI_MELATONIN_INSPECTOR=ON -DMELATONIN_INSPECTOR_DIR=/path/to/melatonin_inspector
option(CHOMPI_MELATONIN_INSPECTOR "Enable melatonin_inspector dev tool" OFF)
if(CHOMPI_MELATONIN_INSPECTOR)
    add_compile_definitions(CHOMPI_MELATONIN_INSPECTOR=1)
endif()
if(CHOMPI_MELATONIN_INSPECTOR)
    set(MELATONIN_INSPECTOR_DIR "" CACHE PATH "Path to melatonin_inspector")
    if(NOT MELATONIN_INSPECTOR_DIR)
        message(FATAL_ERROR "MELATONIN_INSPECTOR_DIR not set. Pass -DMELATONIN_INSPECTOR_DIR=/path/to/melatonin_inspector")
    endif()
    add_subdirectory(${MELATONIN_INSPECTOR_DIR} ${CMAKE_BINARY_DIR}/melatonin_inspector)
endif()

juce_add_binary_data(chompi_assets SOURCES
    dev_assets/chompi_logo.png
    fonts/Poppins-Regular.ttf
    fonts/Poppins-Medium.ttf
    fonts/Poppins-SemiBold.ttf
    fonts/Poppins-Bold.ttf)

# Create the GUI application
juce_add_gui_app(chompi_pack
    PRODUCT_NAME "Chompi Pack"
    COMPANY_NAME "Matt from Atlanta"
    BUNDLE_ID "com.mattfromatlanta.chompipack")

# Add source files
target_sources(chompi_pack
    PRIVATE
        Source/Main.cpp
        Source/Logger.h
        Source/Logger.cpp
        Source/AudioConfiguration.h
        Source/CLI/CliProcessor.h
        Source/CLI/CliProcessor.cpp
        Source/GUI/MainWindow.h
        Source/GUI/MainWindow.cpp
        Source/GUI/AppMenuBar.h
        Source/GUI/AppMenuBar.cpp
        Source/GUI/MainComponent.h
        Source/GUI/MainComponent.cpp
        Source/GUI/GuiProcessor.h
        Source/GUI/GuiProcessor.cpp
        Source/GUI/FolderDropZone.h
        Source/GUI/FolderDropZone.cpp
        Source/GUI/BankSlotComponent.h
        Source/GUI/BankSlotComponent.cpp
        Source/GUI/BankRowComponent.h
        Source/GUI/BankRowComponent.cpp
        Source/GUI/BankEditorPanel.h
        Source/GUI/BankEditorPanel.cpp
        Source/GUI/FocusedSlotRow.h
        Source/GUI/FocusedSlotRow.cpp
        Source/GUI/BankFocusPanel.h
        Source/GUI/BankFocusPanel.cpp
        Source/GUI/AudioPreviewPlayer.h
        Source/GUI/AudioPreviewPlayer.cpp
        Source/GUI/WaveformDisplay.h
        Source/GUI/WaveformDisplay.cpp
        Source/GUI/PreviewPanel.h
        Source/GUI/PreviewPanel.cpp
        Source/GUI/ClipboardHelper.h
        Source/GUI/ClipboardHelper.mm
        Source/FileSystemHelper.h
        Source/FileSystemHelper.cpp
        Source/Processing/AudioConverter.h
        Source/Processing/AudioConverter.cpp
        Source/Processing/ChompiNamer.h
        Source/Processing/ChompiNamer.cpp
        Source/Processing/ChompiProcessor.h
        Source/Processing/ChompiProcessor.cpp
        Source/Processing/BankFolderParser.h
        Source/Processing/BankFolderParser.cpp)

# Link required JUCE modules
target_link_libraries(chompi_pack
    PRIVATE
        juce::juce_core
        juce::juce_audio_formats
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_gui_extra
        $<$<BOOL:${CHOMPI_MELATONIN_INSPECTOR}>:melatonin_inspector>
        chompi_assets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Set compiler definitions
target_compile_definitions(chompi_pack
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CAMERA=0
        JUCE_USE_FLAC=1
        JUCE_USE_MP3AUDIOFORMAT=1)

# M11 unit test suite (JUCE UnitTest framework, no extra dependencies)
juce_add_console_app(chompi_pack_tests
    PRODUCT_NAME "Chompi Pack Tests")

target_sources(chompi_pack_tests
    PRIVATE
        tests/TestMain.cpp
        Source/Logger.h
        Source/Logger.cpp
        Source/FileSystemHelper.h
        Source/FileSystemHelper.cpp
        Source/Processing/ChompiNamer.h
        Source/Processing/ChompiNamer.cpp
        Source/Processing/BankFolderParser.h
        Source/Processing/BankFolderParser.cpp)

target_link_libraries(chompi_pack_tests
    PRIVATE
        juce::juce_core
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_compile_definitions(chompi_pack_tests
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CAMERA=0
        JUCE_USE_FLAC=1
        JUCE_USE_MP3AUDIOFORMAT=1)

# Legacy ChompiNamer test executable
juce_add_console_app(test_chompi_namer
    PRODUCT_NAME "Test Chompi Namer")

target_sources(test_chompi_namer
    PRIVATE
        Source/TestChompiNamer.cpp
        Source/Processing/ChompiNamer.h
        Source/Processing/ChompiNamer.cpp
        Source/Logger.h
        Source/Logger.cpp)

target_link_libraries(test_chompi_namer
    PRIVATE
        juce::juce_core
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_compile_definitions(test_chompi_namer
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CAMERA=0)
