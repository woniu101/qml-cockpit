set(COCKPIT_NATIVE_SDK "${CMAKE_CURRENT_SOURCE_DIR}/.deps/assistant-native" CACHE PATH "Pinned native inference SDK")
find_path(COCKPIT_SHERPA_INCLUDE sherpa-onnx/c-api/c-api.h PATHS "${COCKPIT_NATIVE_SDK}/include" REQUIRED NO_DEFAULT_PATH)
find_library(COCKPIT_SHERPA_LIB sherpa-onnx-c-api PATHS "${COCKPIT_NATIVE_SDK}/lib" REQUIRED NO_DEFAULT_PATH)
find_library(COCKPIT_ORT_LIB onnxruntime PATHS "${COCKPIT_NATIVE_SDK}/lib" REQUIRED NO_DEFAULT_PATH)
add_library(CockpitInference STATIC
    src/assistant/AssistantInference.cpp src/assistant/AssistantInference.h
    src/assistant/inference/InferenceEngine.h
    src/assistant/inference/NativeAudio.cpp src/assistant/inference/NativeAudio.h
    src/assistant/inference/NativeAsr.cpp src/assistant/inference/NativeWake.cpp
    src/assistant/inference/NativeVoice.cpp src/assistant/inference/NativeNlu.cpp
    src/assistant/inference/VoiceText.cpp src/assistant/inference/VoiceText.h
    src/assistant/inference/BertTokenizer.cpp src/assistant/inference/BertTokenizer.h
    src/assistant/inference/BioDecoder.cpp src/assistant/inference/BioDecoder.h
    src/assistant/inference/NluSlots.cpp src/assistant/inference/NluSlots.h)
target_compile_options(CockpitInference PRIVATE /utf-8)
target_include_directories(CockpitInference PUBLIC src PRIVATE "${COCKPIT_SHERPA_INCLUDE}")
target_link_libraries(CockpitInference PUBLIC Qt6::Core PRIVATE "${COCKPIT_SHERPA_LIB}" "${COCKPIT_ORT_LIB}")
function(cockpit_attach_inference target)
    target_link_libraries(${target} PRIVATE CockpitInference)
    foreach(name sherpa-onnx-c-api.dll onnxruntime.dll onnxruntime_providers_shared.dll)
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${COCKPIT_NATIVE_SDK}/lib/${name}" "$<TARGET_FILE_DIR:${target}>/${name}")
    endforeach()
endfunction()
