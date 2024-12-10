add_library(SingularityEngine SHARED)

file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp" "src/*.hpp")
file(GLOB_RECURSE INTERNAL_HEADERS "src/internal/*.h" "src/internal/*.hpp")
list(REMOVE_ITEM ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.cpp")

target_sources(SingularityEngine
    PRIVATE ${ENGINE_SOURCES} ${INTERNAL_HEADERS}
    PUBLIC ${ENGINE_HEADERS}
)

target_include_directories(SingularityEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(SingularityEngine PRIVATE SINGULARITY_ENGINE_EXPORTS)
target_precompile_headers(SingularityEngine PRIVATE src/pch.h)

target_link_libraries(SingularityEngine
    PUBLIC
        Vulkan::Vulkan
        vkbootstrap
        stb_image
        glm
        vma
        Pal::Sigslot
        volk
        fastgltf
        fmt::fmt
        SDL2::SDL2
        SDL2::SDL2main
        imgui
        dxcompiler
)

add_custom_target(CopyEngineResources ALL)
add_custom_command(TARGET CopyEngineResources POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:SingularityEngine>" "$<TARGET_FILE_DIR:SingularityEngine>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:SingularityEngine> "$<TARGET_FILE_DIR:SingularityEngine>"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:SingularityEngine>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SHADER_OUTPUT_DIR}" "$<TARGET_FILE_DIR:SingularityEngine>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:SingularityEngine>/assets"
)

add_dependencies(SingularityEngine vma)
add_dependencies(CopyEngineResources SingularityEngine Shaders)

# Organize targets into folders for cleanliness
set_target_properties(SingularityEngine PROPERTIES FOLDER "Engine")
set_target_properties(CopyEngineResources PROPERTIES FOLDER "Utilities")
