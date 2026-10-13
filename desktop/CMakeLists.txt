cmake_minimum_required(VERSION 3.22)

project(SonoForgeStudio VERSION 0.8.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(SonoForge
    PRODUCT_NAME "SonoForge Studio"
    COMPANY_NAME "SonoForge"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon.png"
    VERSION ${PROJECT_VERSION}
)

target_sources(SonoForge
    PRIVATE
        Source/Main.cpp
        Source/AppServices.cpp
        Source/AppServices.h
        Source/MainComponent.cpp
        Source/MainComponent.h
        Source/WindowDragHandle.h
        Source/AudioEngine.cpp
        Source/AudioEngine.h
        Source/AudioTrack.cpp
        Source/AudioTrack.h
        Source/MidiInstrumentTrack.cpp
        Source/MidiInstrumentTrack.h
        Source/MidiTrackRowComponent.cpp
        Source/MidiTrackRowComponent.h
        Source/MixerPanelComponent.cpp
        Source/MixerPanelComponent.h
        Source/TrackRowComponent.cpp
        Source/TrackRowComponent.h
        Source/TimelineRulerComponent.cpp
        Source/TimelineRulerComponent.h
)

target_compile_definitions(SonoForge
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(SonoForge
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

include(CTest)
if(BUILD_TESTING)
    juce_add_console_app(SonoForgeTests PRODUCT_NAME "SonoForge Tests")
    get_target_property(sonoforge_sources SonoForge SOURCES)
    list(FILTER sonoforge_sources EXCLUDE REGEX "Source/Main\\.cpp$")
    target_sources(SonoForgeTests PRIVATE ${sonoforge_sources} Tests/MidiLifetimeTests.cpp)
    target_compile_definitions(SonoForgeTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(SonoForgeTests PRIVATE
        juce::juce_audio_utils juce::juce_audio_formats juce::juce_gui_extra
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME MidiLifetime COMMAND SonoForgeTests)
endif()
