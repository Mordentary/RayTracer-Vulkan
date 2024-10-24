# Shader compilation configuration
find_program(GLSL_VALIDATOR glslangValidator HINTS 
    /usr/bin 
    /usr/local/bin 
    $ENV{VULKAN_SDK}/Bin/ 
    $ENV{VULKAN_SDK}/Bin32/
    REQUIRED
)

message(STATUS "Found glslangValidator: ${GLSL_VALIDATOR}")

# Shader compilation target
add_custom_target(Shaders ALL
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
        python "${CMAKE_CURRENT_SOURCE_DIR}/compile_shaders.py"
        "${SHADER_SOURCE_DIR}"
        "${SHADER_OUTPUT_DIR}"
        "${GLSL_VALIDATOR}"
    COMMENT "Compiling shaders from ${SHADER_SOURCE_DIR} to ${SHADER_OUTPUT_DIR}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)