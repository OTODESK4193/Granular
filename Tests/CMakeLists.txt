juce_add_console_app(GranularTests
    PRODUCT_NAME "GranularTests"
)

juce_generate_juce_header(GranularTests)

target_sources(GranularTests PRIVATE
    GranularTests.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginEditor.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/GranularEngine.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/PitchAnalyzer.cpp
    ${CMAKE_SOURCE_DIR}/Source/DSP/Arpeggiator.cpp
    ${CMAKE_SOURCE_DIR}/Source/GUI/ArcDial.cpp
    ${CMAKE_SOURCE_DIR}/Source/GUI/DotWaveformDisplay.cpp
    ${CMAKE_SOURCE_DIR}/Source/GUI/MainPanel.cpp
    ${CMAKE_SOURCE_DIR}/Source/GUI/EnginePanel.cpp
    ${CMAKE_SOURCE_DIR}/Source/GUI/ArpPanel.cpp
    ${CMAKE_SOURCE_DIR}/Source/GUI/ConfigPanel.cpp
    ${CMAKE_SOURCE_DIR}/Source/GUI/FxPanel.cpp
    ${CMAKE_SOURCE_DIR}/Source/GUI/ModPanel.cpp
)

target_include_directories(GranularTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Source/DSP
    ${CMAKE_SOURCE_DIR}/Source/GUI
    ${CMAKE_SOURCE_DIR}/Source/third_party
)

target_compile_definitions(GranularTests PRIVATE
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    GRANULAR_VERSION="1.2.1"
    JucePlugin_Name="Granular"
)

target_link_libraries(GranularTests PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

if(MSVC)
    target_compile_options(GranularTests PRIVATE
        /arch:AVX2
        /fp:precise
        /utf-8
        /W3
        /wd4819
        /O2
        /bigobj
        /MP
    )
endif()
