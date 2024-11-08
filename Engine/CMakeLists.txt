add_library(SingularityEngine SHARED)

# Source files
file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp" "src/*.hpp")


file(GLOB_RECURSE INTERNAL_HEADERS "src/internal/*.h" "src/internal/*.hpp")

# Remove PCH from sources
list(REMOVE_ITEM ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.cpp")

# Configure target sources
target_sources(SingularityEngine
    PRIVATE
        ${ENGINE_SOURCES}
        ${INTERNAL_HEADERS}
    PUBLIC
        ${ENGINE_HEADERS}
)

# Include directories
target_include_directories(SingularityEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile definitions and dependencies
target_compile_definitions(SingularityEngine PRIVATE SINGULARITY_ENGINE_EXPORTS)
target_precompile_headers(SingularityEngine PRIVATE src/pch.h)

# Link dependencies
target_link_libraries(SingularityEngine
    PUBLIC
        Vulkan::Vulkan
        vkbootstrap
        stb_image
        glm
        vma
        Pal::Sigslot
        fastgltf
        fmt::fmt
        SDL2
        SDL2main
        imgui
)

# Resource copying
add_custom_target(CopyEngineResources ALL
    COMMENT "Copying engine resources"
)

# Copy commands
add_custom_command(TARGET CopyEngineResources POST_BUILD
    # Copy engine DLL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:SingularityEngine>"
        "$<TARGET_FILE_DIR:SingularityEngine>"
    # Copy dependency DLLs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:SingularityEngine>
        "$<TARGET_FILE_DIR:SingularityEngine>"
    # Create and copy shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        "$<TARGET_FILE_DIR:SingularityEngine>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SHADER_OUTPUT_DIR}"
        "$<TARGET_FILE_DIR:SingularityEngine>/shaders"
    # Copy assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:SingularityEngine>/assets"
    COMMENT "Copying all engine resources to output directory"
    COMMAND_EXPAND_LISTS
)

# Dependencies
add_dependencies(SingularityEngine vma)
add_dependencies(CopyEngineResources SingularityEngine Shaders)