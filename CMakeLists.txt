cmake_minimum_required(VERSION 3.5)
project(MyProject VERSION 0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE RELEASE)



# 查找 DirectX 12 和 DXGI 库
find_library(D3D12_LIBRARY d3d12 HINTS "$ENV{WindowsSdkDir}/Lib/$ENV{WindowsSDKVersion}/um/x64" "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64")
find_library(DXGI_LIBRARY dxgi HINTS "$ENV{WindowsSdkDir}/Lib/$ENV{WindowsSDKVersion}/um/x64" "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64")

# Set the desired runtime library for all targets in this project
# This selects MultiThreadedDLL (/MD) for Release and
# MultiThreadedDebugDLL (/MDd) for Debug builds.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MSVC runtime library selection")

add_compile_definitions(
    _HAS_EXCEPTIONS=0
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_ALL_CXX16_DEPRECATION_WARNINGS
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
    src/ConfigManager.cpp
    src/GameHook.cpp
    src/GraphicsHookManager.cpp
    src/ModManager.cpp
    src/RenderHookDX12.cpp
    src/RenderHookVulkan.cpp
    src/UIManager.cpp
    src/Logger.cpp
    third-party/imgui/imgui.cpp
    third-party/imgui/imgui_draw.cpp
    third-party/imgui/imgui_widgets.cpp
    third-party/imgui/imgui_tables.cpp
    third-party/imgui/imgui_demo.cpp
    third-party/imgui/backends/imgui_impl_vulkan.cpp
    third-party/imgui/backends/imgui_impl_dx12.cpp
    third-party/imgui/backends/imgui_impl_win32.cpp
)

target_link_libraries(CustomSensitivity ${DXGI_LIBRARY} ${D3D12_LIBRARY} minhook Vulkan::Vulkan)

set_target_properties(CustomSensitivity PROPERTIES
                      OUTPUT_NAME "CustomSensitivity"
                      SUFFIX ".asi")