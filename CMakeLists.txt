cmake_minimum_required(VERSION 3.16)
project(MyProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE RELEASE)

add_compile_definitions(
    _HAS_EXCEPTIONS=1
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
)

find_package(Vulkan REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/third-party/minhook/include)
include_directories(${PROJECT_SOURCE_DIR}/third-party/nlohmann)
include_directories(${PROJECT_SOURCE_DIR}/third-party/imgui)
include_directories(${PROJECT_SOURCE_DIR}/third-party/imgui/backends)

add_subdirectory(third-party/minhook)

# Just one big src blob...
add_library(CustomSensitivity SHARED
    src/main.cpp
    src/logger.cpp
    src/mod.cpp
    src/renderhook_vulkan.cpp
    src/config.cpp
    src/ui.cpp
    third-party/imgui/imgui.cpp
    third-party/imgui/imgui_draw.cpp
    third-party/imgui/imgui_widgets.cpp
    third-party/imgui/imgui_tables.cpp
    third-party/imgui/backends/imgui_impl_vulkan.cpp
    third-party/imgui/backends/imgui_impl_win32.cpp
)

target_link_libraries(CustomSensitivity minhook Vulkan::Vulkan)

set_target_properties(CustomSensitivity PROPERTIES
                      OUTPUT_NAME "CustomSensitivity"
                      SUFFIX ".asi")