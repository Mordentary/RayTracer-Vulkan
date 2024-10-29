cmake_minimum_required(VERSION 3.16)
project("SingularityEngine" LANGUAGES CXX)

# Global settings
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Platform and architecture detection
function(determine_platform PLATFORM_VAR)
    if(WIN32)
        if(WINDOWS_STORE)
            set(${PLATFORM_VAR} "UWP" PARENT_SCOPE)
        else()
            set(${PLATFORM_VAR} "Windows" PARENT_SCOPE)
        endif()
    elseif(APPLE)
        if(IOS)
            set(${PLATFORM_VAR} "iOS" PARENT_SCOPE)
        else()
            set(${PLATFORM_VAR} "macOS" PARENT_SCOPE)
        endif()
    elseif(ANDROID)
        set(${PLATFORM_VAR} "Android" PARENT_SCOPE)
    elseif(UNIX)
        set(${PLATFORM_VAR} "Linux" PARENT_SCOPE)
    else()
        set(${PLATFORM_VAR} "Unknown" PARENT_SCOPE)
    endif()
endfunction()

# Determine platform and architecture
determine_platform(PLATFORM_NAME)
set(ARCHITECTURE ${MSVC_CXX_ARCHITECTURE_ID})
if(NOT ARCHITECTURE)
    set(ARCHITECTURE ${CMAKE_GENERATOR_PLATFORM})
endif()


# Output directories setup
set(BASE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}/${ARCHITECTURE}")

# Build directory (use CMAKE_BINARY_DIR)
set(ENGINE_BUILD_DIR "${CMAKE_BINARY_DIR}")

# Configuration types handling
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(IS_MULTI_CONFIG)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
    foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${CONFIG}" CONFIG_UPPER)
        foreach(TYPE RUNTIME LIBRARY ARCHIVE)
            set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${BASE_OUTPUT_DIR}/${CONFIG}")
        endforeach()
    endforeach()
else()
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
    endif()
    foreach(TYPE RUNTIME LIBRARY ARCHIVE)
        # Use CMAKE_BUILD_TYPE directly
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY "${BASE_OUTPUT_DIR}/${CMAKE_BUILD_TYPE}")
    endforeach()
endif()

# Shader configuration
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders" CACHE PATH "Path to shader source files")
if(IS_MULTI_CONFIG)
    # Use ARCHITECTURE and CONFIG directly without duplicating
    set(SHADER_OUTPUT_DIR "${ENGINE_BUILD_DIR}/${ARCHITECTURE}-$<CONFIG>/compiled_shaders")
else()
    set(SHADER_OUTPUT_DIR "${ENGINE_BUILD_DIR}/compiled_shaders")
endif()



# Dependencies
find_package(Vulkan REQUIRED)

# Add subdirectories
add_subdirectory(external)
add_subdirectory(Engine)
add_subdirectory(shaders)
add_subdirectory(Application)

# Solution organization
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")
set_property(GLOBAL PROPERTY EXTERNAL_TARGETS_FOLDER "ThirdParty")
set_target_properties(imgui vkbootstrap stb_image vma glm 
    PROPERTIES FOLDER "ThirdParty"
)

# Startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
    PROPERTY VS_STARTUP_PROJECT SingularityApp
)

# Status messages
message(STATUS "Configuration Summary:")
message(STATUS "- Platform: ${PLATFORM_NAME}")
message(STATUS "- Architecture: ${ARCHITECTURE}")
message(STATUS "- Build directory: ${ENGINE_BUILD_DIR}")
message(STATUS "- Output directory: ${BASE_OUTPUT_DIR}")
message(STATUS "- Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "- Shader output directory: ${SHADER_OUTPUT_DIR}")