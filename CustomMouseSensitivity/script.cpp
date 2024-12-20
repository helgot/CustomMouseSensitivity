#include "script.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>

using namespace std;

bool PatchAddress(void* target_address, const void* patch_bytes, size_t patch_size)
{
    //DWORD oldProtect;
    //// Change memory protections to allow writing
    //if (!VirtualProtect(target_address, patch_size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    //    std::cerr << "Failed to change memory protection!" << std::endl;
    //    return false;
    //}

    // Write the patch
    memcpy(target_address, patch_bytes, patch_size);

    //// Restore original memory protections
    //if (!VirtualProtect(target_address, patch_size, oldProtect, &oldProtect)) {
    //    std::cerr << "Failed to restore memory protection!" << std::endl;
    //    return false;
    //}

    //// Flush the CPU instruction cache to ensure changes are visible
    //FlushInstructionCache(GetCurrentProcess(), target_address, patch_size);

    return true;
}

// Function to find the base address of RDR2.exe
uintptr_t GetModuleBaseAddress(const char* moduleName) {
    // Get a snapshot of all modules in the current process
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to take snapshot of modules!" << std::endl;
        return 0;
    }

    MODULEENTRY32 moduleEntry;
    moduleEntry.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnapshot, &moduleEntry)) {
        do {
            // Compare module name (case insensitive)
            if (_stricmp(moduleEntry.szModule, moduleName) == 0) {
                // Found the target module, return its base address
                CloseHandle(hSnapshot);
                return reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
            }
        } while (Module32Next(hSnapshot, &moduleEntry));
    }

    CloseHandle(hSnapshot);
    std::cerr << "Module not found!" << std::endl;
    return 0;
}

std::string GetCurrentDirectoryFilePath(const std::string& fileName) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string directory = std::string(path).substr(0, std::string(path).find_last_of("\\/"));
    return directory + "\\" + fileName;
}

float GetPrivateProfileFloat(const std::string& section, const std::string& key, float defaultValue, const std::string& fileName) {
    char buffer[256];
    // Retrieve the value as a string
    GetPrivateProfileStringA(section.c_str(), key.c_str(), "", buffer, sizeof(buffer), fileName.c_str());

    // Convert the string to a float
    try {
        return std::stof(buffer);
    }
    catch (const std::invalid_argument& e) {
        // If conversion fails, return the default value
        std::cerr << "Error: Invalid float value for key '" << key << "' in section '" << section << "'.\n";
        return defaultValue;
    }
    catch (const std::out_of_range& e) {
        // If the value is out of range, return the default value
        std::cerr << "Error: Float value out of range for key '" << key << "' in section '" << section << "'.\n";
        return defaultValue;
    }
}

void main()
{
    const std::string config_filename = "CustomMouseSensitivity.ini";
    const float first_person_quake_sense = GetPrivateProfileFloat("Settings", "FirstPersonMouseSensitivity", 1.0f, GetCurrentDirectoryFilePath(config_filename));
    const float third_person_quake_sense = GetPrivateProfileFloat("Settings", "ThirdPersonMouseSensitivity", 1.0f, GetCurrentDirectoryFilePath(config_filename));
    
    int screen_x, screen_y;
    int line_height = 10;
    GRAPHICS::GET_SCREEN_RESOLUTION(&screen_x, &screen_y);
    float fov = 0.0f;
    float frame_time = 0.0f;
    constexpr float target_frame_time = 0.0166667f;
    char buffer[64];
    bool is_first_person = false;
    bool is_in_scope = false;
    bool is_free_aiming = false;
    float zoom_factor = 0.0f;
    float stamina = 0.0f;
    int core_value = 0;

    const float base_first_person_fov = 55.0f;

    // Get base adddress of module:
    uintptr_t base_address = GetModuleBaseAddress("RDR2.exe");

    // Target addresses offsets:
    constexpr uintptr_t first_person_scale_offset = 0x39806FC; 
    constexpr uintptr_t first_person_mouse_look_offset = 0x3973114; 
    constexpr uintptr_t first_person_mouse_aim_offset = 0x3973118; 
    constexpr uintptr_t third_person_scale_offset = 0x39806BC; 
    constexpr uintptr_t third_person_mouse_look_offset = 0x3973128; 
    constexpr uintptr_t third_person_mouse_aim_offset = 0x397312C; 

    // Conversion constants:
    constexpr float core_empty_scale = 0.9881422924f;
    const float first_person_scale_value = 0.00914375018f * first_person_quake_sense;
    const float third_person_scale_value = 0.01117500011f * third_person_quake_sense;
 
    // Before main loop, patch slider values to all be 0.
    {
        int patch_value = 0;
        PatchAddress(reinterpret_cast<void*>(base_address + first_person_mouse_look_offset), reinterpret_cast<const void*>(&patch_value), sizeof(patch_value));
        PatchAddress(reinterpret_cast<void*>(base_address + first_person_mouse_aim_offset), reinterpret_cast<const void*>(&patch_value), sizeof(patch_value));
        PatchAddress(reinterpret_cast<void*>(base_address + third_person_mouse_look_offset), reinterpret_cast<const void*>(&patch_value), sizeof(patch_value));
        PatchAddress(reinterpret_cast<void*>(base_address + third_person_mouse_aim_offset), reinterpret_cast<const void*>(&patch_value), sizeof(patch_value));
    }

	while (true)
	{		
		// DEBUG Stuff:
		//frame_time = GAMEPLAY::GET_FRAME_TIME();
		//fov = CAM::GET_GAMEPLAY_CAM_FOV();
        //zoom_factor = CAM::_GET_GAMEPLAY_CAM_ZOOM();
        is_first_person = CAM::_0xD1BA66940E94C547();
        //stamina = PLAYER::_0x0FF421E467373FCF(PLAYER::PLAYER_ID());
        //is_in_scope = PLAYER::_0x04D7F33640662FA2(PLAYER::PLAYER_ID());
        core_value = ATTRIBUTE::_0x36731AC041289BB1(PLAYER::PLAYER_PED_ID(), 1);
        is_free_aiming = PLAYER::IS_PLAYER_FREE_AIMING(PLAYER::PLAYER_ID());

        // LOGGING:
        /*
		UI::SET_TEXT_SCALE(0.5f, 0.5f);
		snprintf(buffer, sizeof(buffer), "frame_time: %.5f ms", frame_time * 1000.0f);
		UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", buffer), 0.01f, 0.01f);
		snprintf(buffer, sizeof(buffer), "width: %d, height: %d", screen_x, screen_y);
		UI::SET_TEXT_SCALE(0.5f, 0.5f);
		UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", buffer), 0.01f, 0.01f + 1 * 25.0f / screen_y);
		UI::SET_TEXT_SCALE(0.5f, 0.5f);
		snprintf(buffer, sizeof(buffer), "fov: %.4f", fov);
		UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", buffer), 0.01f, 0.01f + 2 * 25.0f / screen_y);
		UI::SET_TEXT_SCALE(0.5f, 0.5f);
		UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", is_first_person? "First person: True" : "First person: False"), 0.01f, 0.01f + 3 * 25.0f / screen_y);
        UI::SET_TEXT_SCALE(0.5f, 0.5f);
        snprintf(buffer, sizeof(buffer), "zoom factor: %.4f", zoom_factor);
        UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", buffer), 0.01f, 0.01f + 4 * 25.0f / screen_y);
        UI::SET_TEXT_SCALE(0.5f, 0.5f);
        UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", is_in_scope ? "scoped: True" : "scoped: False"), 0.01f, 0.01f + 5 * 25.0f / screen_y);
        UI::SET_TEXT_SCALE(0.5f, 0.5f);
        UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", is_free_aiming ? "is_free_aiming: True" : "is_free_aiming: False"), 0.01f, 0.01f + 6 * 25.0f / screen_y);
        UI::SET_TEXT_SCALE(0.5f, 0.5f);
        snprintf(buffer, sizeof(buffer), "stamina: %.4f", stamina);
        UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", buffer), 0.01f, 0.01f + 7 * 25.0f / screen_y);
        UI::SET_TEXT_SCALE(0.5f, 0.5f);
        snprintf(buffer, sizeof(buffer), "core: %d", core_value);
        UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", buffer), 0.01f, 0.01f + 8 * 25.0f / screen_y);
        */
        
        // Apply patches:
        if (is_first_person) // First-person:
        {
            //WIP:
            //float patch_value = first_person_scale_value;
            //patch_value *= base_first_person_fov / fov;
            //if (core_value == 0)
            //{
            //    patch_value *= core_empty_scale;
            //}
            ////patch_value *= frame_time / target_frame_time;
            //PatchAddress(reinterpret_cast<void*>(base_address + first_person_scale_offset), reinterpret_cast<const void*>(&patch_value), sizeof(float));
        }
        else // Third-person:
        {
            float patch_value = third_person_scale_value;
            if (is_free_aiming && core_value == 0)
            {
                patch_value *= core_empty_scale;
            }
            PatchAddress(reinterpret_cast<void*>(base_address + third_person_scale_offset), reinterpret_cast<const void*>(&patch_value), sizeof(float));
        }

		WAIT(0);
	}
}

void ScriptMain()
{
	srand(GetTickCount64());
	main();
}