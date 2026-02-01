cmake_minimum_required(VERSION 3.22)

project(chompi_pack VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE
add_subdirectory(/Users/matthewfishel/Repos/JUCE ${CMAKE_BINARY_DIR}/JUCE)

# Create the console application
juce_add_console_app(chompi_pack
    PRODUCT_NAME "Chompi Pack")

# Add source files
target_sources(chompi_pack
    PRIVATE
        Source/Main.cpp
        Source/Logger.h
        Source/Logger.cpp
        Source/AudioScanner.h
        Source/AudioScanner.cpp
        Source/FileSystemHelper.h
        Source/FileSystemHelper.cpp
        Source/AudioConverter.h
        Source/AudioConverter.cpp
        Source/ChompiNamer.h
        Source/ChompiNamer.cpp)

# Link required JUCE modules
target_link_libraries(chompi_pack
    PRIVATE
        juce::juce_core
        juce::juce_audio_formats
        juce::juce_audio_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Set compiler definitions
target_compile_definitions(chompi_pack
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CAMERA=0)

# Create test executable for ChompiNamer unit tests
juce_add_console_app(test_chompi_namer
    PRODUCT_NAME "Test Chompi Namer")

target_sources(test_chompi_namer
    PRIVATE
        Source/TestChompiNamer.cpp
        Source/ChompiNamer.h
        Source/ChompiNamer.cpp
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
