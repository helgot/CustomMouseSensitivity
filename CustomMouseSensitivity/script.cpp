#include "script.h"
#include "keyboard.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>

using namespace std;


// Target addresses offsets:
constexpr uintptr_t kFirstPersonScaleOffset = 0x39806FC;
constexpr uintptr_t kFirstPersonMouseLookOffset = 0x3973114;
constexpr uintptr_t kFirstPersonMouseAimOffset = 0x3973118;
constexpr uintptr_t kThirdPersonScaleOffset = 0x39806BC;
constexpr uintptr_t kThirdPersonMouseMouseLookOffset = 0x3973128;
constexpr uintptr_t kThirdPersonMouseAimOffset = 0x397312C;

constexpr float kCoreEmptyScale = 0.9881422924f;
constexpr float kInverseCoreEmptyScale = 1.0f / kCoreEmptyScale;
constexpr float FirstPersonAimScale = 0.00914375018f;
constexpr float ThirdPersonAimScale = 0.01117500011f;
constexpr float DefaultScaleValue = 0.01666667f;
//constexpr float ScopedSenseScale = 0.190525f;
constexpr float FirstPersonScopedSenseScale = 0.087486349120136f;
constexpr float ThirdPersonScopedSenseScale = 0.087486349120136f;
constexpr float FirstPersonScopedCoreEmptyScale = 1.823375f;


struct MouseSensitivitySettings {
    float first_person_mouse_look_sensitivity;
    float first_person_mouse_aim_sensitivity;
    float third_person_mouse_look_sensitivity;
    float third_person_mouse_aim_sensitivity;
    bool fov_independent_rotation;
    float scoped_sensitivity;
};




bool PatchAddress(void* target_address, const void* patch_bytes, size_t patch_size)
{
    memcpy(target_address, patch_bytes, patch_size);
    return true;
}

// Function to find the base address of RDR2.exe
uintptr_t GetModuleBaseAddress(const char* moduleName) {
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

bool StringToBool(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);

    if (lowerStr == "true" || lowerStr == "1") {
        return true;
    }
    else if (lowerStr == "false" || lowerStr == "0") {
        return false;
    }
    else {
        throw std::invalid_argument("Invalid string for boolean conversion: " + str);
    }
}

float GetPrivateProfileBool(const std::string& section, const std::string& key, bool defaultValue, const std::string& fileName) {
    char buffer[256];
    GetPrivateProfileStringA(section.c_str(), key.c_str(), "", buffer, sizeof(buffer), fileName.c_str());
    try {
        bool value;
        return StringToBool(buffer);
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

void ParseIniFile(struct MouseSensitivitySettings& mouse_sensitivity_settings)
{
    const std::string config_filename = "CustomMouseSensitivity.ini";
    mouse_sensitivity_settings.first_person_mouse_look_sensitivity = GetPrivateProfileFloat("FirstPerson", "FirstPersonMouseLookSensitivity", 1.0f, GetCurrentDirectoryFilePath(config_filename));
    mouse_sensitivity_settings.first_person_mouse_aim_sensitivity = GetPrivateProfileFloat("FirstPerson", "FirstPersonMouseAimSensitivity", 1.0f, GetCurrentDirectoryFilePath(config_filename));
    mouse_sensitivity_settings.third_person_mouse_look_sensitivity = GetPrivateProfileFloat("ThirdPerson", "ThirdPersonMouseLookSensitivity", 1.0f, GetCurrentDirectoryFilePath(config_filename));
    mouse_sensitivity_settings.third_person_mouse_aim_sensitivity = GetPrivateProfileFloat("ThirdPerson", "ThirdPersonMouseAimSensitivity", 1.0f, GetCurrentDirectoryFilePath(config_filename));
    mouse_sensitivity_settings.fov_independent_rotation = GetPrivateProfileBool("FirstPerson", "FOVIndependentRotation", true, GetCurrentDirectoryFilePath(config_filename));
    const float scoped_sensitivity = GetPrivateProfileFloat("Misc", "ScopedSensitivity", 1.0f, GetCurrentDirectoryFilePath(config_filename));
}

void main()
{
	// Get settings from INI file:
    struct MouseSensitivitySettings mouse_sensitivity_settings {};
    ParseIniFile(mouse_sensitivity_settings);

    constexpr float target_time_step = 0.0166667f;
    constexpr float base_first_person_fov = 55.0f;
    constexpr int kIntialInGameSliderValue = 0;
   
	// Varaibles to track game
    int screen_x, screen_y;
    int line_height = 10;
    float fov = 0.0f;

    bool is_first_person = false;
    bool is_in_scope = false;
    bool is_free_aiming = false;
    int core_value = 0;
    bool debug_enabled = false;

    char debug_label_text_buffer[64];

    // Get base adddress of game in main memory:
    uintptr_t base_address = GetModuleBaseAddress("RDR2.exe");

    // Before main loop, patch in-game slider values to all be 0.        
    PatchAddress(reinterpret_cast<void*>(base_address + kFirstPersonMouseLookOffset), reinterpret_cast<const void*>(&kIntialInGameSliderValue), sizeof(kIntialInGameSliderValue));
    PatchAddress(reinterpret_cast<void*>(base_address + kFirstPersonMouseAimOffset), reinterpret_cast<const void*>(&kIntialInGameSliderValue), sizeof(kIntialInGameSliderValue));
    PatchAddress(reinterpret_cast<void*>(base_address + kThirdPersonMouseMouseLookOffset), reinterpret_cast<const void*>(&kIntialInGameSliderValue), sizeof(kIntialInGameSliderValue));
    PatchAddress(reinterpret_cast<void*>(base_address + kThirdPersonMouseAimOffset), reinterpret_cast<const void*>(&kIntialInGameSliderValue), sizeof(kIntialInGameSliderValue));
       

    while (true)
    {
        // Acquire variables using ScriptHook:
        GRAPHICS::GET_SCREEN_RESOLUTION(&screen_x, &screen_y);
        fov = CAM::GET_GAMEPLAY_CAM_FOV();
        is_first_person = CAM::_0xD1BA66940E94C547();
        is_in_scope = PLAYER::_0x04D7F33640662FA2(PLAYER::PLAYER_ID());
        core_value = ATTRIBUTE::_0x36731AC041289BB1(PLAYER::PLAYER_PED_ID(), 1);
        is_free_aiming = PLAYER::IS_PLAYER_FREE_AIMING(PLAYER::PLAYER_ID());

        if (debug_enabled)
        {
            // LOGGING:
            UI::SET_TEXT_SCALE(0.5f, 0.5f);
            snprintf(debug_label_text_buffer, sizeof(debug_label_text_buffer), "FOV: %.4f", fov);
            UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", debug_label_text_buffer), 0.01f, 0.01f + 0 * 25.0f / screen_y);
            UI::SET_TEXT_SCALE(0.5f, 0.5f);
            UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", is_first_person ? "First person: True" : "First person: False"), 0.01f, 0.01f + 1 * 25.0f / screen_y);
            UI::SET_TEXT_SCALE(0.5f, 0.5f);
            UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", is_in_scope ? "Scoped: True" : "Scoped: False"), 0.01f, 0.01f + 2 * 25.0f / screen_y);
            UI::SET_TEXT_SCALE(0.5f, 0.5f);
            UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", is_free_aiming ? "FreeAiming: True" : "FreeAiming: False"), 0.01f, 0.01f +3 * 25.0f / screen_y);
            UI::SET_TEXT_SCALE(0.5f, 0.5f);
            snprintf(debug_label_text_buffer, sizeof(debug_label_text_buffer), "Stamina Core: %d", core_value);
            UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", debug_label_text_buffer), 0.01f, 0.01f + 4 * 25.0f / screen_y);

            // Input Values:
            UI::SET_TEXT_SCALE(0.5f, 0.5f);
            snprintf(debug_label_text_buffer, sizeof(debug_label_text_buffer), "ThirdPersonMouseLookSensitivity: %.4f", mouse_sensitivity_settings.third_person_mouse_look_sensitivity);
            UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", debug_label_text_buffer), 0.01f, 0.01f + 5 * 25.0f / screen_y);
            UI::SET_TEXT_SCALE(0.5f, 0.5f);
            snprintf(debug_label_text_buffer, sizeof(debug_label_text_buffer), "ThirdPersonMouseAimSensitivity: %.4f", mouse_sensitivity_settings.third_person_mouse_aim_sensitivity);
            UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", debug_label_text_buffer), 0.01f, 0.01f + 6 * 25.0f / screen_y);
            UI::SET_TEXT_SCALE(0.5f, 0.5f);
            snprintf(debug_label_text_buffer, sizeof(debug_label_text_buffer), "FirstPersonMouseLookSensitivity: %.4f", mouse_sensitivity_settings.first_person_mouse_look_sensitivity);
            UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", debug_label_text_buffer), 0.01f, 0.01f + 7 * 25.0f / screen_y);
            UI::SET_TEXT_SCALE(0.5f, 0.5f);
            snprintf(debug_label_text_buffer, sizeof(debug_label_text_buffer), "FirstPersonMouseAimSensitivity: %.4f", mouse_sensitivity_settings.first_person_mouse_aim_sensitivity);
            UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", debug_label_text_buffer), 0.01f, 0.01f + 8 * 25.0f / screen_y);
            UI::SET_TEXT_SCALE(0.5f, 0.5f);
            UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", mouse_sensitivity_settings.fov_independent_rotation ? "ScaleRotationWithFOV: True" : "ScaleRotationWithFOV: False"), 0.01f, 0.01f + 9 * 25.0f / screen_y);
            UI::SET_TEXT_SCALE(0.5f, 0.5f);
            snprintf(debug_label_text_buffer, sizeof(debug_label_text_buffer), "ScopedSensitivity: %.4f", mouse_sensitivity_settings.scoped_sensitivity);
            UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", debug_label_text_buffer), 0.01f, 0.01f + 10 * 25.0f / screen_y);
        }

        // Apply patches:
        {
            float patch_value = 0.0f;

            if (is_in_scope) // In scope
            {
                if (is_first_person)
                {
                    patch_value = FirstPersonScopedSenseScale * mouse_sensitivity_settings.scoped_sensitivity;
                }
                else
                {
                    patch_value = ThirdPersonScopedSenseScale * mouse_sensitivity_settings.scoped_sensitivity;
                }

                // Patch both values: 
                PatchAddress(reinterpret_cast<void*>(base_address + kFirstPersonScaleOffset), reinterpret_cast<const void*>(&patch_value), sizeof(float));
                PatchAddress(reinterpret_cast<void*>(base_address + kThirdPersonScaleOffset), reinterpret_cast<const void*>(&patch_value), sizeof(float));
            }
            else if (is_first_person) // First-person:
            {
                patch_value = FirstPersonAimScale;
                if (is_free_aiming)
                {
                    patch_value *= mouse_sensitivity_settings.first_person_mouse_aim_sensitivity;
                }
                else
                {
                    patch_value *= mouse_sensitivity_settings.first_person_mouse_look_sensitivity;
                }

                if (mouse_sensitivity_settings.fov_independent_rotation)
                {
                    patch_value *= base_first_person_fov / fov;
                }

                PatchAddress(reinterpret_cast<void*>(base_address + kFirstPersonScaleOffset), reinterpret_cast<const void*>(&patch_value), sizeof(float));
            }
            else // Third-person:
            {

                patch_value = ThirdPersonAimScale;
                if (is_free_aiming)
                {
                    patch_value *= mouse_sensitivity_settings.third_person_mouse_aim_sensitivity;
                }
                else
                {
                    patch_value *= mouse_sensitivity_settings.third_person_mouse_look_sensitivity;
                }

                if (is_free_aiming && core_value == 0)
                {
                    patch_value *= kCoreEmptyScale;
                }
                PatchAddress(reinterpret_cast<void*>(base_address + kThirdPersonScaleOffset), reinterpret_cast<const void*>(&patch_value), sizeof(float));
            }
        }
        if (IsKeyDown(VK_F9))
        {
            debug_enabled = !debug_enabled;
        }
        WAIT(0);
    }
}

void ScriptMain()
{
	srand(GetTickCount64());
	main();
}
