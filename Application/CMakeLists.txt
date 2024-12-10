add_executable(SingularityApp)

file(GLOB_RECURSE APP_SOURCES "src/*.cpp")
file(GLOB_RECURSE APP_HEADERS "include/*.h" "include/*.hpp")

target_sources(SingularityApp
    PRIVATE ${APP_SOURCES}
    PUBLIC ${APP_HEADERS}
)

target_include_directories(SingularityApp
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(SingularityApp PRIVATE SingularityEngine)
add_dependencies(SingularityApp CopyEngineResources SingularityEngine SDL2::SDL2)

# Organize the application target into a folder
set_target_properties(SingularityApp PROPERTIES FOLDER "Applications")
