if(NOT DEFINED HARSHNOISE_STAGED_DIR)
    message(FATAL_ERROR "HARSHNOISE_STAGED_DIR is required")
endif()

set(vst3 "${HARSHNOISE_STAGED_DIR}/VST3/HarshNoise.vst3")
if(HARSHNOISE_EXPECT_AU)
    set(standalone "${HARSHNOISE_STAGED_DIR}/Standalone/HarshNoise.app")
elseif(WIN32)
    set(standalone "${HARSHNOISE_STAGED_DIR}/Standalone/HarshNoise.exe")
else()
    set(standalone "${HARSHNOISE_STAGED_DIR}/Standalone/HarshNoise")
endif()

if(NOT EXISTS "${vst3}")
    message(FATAL_ERROR "Missing staged VST3: ${vst3}")
endif()

if(NOT EXISTS "${standalone}")
    message(FATAL_ERROR "Missing staged Standalone app: ${standalone}")
endif()

set(module_info "${vst3}/Contents/Resources/moduleinfo.json")
if(NOT EXISTS "${module_info}")
    message(FATAL_ERROR "Missing staged VST3 moduleinfo.json: ${module_info}")
endif()

if(NOT DEFINED Python3_EXECUTABLE)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
endif()

execute_process(
    COMMAND ${Python3_EXECUTABLE} -m json.tool "${module_info}"
    RESULT_VARIABLE json_result
    OUTPUT_QUIET
    ERROR_VARIABLE json_error)
if(NOT json_result EQUAL 0)
    message(FATAL_ERROR "Invalid strict JSON in ${module_info}: ${json_error}")
endif()

if(HARSHNOISE_EXPECT_AU)
    set(au "${HARSHNOISE_STAGED_DIR}/AU/HarshNoise.component")

    if(NOT EXISTS "${au}")
        message(FATAL_ERROR "Missing staged AU: ${au}")
    endif()

    foreach(bundle IN ITEMS "${vst3}" "${standalone}" "${au}")
        execute_process(
            COMMAND codesign --verify --deep --strict "${bundle}"
            RESULT_VARIABLE codesign_result
            ERROR_VARIABLE codesign_error)
        if(NOT codesign_result EQUAL 0)
            message(FATAL_ERROR "Invalid signature for ${bundle}: ${codesign_error}")
        endif()
    endforeach()
endif()
