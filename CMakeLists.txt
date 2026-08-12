cmake_minimum_required(VERSION 3.22)

project(HarshNoise VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HARSHNOISE_BUILD_PLUGIN "Build the JUCE plug-in targets" ON)
option(HARSHNOISE_BUILD_TESTS "Build plug-in smoke tests" ON)
set(HARSHNOISE_ARTIFACT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/artifacts" CACHE PATH
    "Directory for final VST3, AU, and Standalone products")
set(EHL_JUCE_SOURCE_DIR "" CACHE PATH "Existing JUCE source checkout; avoids network when set")

if(HARSHNOISE_BUILD_PLUGIN)
    include(FetchContent)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    if(EHL_JUCE_SOURCE_DIR)
        if(NOT EXISTS "${EHL_JUCE_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "EHL_JUCE_SOURCE_DIR does not contain JUCE: ${EHL_JUCE_SOURCE_DIR}")
        endif()
        add_subdirectory("${EHL_JUCE_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/juce")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
        add_subdirectory(JUCE)
    else()
        FetchContent_Declare(
            JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 8.0.15
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()
    add_subdirectory(modules/juce-ehl-design-module)

    set(HARSHNOISE_FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND HARSHNOISE_FORMATS AU)
    endif()

    juce_add_plugin(HarshNoise
        VERSION ${PROJECT_VERSION}
        COMPANY_NAME "EsionHsrahLatigid"
        BUNDLE_ID "jp.ehl.harshnoise"
        PRODUCT_NAME "HarshNoise"
        DESCRIPTION "Brutal digital harsh noise generator"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE
        PLUGIN_MANUFACTURER_CODE EHL_
        PLUGIN_CODE Hrsh
        FORMATS ${HARSHNOISE_FORMATS}
        VST3_CATEGORIES "Fx" "Distortion"
        AU_MAIN_TYPE "kAudioUnitType_Effect")

    juce_generate_juce_header(HarshNoise)

    target_sources(HarshNoise
        PRIVATE
            src/PluginEditor.cpp
            src/PluginEditor.h
            src/PluginProcessor.cpp
            src/PluginProcessor.h)

    target_compile_definitions(HarshNoise
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(HarshNoise
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            EHL::JuceDesign
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    set(harshnoise_juce_products "${CMAKE_CURRENT_BINARY_DIR}/HarshNoise_artefacts/$<CONFIG>")
    set(harshnoise_staged_products "${HARSHNOISE_ARTIFACT_DIR}/$<CONFIG>")
    set(harshnoise_artifact_dependencies HarshNoise_VST3 HarshNoise_Standalone)
    set(harshnoise_artifact_commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "${harshnoise_staged_products}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${harshnoise_staged_products}/VST3"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${harshnoise_staged_products}/Standalone"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${harshnoise_juce_products}/VST3" "${harshnoise_staged_products}/VST3"
        COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NormalizeVst3ModuleInfo.py"
                "${harshnoise_staged_products}/VST3/HarshNoise.vst3/Contents/Resources/moduleinfo.json"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${harshnoise_juce_products}/Standalone" "${harshnoise_staged_products}/Standalone")

    if(TARGET HarshNoise_AU)
        list(APPEND harshnoise_artifact_dependencies HarshNoise_AU)
        list(APPEND harshnoise_artifact_commands
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${harshnoise_staged_products}/AU"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${harshnoise_juce_products}/AU" "${harshnoise_staged_products}/AU")
    endif()

    if(APPLE)
        list(APPEND harshnoise_artifact_commands
            COMMAND codesign --force --deep --sign -
                    "${harshnoise_staged_products}/VST3/HarshNoise.vst3"
            COMMAND codesign --force --deep --sign -
                    "${harshnoise_staged_products}/Standalone/HarshNoise.app")
        if(TARGET HarshNoise_AU)
            list(APPEND harshnoise_artifact_commands
                COMMAND codesign --force --deep --sign -
                        "${harshnoise_staged_products}/AU/HarshNoise.component")
        endif()
    endif()

    add_custom_target(HarshNoise_Artifacts
        ${harshnoise_artifact_commands}
        DEPENDS ${harshnoise_artifact_dependencies}
        COMMENT "Staging HarshNoise products in ${HARSHNOISE_ARTIFACT_DIR}/$<CONFIG>")
    set(ehl_stage_platform "linux-x64")
    if(APPLE)
        set(ehl_stage_platform "macos-arm64")
    elseif(WIN32)
        set(ehl_stage_platform "windows-x64")
    endif()
    set(ehl_stage_source_products "${HARSHNOISE_ARTIFACT_DIR}/$<CONFIG>")
    set(ehl_staged_products "${HARSHNOISE_ARTIFACT_DIR}/plugin-release/${ehl_stage_platform}")

    set(ehl_stage_commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ehl_staged_products}/vst3"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ehl_staged_products}/standalone"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ehl_staged_products}/au")

    if(APPLE)
        list(APPEND ehl_stage_commands
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/vst3/harshnoise_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/standalone/harshnoise_standalone_plugin.app"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/au/harshnoise_au_plugin.component"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_stage_source_products}/VST3/HarshNoise.vst3" "${ehl_staged_products}/vst3/harshnoise_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_stage_source_products}/Standalone/HarshNoise.app" "${ehl_staged_products}/standalone/harshnoise_standalone_plugin.app"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_stage_source_products}/AU/HarshNoise.component" "${ehl_staged_products}/au/harshnoise_au_plugin.component")
    elseif(WIN32)
        list(APPEND ehl_stage_commands
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/vst3/harshnoise_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E rm -f "${ehl_staged_products}/standalone/harshnoise_standalone_plugin.exe"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_stage_source_products}/VST3/HarshNoise.vst3" "${ehl_staged_products}/vst3/harshnoise_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy "${ehl_stage_source_products}/Standalone/HarshNoise.exe" "${ehl_staged_products}/standalone/harshnoise_standalone_plugin.exe")
    else()
        list(APPEND ehl_stage_commands
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/vst3/harshnoise_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E rm -f "${ehl_staged_products}/standalone/harshnoise_standalone_plugin"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_stage_source_products}/VST3/HarshNoise.vst3" "${ehl_staged_products}/vst3/harshnoise_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy "${ehl_stage_source_products}/Standalone/HarshNoise" "${ehl_staged_products}/standalone/harshnoise_standalone_plugin")
    endif()

    list(APPEND ehl_stage_commands
        COMMAND ${CMAKE_COMMAND}
            -DPRODUCT_NAME=HarshNoise
            -DSLUG=harshnoise
            -DBUNDLE_ID=jp.ehl.harshnoise
            -DPLUGIN_CODE=Hrsh
            -DSTAGE_DIR=${ehl_staged_products}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteArtifactsManifest.cmake")

    add_custom_target(ehl_stage_products
        ${ehl_stage_commands}
        DEPENDS HarshNoise_Artifacts
        COMMENT "Staging HarshNoise CI release products under ${HARSHNOISE_ARTIFACT_DIR}/plugin-release")


    if(HARSHNOISE_BUILD_TESTS)
        enable_testing()
        add_executable(HarshNoiseSmokeTests Tests/SmokeTests.cpp)
        target_include_directories(HarshNoiseSmokeTests
            PRIVATE
                src
                "${CMAKE_CURRENT_BINARY_DIR}/HarshNoise_artefacts/JuceLibraryCode")
        target_link_libraries(HarshNoiseSmokeTests PRIVATE HarshNoise juce::juce_audio_utils)
        add_dependencies(HarshNoiseSmokeTests HarshNoise_Artifacts)
        add_test(NAME HarshNoiseSmokeTests COMMAND HarshNoiseSmokeTests)
        add_executable(HarshNoiseEditorTests Tests/EditorTests.cpp)
        target_include_directories(HarshNoiseEditorTests
            PRIVATE
                src
                "${CMAKE_CURRENT_BINARY_DIR}/HarshNoise_artefacts/JuceLibraryCode")
        target_link_libraries(HarshNoiseEditorTests PRIVATE HarshNoise EHL::JuceDesign juce::juce_audio_utils)
        add_test(NAME HarshNoiseEditorTests COMMAND HarshNoiseEditorTests)
        add_test(
            NAME HarshNoiseArtifactChecks
            COMMAND ${CMAKE_COMMAND}
                    -DBUILD_CONFIG=$<CONFIG>
                    -DHARSHNOISE_STAGED_DIR=${HARSHNOISE_ARTIFACT_DIR}/$<CONFIG>
                    -DHARSHNOISE_EXPECT_AU=$<BOOL:${APPLE}>
                    -DPython3_EXECUTABLE=${Python3_EXECUTABLE}
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/Tests/VerifyArtifacts.cmake)
    endif()
endif()
