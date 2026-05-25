cmake_minimum_required(VERSION 3.22)

project(lunch_box VERSION 0.1.0)

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
# Enable with: cmake .. -DLUNCH_BOX_MELATONIN_INSPECTOR=ON -DMELATONIN_INSPECTOR_DIR=/path/to/melatonin_inspector
option(LUNCH_BOX_MELATONIN_INSPECTOR "Enable melatonin_inspector dev tool" OFF)
if(LUNCH_BOX_MELATONIN_INSPECTOR)
    add_compile_definitions(LUNCH_BOX_MELATONIN_INSPECTOR=1)
endif()
if(LUNCH_BOX_MELATONIN_INSPECTOR)
    set(MELATONIN_INSPECTOR_DIR "" CACHE PATH "Path to melatonin_inspector")
    if(NOT MELATONIN_INSPECTOR_DIR)
        message(FATAL_ERROR "MELATONIN_INSPECTOR_DIR not set. Pass -DMELATONIN_INSPECTOR_DIR=/path/to/melatonin_inspector")
    endif()
    add_subdirectory(${MELATONIN_INSPECTOR_DIR} ${CMAKE_BINARY_DIR}/melatonin_inspector)
endif()

juce_add_binary_data(lunch_box_assets SOURCES
    fonts/Fredoka.ttf
    fonts/Poppins-Regular.ttf
    fonts/Poppins-Medium.ttf
    fonts/Poppins-SemiBold.ttf
    fonts/Poppins-Bold.ttf)

# Create the GUI application
juce_add_gui_app(lunch_box
    PRODUCT_NAME "Lunch Box"
    COMPANY_NAME "Matt from Atlanta"
    BUNDLE_ID "com.mattfromatlanta.lunchbox")

# Add source files
target_sources(lunch_box
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
        Source/Processing/LunchBoxNamer.h
        Source/Processing/LunchBoxNamer.cpp
        Source/Processing/LunchBoxProcessor.h
        Source/Processing/LunchBoxProcessor.cpp
        Source/Processing/BankFolderParser.h
        Source/Processing/BankFolderParser.cpp)

# Link required JUCE modules
target_link_libraries(lunch_box
    PRIVATE
        juce::juce_core
        juce::juce_audio_formats
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_gui_extra
        $<$<BOOL:${LUNCH_BOX_MELATONIN_INSPECTOR}>:melatonin_inspector>
        lunch_box_assets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Set compiler definitions
target_compile_definitions(lunch_box
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CAMERA=0
        JUCE_USE_FLAC=1
        JUCE_USE_MP3AUDIOFORMAT=1)

# M11 unit test suite (JUCE UnitTest framework, no extra dependencies)
juce_add_console_app(lunch_box_tests
    PRODUCT_NAME "Lunch Box Tests")

target_sources(lunch_box_tests
    PRIVATE
        tests/TestMain.cpp
        Source/Logger.h
        Source/Logger.cpp
        Source/FileSystemHelper.h
        Source/FileSystemHelper.cpp
        Source/Processing/LunchBoxNamer.h
        Source/Processing/LunchBoxNamer.cpp
        Source/Processing/BankFolderParser.h
        Source/Processing/BankFolderParser.cpp)

target_link_libraries(lunch_box_tests
    PRIVATE
        juce::juce_core
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_compile_definitions(lunch_box_tests
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CAMERA=0
        JUCE_USE_FLAC=1
        JUCE_USE_MP3AUDIOFORMAT=1)

# Legacy LunchBoxNamer test executable
juce_add_console_app(test_lunch_box_namer
    PRODUCT_NAME "Test Lunch Box Namer")

target_sources(test_lunch_box_namer
    PRIVATE
        Source/TestLunchBoxNamer.cpp
        Source/Processing/LunchBoxNamer.h
        Source/Processing/LunchBoxNamer.cpp
        Source/Logger.h
        Source/Logger.cpp)

target_link_libraries(test_lunch_box_namer
    PRIVATE
        juce::juce_core
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_compile_definitions(test_lunch_box_namer
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CAMERA=0)
