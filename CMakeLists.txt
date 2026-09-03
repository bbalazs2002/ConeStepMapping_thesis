cmake_minimum_required(VERSION 3.21)
project(CSM_thesis)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. Find all .cpp files under src/, excluding main.cpp
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

# 2. Find packages
find_package(SDL3          REQUIRED)
find_package(SDL3_image    REQUIRED)
find_package(GLEW          REQUIRED)
find_package(glm           REQUIRED)
find_package(imgui         CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

# 3. Static library from all sources except main.cpp
#    PUBLIC properties propagate automatically to tests and the main executable.
add_library(CSM_lib STATIC ${SOURCES})

target_compile_definitions(CSM_lib PUBLIC
    interface=struct
    PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/"
    GLM_ENABLE_EXPERIMENTAL
    $<$<CONFIG:Debug>:DEBUG>
    IMGUI_HAS_DOCK
)

target_include_directories(CSM_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Headers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils
)

target_link_libraries(CSM_lib PUBLIC
    SDL3::SDL3
    SDL3_image::SDL3_image
    GLEW::GLEW
    glm::glm
    imgui::imgui
    tinyobjloader::tinyobjloader
)

# 4. Main executable — only needs main.cpp; everything else via CSM_lib
add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE CSM_lib)

# 5. Copy all runtime DLLs next to the executable after each build (Windows).
#    Ensures the correct vcpkg-built DLLs
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )
endif()

# 6. Optional test targets
option(BUILD_TESTS "Build unit and integration tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(tests)
endif()
