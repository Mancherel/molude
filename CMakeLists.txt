cmake_minimum_required(VERSION 3.25)
project(Molude VERSION 0.1.0)

add_subdirectory(JUCE)

if(APPLE)
    set(MOLUDE_FORMATS AU VST3 Standalone)
else()
    set(MOLUDE_FORMATS VST3 Standalone)
endif()

juce_add_plugin(Molude
    COMPANY_NAME "MagnusBlix"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Mblx
    PLUGIN_CODE Mold
    FORMATS ${MOLUDE_FORMATS}
    PRODUCT_NAME "Molude"
    BUNDLE_ID "com.magnusblix.molude"
)

target_sources(Molude PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SynthEngine.cpp
    Source/MoludeLookAndFeel.cpp
)

target_compile_definitions(Molude PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(Molude
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
