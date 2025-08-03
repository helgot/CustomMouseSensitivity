#pragma once
#include <cstdlib>
#include <d3d12sdklayers.h>
#include <unordered_map>
#include <string_view>

#include <Windows.h>
#include <winnt.h>

namespace CustomSensitivity
{
    struct DebugData 
    {
        uint64_t baseAddress = 0;
        uint64_t thirdPersonHookAddress = 0;
        uint64_t firstPersonHookAddress = 0;
        uint64_t fov_address = 0;
        bool show_menu = true;

        std::unordered_map<std::string_view, int> int_probe;
        std::unordered_map<std::string_view, float> float_probe;
        std::unordered_map<std::string_view, DWORD> dword_probe;
        std::unordered_map<std::string_view, BYTE> byte_probe;
        std::unordered_map<std::string_view, uint64_t> qword_probe;
    };
}