cmake_minimum_required(VERSION 3.20)
project(CSM_thesis)

set(CMAKE_CXX_STANDARD 20)              # volt: 17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # IntelliSense compile_commands.json

# 1. Find all .cpp files under src/
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

# 2. Look for packages (using the correct Case-Sensitive names for vcpkg)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(GLEW REQUIRED) # Use uppercase GLEW
find_package(glm REQUIRED)
find_package(imgui CONFIG REQUIRED) # ImGui often needs the CONFIG flag
find_package(tinyobjloader CONFIG REQUIRED)

# 3. Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    interface=struct                                        # Define the interface keyword as struct
    PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/"             # Define the project root directory path for the source code
    GLM_ENABLE_EXPERIMENTAL                                 # Enable experimental GLM extensions globally
    $<$<CONFIG:Debug>:DEBUG>                                # Define DEBUG macro when CMake is in Debug mode
    SSBO_PADDING=5                                          # SSBO padding for cone step mapping debug
    IMGUI_HAS_DOCK                                          # Enable ImGui docking panels
)

# 4. Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Headers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils
)

# 5. Link libraries (using the specific vcpkg targets)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3::SDL3 
    SDL3_image::SDL3_image
    GLEW::GLEW
    glm::glm
    imgui::imgui
    tinyobjloader::tinyobjloader
)