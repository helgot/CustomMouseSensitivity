#include "script.h"
#include "logger.h"


#include <cstdint>
#include <vector>
#include <ostream>
#include <unordered_map>
#include <Windows.h>
#include <imgui.h>
#include <MinHook.h>

#include <winnt.h>


#define HIDWORD(x) ((DWORD)(((unsigned __int64)(x) >> 32) & 0xFFFFFFFF))
#define LODWORD(l) (*((DWORD*)&(l)))
#define HIDWORD(l) (*((DWORD*)&(l)))




namespace GameData
{
    uintptr_t baseAddress = 0;

    uintptr_t thirdPersonHookAddress;
    uintptr_t firstPersonHookAddress;
    uintptr_t firstPerson2HookAddress;


    float third_person_mouse_sensitivity_scale = 1.0f;
    float aim_scale = 1.0f;

    std::unordered_map<std::string_view, int> int_probe;
    std::unordered_map<std::string_view, float> float_probe;
    std::unordered_map<std::string_view, DWORD> dword_probe;
    std::unordered_map<std::string_view, BYTE> byte_probe;
    std::unordered_map<std::string_view, uint64_t> qword_probe;
}

void DrawMemoryDump(uintptr_t addr, size_t size)
{
    if (addr == 0)
    {
        ImGui::Text("Camera pointer is null. Move the mouse in-game to capture.");
        return;
    }

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

    const uintptr_t start_addr = addr;
    const uintptr_t end_addr = start_addr + size;
    const int bytes_per_line = 16;

    char hex_buffer[bytes_per_line * 3 + 1] = { 0 };
    char ascii_buffer[bytes_per_line + 1] = { 0 };

    for (uintptr_t current_addr = start_addr; current_addr < end_addr; current_addr += bytes_per_line)
    {
        ImGui::Text("0x%p: ", (void*)current_addr);
        ImGui::SameLine();

        for (int i = 0; i < bytes_per_line; ++i)
        {
            if (current_addr + i < end_addr)
            {
                const unsigned char byte = *(unsigned char*)(current_addr + i);
                snprintf(&hex_buffer[i * 3], 4, "%02X ", byte);
                ascii_buffer[i] = (byte >= 32 && byte < 127) ? byte : '.';
            }
            else
            {
                snprintf(&hex_buffer[i * 3], 4, "   ");
                ascii_buffer[i] = ' ';
            }
        }
        ImGui::TextUnformatted(hex_buffer);
        ImGui::SameLine();
        ImGui::TextUnformatted(ascii_buffer);
    }

    ImGui::PopFont();
}

template<class T>
void PrintMap(const std::unordered_map<std::string_view, T>& map)
{
    if (map.empty())
    {
        ImGui::Text("map is null.");
        return;
    }

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

    for (const auto& [key, value] : map)
    {
        if constexpr (std::is_same_v<T, int>)
        {
            ImGui::Text("   %s: %d", key.data(), value);
        }
        else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
        {
            ImGui::Text("   %s: %.3f", key.data(), static_cast<double>(value));
        }
        else if constexpr (std::is_same_v<T, uint8_t>)
        {
            ImGui::Text("   %s: 0x%02X", key.data(), value);
        }
        else if constexpr (std::is_same_v<T, uint32_t>)
        {
            ImGui::Text("   %s: 0x%08X", key.data(), value);
        }
        else if constexpr (std::is_same_v<T, BYTE>)
        {
            ImGui::Text("   %s: 0x%02X", key.data(), value);
        }
        else
        {
            ImGui::Text("   %s: [unsupported type]", key.data());
        }
    }

    ImGui::PopFont();
}

void __fastcall ImGuiDrawCommandsCallback()
{
    ImGui::Begin("Custom Mouse Sensitivity");
    ImGui::Text("Framerate: %   .1f FPS", ImGui::GetIO().Framerate);
    ImGui::SliderFloat("3rd person scale", &GameData::third_person_mouse_sensitivity_scale, 0.0f, 10.0f);
    ImGui::SliderFloat("aim scale", &GameData::aim_scale, 0.0f, 10.0f);

    ImGui::Separator();
    PrintMap(GameData::float_probe);
    ImGui::Separator();
    PrintMap(GameData::byte_probe);
    ImGui::Separator();
    PrintMap(GameData::dword_probe);
    ImGui::Separator();
    PrintMap(GameData::qword_probe);
    ImGui::Separator();
    PrintMap(GameData::int_probe);
    ImGui::Separator();
    ImGui::End();
}

namespace Target {
	/*
		This is really ugly.
		Wow! Not having the editor auto-complete my comments is really nice.
		Yes, it is!
		Yes, yes, yes!
		
		Do I like my desk like this, the way it is positioned right now?

		Like this:
		--------------------
		|| |             ( |
	    || |           |   ||
	    ||  |          |   ||
		||  |          |   ||
		||  |          |   ||
		--------------------
		Yes, something like that.
		I need to get better at typing with the HHKB keyboard.

	*/  
    using t_sub_4162B0 = bool(__fastcall*)(void* a2);
    using t_sub_1461C0 = bool(__fastcall*)();
    using t_sub_2D32BE0 = void(__fastcall*)(void* buf);
    using t_sub_263DAF4 = double(__fastcall*)(uintptr_t arg1, uintptr_t arg2, int arg3, int arg4, int arg5);
    using t_sub_3ADEA8 = float(__fastcall*)(__int64 a1);
    using t_sub_391B78 = void (__fastcall*)(float* a1, float* a2);
    using t_sub_3915A8 = __int64(__fastcall*)(__int64 a1);
    using t_sub_3923E4 = float(__fastcall*)(__int64 a1);
    using t_sub_3ADF20 = float (__fastcall*)(__int64 a1, char a2);
    using t_sub_173550 = __int64 (*)();
    using t_sub_1B5C09C = char(__fastcall*)(__int64 a1, int a2);
    using t_sub_112F1D8 = __m128 (__fastcall*)(double a1, char a2);
    using t_sub_1489360 = float(__fastcall*)(float** a1, float a2, float a3, unsigned int a4);
    using t_sub_2C3F13C = double(__fastcall*)();
    using t_sub_11A5EBC = bool(__fastcall*)(__int64 a1, char a2);
    using t_sub_2C3F0FC = float(__fastcall*)(float a1);
    using t_sub_3B83FC = __m128 (__fastcall*)(float a1);

    // Function pointers to be initialized
    t_sub_4162B0  sub_4162B0;
    t_sub_1461C0  sub_1461C0;
    t_sub_2D32BE0 sub_2D32BE0;
    t_sub_263DAF4 sub_263DAF4;
    t_sub_3ADEA8 sub_3ADEA8;
    t_sub_391B78 sub_391B78;
    t_sub_3915A8 sub_3915A8;
    t_sub_3923E4 sub_3923E4;
    t_sub_3ADF20 sub_3ADF20;
    t_sub_173550 sub_173550;
    t_sub_1B5C09C sub_1B5C09C;
    t_sub_112F1D8 sub_112F1D8;
    t_sub_1489360 sub_1489360;
    t_sub_2C3F13C sub_2C3F13C;
    t_sub_11A5EBC sub_11A5EBC;
    t_sub_2C3F0FC sub_2C3F0FC;
    t_sub_3B83FC sub_3B83FC;


    // Pointers to the global variable
    int64_t* dword_39ECE40;
    int64_t* dword_39806BC;
    int64_t* dword_3973128;
    int64_t* dword_39806FC;
    int64_t* qword_39ECE48;
    int64_t* qword_571A380;
}

float __fastcall sub_3ADEA8(__int64 a1)
{
    float v1;
    float v2;
    float v3;
    float v4;
    float v6;

    v1 = 0.0;
    v2 = (float)*Target::dword_3973128 * 0.050000001;
    if (v2 >= 0.0)
        v3 = fminf(v2, 1.0);
    else
        v3 = 0.0;
    v4 = (float)*Target::dword_3973128 * 0.050000001;
    if (v4 >= 0.0)
        v1 = fminf(v4, 1.0);
    v6 = (float)((float)(v1 - v3) * *(float*)(a1 + 48)) + v3;
    Target::sub_3923E4((uint64_t) & v6);
    return v6;
}

/*
    TODO: Implement Loading/Saving from json configuration file, located beside this .asi file.
*/

/*
    Third-person aim sensitivity function:
    Supports setting the free-aim sensitivity, as well as the aiming sensitivity.
*/
__int64 __fastcall Hooked_sub_395E18(int64_t a1, int64_t a2, float a3, float a4)
{

    GameData::float_probe = {};
    GameData::byte_probe = {};
    GameData::qword_probe = {};

    float v5 = 0.0f; // xmm0_4
    float* v6; // rax
    float v7 = 0.0f; // xmm4_4
    float v8 = 0.0f; // xmm0_4
    float v9 = 0.0f; // xmm1_4
    float v11 = 0.0f; // [rsp+60h] [rbp+18h] BYREF
    float v12 = 0.0f; // [rsp+68h] [rbp+20h] BYREF

    *(float*)(a1 + 80) = a3;
    *(float*)(a1 + 84) = a4;
    v5 = Target::sub_3ADEA8(a1);
    v6 = *(float**)(a1 + 40);
    v7 = v5;



    GameData::float_probe.insert({ "v5", v5 });
    GameData::float_probe.insert({ "v7", v7 });
    GameData::float_probe.insert({ "(a1 + 48)", *(float*)(a1 + 48)});
    GameData::float_probe.insert({ "v6[11]", v6[11]});
    GameData::float_probe.insert({ "v6[13]", v6[13]});
    GameData::float_probe.insert({"v6[14] - v6[13])", v6[14] - v6[13]});
    GameData::float_probe.insert({ "v6[12] - v6[11]", v6[12] - v6[11]});
    GameData::float_probe.insert({ "dword_4A35EF0", *(float*)(GameData::baseAddress + 0x4A35EF0) });

    GameData::qword_probe.insert({ "328LL * ((*(_DWORD *)(v4 + 156) & 0x1FFFFu) - dword_39ECE40) + qword_39ECE48 + 240", *(uint64_t*)(328LL * ((*(DWORD*)(*(uint64_t*)(GameData::baseAddress + 0x3EC0638) + 156) & 0x1FFFFu) - *(DWORD*)(GameData::baseAddress + 0x39ECE40)) + *(uint64_t*)(GameData::baseAddress + 0x39ECE48) + 240) });
    GameData::byte_probe.insert({ "byte_5298473", *(byte*)(GameData::baseAddress + 0x5298473) });


    float aiming = *(float*)(a1 + 48); // 1.0f if aimed, 0.0f if not. (interpolated in transition)
    float aiming_scale = 1 + aiming*(GameData::aim_scale - 1);

    v8 = (float)((float)((float)((float)(v6[14] - v6[13]) * v5) + v6[13]) * 0.017453292) * *(float*)Target::dword_39806BC * GameData::third_person_mouse_sensitivity_scale * aiming_scale; // y_scale
    v11 = (float)((float)((float)((float)(v6[12] - v6[11]) * v5) + v6[11]) * 0.017453292) * *(float*)Target::dword_39806BC * GameData::third_person_mouse_sensitivity_scale * aiming_scale; // x_scale
    v12 = v8;
    Target::sub_391B78(&v11, &v12);
    v9 = v12 * a4;
    *(float*)(a1 + 88) = v11 * a3;
    *(float*)(a1 + 92) = v9;
    return Target::sub_3915A8(a1);
}


//char __fastcall sub_3F3434(__int64 a1, __int64 a2)
//{
//    __int64 v4; // rax
//    __int64 v5; // rsi
//    unsigned int v6; // r15d
//    __int128* v7; // rdx
//    __int64 v8; // r8
//    __int128* v9; // rdx
//    __int64 v10; // r8
//    __int128 v11; // xmm0
//    float v12; // xmm0_4
//    __int64 v13; // rax
//    double v14; // xmm0_8
//    float v15; // xmm0_4
//    __int64 v16; // rax
//    double v17; // xmm0_8
//    bool v18; // sf
//    __int64 v19; // rax
//    float* v20; // rdx
//    __int64 v21; // r8
//    __int64 v22; // rax
//    __int64 v23; // r9
//    int v24; // ecx
//    int v25; // ecx
//    __int64 v26; // r8
//    int v27; // r9d
//    float v28; // xmm6_4
//    float v29; // xmm2_4
//    float v30; // xmm1_4
//    float v31; // xmm2_4
//    __int64 v32; // rbx
//    __int64 v33; // r8
//    __int64 v34; // r8
//    double v35; // xmm0_8
//    int v36; // r8d
//    int v37; // r9d
//    __int64 v38; // r8
//    int v39; // r9d
//    float v40; // xmm8_4
//    float v41; // xmm3_4
//    __int64 v42; // r8
//    float v43; // xmm11_4
//    float v44; // xmm13_4
//    double v45; // xmm0_8
//    float v46; // xmm9_4
//    float v47; // xmm9_4
//    char v48; // dl
//    char v49; // cl
//    bool v50; // cf
//    float v51; // xmm0_4
//    float v52; // xmm0_4
//    float v53; // xmm2_4
//    float* v54; // rdx
//    __int64 v55; // rax
//    double v56; // xmm0_8
//    float v57; // xmm6_4
//    float v58; // xmm1_4
//    float v59; // xmm10_4
//    float v60; // xmm1_4
//    __int64 v61; // r8
//    double v62; // xmm0_8
//    float v64; // [rsp+28h] [rbp-E0h]
//    _QWORD v65[3]; // [rsp+40h] [rbp-C8h] BYREF
//    float v66; // [rsp+58h] [rbp-B0h] BYREF
//    float v67; // [rsp+5Ch] [rbp-ACh]
//    __int64 v68; // [rsp+68h] [rbp-A0h] BYREF
//    __int128 v69; // [rsp+70h] [rbp-98h] BYREF
//    __int128 v70; // [rsp+80h] [rbp-88h] BYREF
//    int v71; // [rsp+90h] [rbp-78h]
//    int v72; // [rsp+94h] [rbp-74h]
//    __int128 v73; // [rsp+98h] [rbp-70h] BYREF
//    char v74; // [rsp+A8h] [rbp-60h] BYREF
//    float v75; // [rsp+198h] [rbp+90h] BYREF
//    float v76; // [rsp+1A0h] [rbp+98h] BYREF
//
//    v4 = sub_3ADDD8();
//    v5 = v4;
//    if (v4)
//    {
//        *(_DWORD*)(a1 + 196) = 0;
//        v6 = 228;
//        if ((*(_BYTE*)(a1 + 260) & 4) != 0)
//        {
//            if ((unsigned __int8)sub_AAB8A0(v4))
//            {
//                v7 = &v70;
//                v8 = 228LL;
//                goto LABEL_10;
//            }
//            v8 = 43LL;
//        }
//        else
//        {
//            if ((unsigned __int8)sub_AAB8A0(v4))
//            {
//                v7 = &v70;
//                v8 = 235LL;
//                goto LABEL_10;
//            }
//            v8 = 2LL;
//        }
//        v7 = (__int128*)&v65[1];
//    LABEL_10:
//        v69 = *(_OWORD*)sub_AA5D00(v5, v7, v8);
//        if ((unsigned __int8)sub_AAB8A0(v5))
//        {
//            v9 = &v70;
//            v10 = 236LL;
//        }
//        else
//        {
//            v9 = (__int128*)&v65[1];
//            v10 = 3LL;
//        }
//        v11 = *(_OWORD*)sub_AA5D00(v5, v9, v10);
//        v68 = qword_3C7C390;
//        v73 = v11;
//        if (fabs(*(float*)(a1 + 56)) >= 0.000001)
//        {
//            v13 = sub_2D32BE0(&v69);
//            v14 = sub_2635338(v13, &v68);
//            v12 = -*(float*)&v14;
//        }
//        else
//        {
//            v12 = 0.0;
//        }
//        v75 = v12;
//        if (fabs(*(float*)(a1 + 60)) >= 0.000001)
//        {
//            v16 = sub_2D32BE0(&v73);
//            v17 = sub_2635338(v16, &v68);
//            v15 = -*(float*)&v17;
//        }
//        else
//        {
//            v15 = 0.0;
//        }
//        v18 = *(char*)(a2 + 208) < 0;
//        v76 = v15;
//        if (v18)
//        {
//            v75 = 0.0;
//            v76 = 0.0;
//        }
//        else
//        {
//            v19 = sub_173550();
//            if (v19
//                && (unsigned __int8)sub_2209E4(
//                    (*(_QWORD*)(328LL * ((*(_DWORD*)(v19 + 156) & 0x1FFFFu) - dword_39ECE40)
//                        + qword_39ECE48
//                        + 256) & 0xFFFFFFFFFFFFFFFEuLL & -(__int64)(*(_QWORD*)(328LL
//                            * ((*(_DWORD*)(v19 + 156) & 0x1FFFFu)
//                                - dword_39ECE40)
//                            + qword_39ECE48
//                            + 256) != 0LL))
//                    + 168,
//                    248LL)
//                && (*(_BYTE*)(a1 + 260) & 4) == 0)
//            {
//                v75 = -v75;
//                v76 = -v76;
//            }
//        }
//        if ((*(_BYTE*)(a1 + 260) & 4) != 0)
//        {
//            if ((unsigned __int8)sub_AAB8A0(v5))
//            {
//                v20 = (float*)&v65[1];
//                v21 = 229LL;
//            }
//            else
//            {
//                v20 = &v66;
//                v21 = 44LL;
//            }
//            v70 = *(_OWORD*)sub_AA5D00(v5, v20, v21);
//            v22 = sub_2D32BE0(&v70);
//            sub_2635338(v22, &v68);
//            *(_DWORD*)(a1 + 248) = 1065353216;
//        }
//        v23 = *(_QWORD*)(a1 + 40);
//        if (*(_BYTE*)(v23 + 197))
//            sub_397F40(a1 + 216, a1 + 220, (unsigned int)&v75, (unsigned int)&v76, a1 + 184, a1 + 188, *(_DWORD*)(v23 + 168));
//        v72 = HIDWORD(qword_571A380);
//        v71 = 0;
//        sub_2D32BE0(&v69);
//        if ((*(_BYTE*)(a1 + 260) & 4) != 0)
//        {
//            if (v75 >= -1.0)
//            {
//                if (v75 > 1.0)
//                    v75 = 1.0;
//            }
//            else
//            {
//                v75 = -1.0;
//            }
//        }
//        sub_2D32BE0(&v73);
//        if ((*(_BYTE*)(a1 + 260) & 4) != 0)
//        {
//            if (v76 >= -1.0)
//            {
//                if (v76 > 1.0)
//                    v76 = 1.0;
//            }
//            else
//            {
//                v76 = -1.0;
//            }
//        }
//
//        v24 = *(_DWORD*)(sub_2D32BE0(&v69) + 16);
//        // If mouse moved.
//        if ((v24 & 0xFF00000) == 0x100000
//            || (v24 & 0xFF00000) == 0
//            || (v24 & 0xFF00000) == 0x200000
//            || (v25 = *(_DWORD*)(sub_2D32BE0(&v73) + 16), (v25 & 0xFF00000) == 0x100000)
//            || (v25 & 0xFF00000) == 0
//            || (v25 & 0xFF00000) == 0x200000)
//        {
//            sub_3CAED8(a1, a2, &v75, &v76);
//            v41 = v75;
//            v64 = v76;
//            *(float*)(a1 + 196) = fsqrt((float)(v76 * v76) + (float)(v75 * v75));
//            sub_395EE0(a1, a2, v42, v41, v64);
//            v40 = v76;
//            *(float*)(a1 + 196) = fsqrt((float)(v76 * v76) + (float)(v75 * v75));
//        }
//        else
//        {
//            v28 = (float)(v76 * v76) + (float)(v75 * v75);
//            if (v28 > 0.0099999998)
//            {
//                if (v28 <= 0.0)
//                {
//                    v30 = 0.0;
//                    v31 = 0.0;
//                }
//                else
//                {
//                    v29 = 1.0 / fsqrt(v28);
//                    v30 = v29 * v76;
//                    v31 = v29 * v75;
//                }
//                v66 = v31;
//                v67 = v30;
//                if (!(unsigned __int8)sub_1461C0() || (v32 = *(_QWORD*)(a1 + 40), *(_BYTE*)(v32 + 172)))
//                {
//                    v65[1] = 0x3F80000000000000LL;
//                    sub_3AAA9C(&v65[1], &v66, v26, &v75);
//                    v65[1] = 1065353216LL;
//                    sub_3AAA9C(&v65[1], &v66, v34, &v76);
//                }
//                else if (*(_BYTE*)(v32 + 173))
//                {
//                    v65[1] = 0x3F80000000000000LL;
//                    sub_3AAAEC(&v65[1], &v66, v26, &v75);
//                    v65[1] = 1065353216LL;
//                    sub_3AAAEC(&v65[1], &v66, v33, &v76);
//                }
//            }
//            if (v28 <= 0.0)
//                LODWORD(v35) = 0;
//            else
//                v35 = sub_263DAF4((unsigned int)&v75, (unsigned int)&v76, v26, v27, 1);
//            *(_DWORD*)(a1 + 196) = LODWORD(v35);
//            sub_3CAED8(a1, a2, &v75, &v76);
//            sub_395964(a1, a2, v36, v37, LODWORD(v76));
//            v40 = v76;
//        }
//
//
//        *(_DWORD*)(a2 + 196) = *(_DWORD*)(a1 + 228);
//        if ((*(_BYTE*)(a1 + 260) & 4) != 0 && *(float*)(a1 + 196) > 0.0)
//        {
//            v43 = 0.017453292;
//            v44 = *(float*)(*(_QWORD*)(a1 + 40) + 64LL);
//            v45 = sub_2EAC154();
//            v46 = *(float*)&v45;
//            if (fabs(COERCE_FLOAT(COERCE_UNSIGNED_INT64(sub_2EAC154()))) > (float)(v44 * 0.017453292))
//            {
//                if (*(_QWORD*)(a1 + 144))
//                {
//                    if (!(unsigned __int8)sub_175DB0())
//                    {
//                        if (v46 > 0.0)
//                            v43 = -0.017453292;
//                        v47 = (float)(v46 - (float)(v43 * v44)) * 0.63661975;
//                        v75 = v47;
//                        if (v47 >= -1.0)
//                        {
//                            if (v47 > 1.0)
//                                v75 = 1.0;
//                        }
//                        else
//                        {
//                            v75 = -1.0;
//                        }
//                    }
//                    v40 = v76;
//                }
//            }
//            else
//            {
//                v75 = 0.0;
//            }
//        }
//        v48 = *(_BYTE*)(a1 + 260);
//        v49 = v48;
//        if ((v48 & 4) == 0 && (v48 & 8) != 0)
//        {
//            v50 = *(float*)(a1 + 196) > 0.0;
//            if (*(float*)(a1 + 196) > 0.0)
//            {
//            LABEL_96:
//                *(_BYTE*)(a1 + 260) = v49 & 0xFE | v50;
//                *(_BYTE*)(a2 + 208) &= ~2u;
//                *(_BYTE*)(a2 + 208) |= (v49 & 4 | v50) != 1 ? 0 : 2;
//                LOBYTE(v4) = sub_416268(v5);
//                *(_BYTE*)(a2 + 210) &= ~8u;
//                LOBYTE(v4) = 8 * v4;
//                *(_BYTE*)(a2 + 210) |= v4;
//                return v4;
//            }
//            v51 = *(float*)(a1 + 248);
//            if (v51 > 0.001)
//            {
//                *(float*)(a1 + 248) = v51 * 0.75;
//                v52 = fabs(*(float*)(a1 + 56));
//                *(_QWORD*)&v70 = qword_3C7C390;
//                if (v52 >= 0.000001)
//                {
//                    if ((unsigned __int8)sub_AAB8A0(v5))
//                    {
//                        v54 = &v66;
//                    }
//                    else
//                    {
//                        v54 = (float*)&v74;
//                        v6 = 43;
//                    }
//                    *(_OWORD*)&v65[1] = *(_OWORD*)sub_AA5D00(v5, v54, v6);
//                    v55 = sub_2D32BE0(&v65[1]);
//                    v56 = sub_2635338(v55, &v70);
//                    v49 = *(_BYTE*)(a1 + 260);
//                    v40 = v76;
//                    v53 = -(float)(*(float*)&v56 * *(float*)(a1 + 248));
//                }
//                else
//                {
//                    v53 = 0.0;
//                }
//                if (fabs(v53) > 0.000001)
//                {
//                    v75 = v53;
//                    v57 = (float)(v53 * v53) + (float)(v40 * v40);
//                    if (v57 > 0.0099999998)
//                    {
//                        if (v57 <= 0.0)
//                        {
//                            v59 = 0.0;
//                            v60 = 0.0;
//                        }
//                        else
//                        {
//                            v58 = 1.0 / fsqrt(v57);
//                            v59 = v58 * v40;
//                            v60 = v58 * v53;
//                        }
//                        v65[1] = 0x3F80000000000000LL;
//                        v66 = v60;
//                        v67 = v59;
//                        sub_3AAA9C(&v65[1], &v66, v38, &v75);
//                        v65[1] = 1065353216LL;
//                        sub_3AAA9C(&v65[1], &v66, v61, &v76);
//                    }
//                    if (v57 > 0.0)
//                    {
//                        v62 = sub_263DAF4((unsigned int)&v75, (unsigned int)&v76, v38, v39, 1);
//                        *(_DWORD*)(a1 + 196) = LODWORD(v62);
//                    }
//                    v49 = *(_BYTE*)(a1 + 260) & 0xFB | (*(float*)(a1 + 196) <= 0.000001 ? 0 : 4);
//                }
//            }
//        }
//        v50 = *(float*)(a1 + 196) > 0.0;
//        goto LABEL_96;
//    }
//    return v4;
//}

/*
    First-person aim sensitivity function:
    TODO:
        - Support setting the aim sensitivity and the free-aim sensitivity.
        - Support setting the scoped sensititivity (this should be handled in this function)
        - Toggle to scale with fov (the third-person function didn't do this, but this one should (based on the observed behaviour)).
*/
void __fastcall Hooked_sub_395EE0(__int64 a1, __int64 a2, __int64 a3, float a4, float a5)
{


    GameData::float_probe = {};
    GameData::byte_probe = {};
    GameData::dword_probe = {};
    GameData::qword_probe = {};

    int v7; // xmm0_4
    float v8; // xmm6_4
    float v9; // xmm4_4
    float v10; // xmm0_4
    float v11; // xmm0_4
    float v12; // xmm6_4
    float v13; // [rsp+68h] [rbp+20h] BYREF

    if (*(char*)(a2 + 208) < 0)
        v7 = 0;
    else
        v7 = 1065353216;

    v8 = a5;
    *(float*)(a1 + 172) = a5;
    *(DWORD*)(a1 + 228) = v7;
    *(float*)(a1 + 168) = a4;
    v9 = Target::sub_3ADF20(a1, (*(BYTE*)(a2 + 211) & 8) != 0);

    GameData::float_probe.insert({ "v9", v9});

    // ADS check value. 0x8 -> ADS
    GameData::byte_probe.insert({ "*(BYTE*)(a2 + 211) & 8", *(BYTE*)(a2 + 211) & 8});

    GameData::float_probe.insert({ "(a1 + 88)", *(float*)(a1 + 88) });
    GameData::float_probe.insert({ "(a1 + 84)", *(float*)(a1 + 84) });

    GameData::float_probe.insert({ "(a1 + 80)", *(float*)(a1 + 80) });
    GameData::float_probe.insert({ "(a1 + 76)", *(float*)(a1 + 76) });

    GameData::float_probe.insert({ "* (float*)Target::dword_39806FC", *(float*)Target::dword_39806FC });

    v10 = (float)((float)((float)((float)(*(float*)(a1 + 88) - *(float*)(a1 + 84)) * v9) + *(float*)(a1 + 84)) * 0.017453292) * *(float*)Target::dword_39806FC;
    a5 = (float)((float)((float)((float)(*(float*)(a1 + 80) - *(float*)(a1 + 76)) * v9) + *(float*)(a1 + 76)) * 0.017453292) * *(float*)Target::dword_39806FC;
    v13 = v10;
    
    Target::sub_391B78(&a5, &v13);
    
    v11 = -3.1415927;
    *(float*)(a2 + 128) = (float)(a4 * *(float*)(a1 + 240)) * a5;
    
    v12 = (float)(v8 * *(float*)(a1 + 244)) * v13;
    
    if (v12 >= -3.1415927)
        v11 = fminf(v12, 3.1415927);
    *(float*)(a2 + 136) = v11;

    //if (a4 != 0.0f)
    //    *(float*)(a2 + 128) = 0.05f * a4 / abs(a4);
    //if (v8 != 0.0f)
    //    *(float*)(a2 + 136) = 0.05f * v8 / abs(v8);

    GameData::float_probe.insert({ "(a2 + 128)", *(float*)(a2 + 128) });
    GameData::float_probe.insert({ "(a2 + 136)", *(float*)(a2 + 136) });
    GameData::float_probe.insert({ "(a1 + 240)", *(float*)(a1 + 240) });
    GameData::float_probe.insert({ "(a1 + 244)", *(float*)(a1 + 244) });
    GameData::float_probe.insert({ "*(float*)(a1 + 172)", *(float*)(a1 + 172) });
    GameData::float_probe.insert({ "*(float*)(a1 + 168)", *(float*)(a1 + 168) });
    
    GameData::dword_probe.insert({ "*(DWORD*)(a1 + 228)", *(DWORD*)(a1 + 228) });
}

void HookGameFunctions()
{
    GameData::baseAddress = (uintptr_t)GetModuleHandleA(NULL);
    LOG_DEBUG("Base address: 0x%llx", GameData::baseAddress);

    Target::sub_4162B0 = reinterpret_cast<Target::t_sub_4162B0>(GameData::baseAddress + 0x4162B0);
    Target::sub_2D32BE0 = reinterpret_cast<Target::t_sub_2D32BE0>(GameData::baseAddress + 0x2D32BE0);
    Target::sub_263DAF4 = reinterpret_cast<Target::t_sub_263DAF4>(GameData::baseAddress + 0x263DAF4);
    Target::sub_3ADEA8 = reinterpret_cast<Target::t_sub_3ADEA8>(GameData::baseAddress + 0x3ADEA8);
    Target::sub_391B78 = reinterpret_cast<Target::t_sub_391B78>(GameData::baseAddress + 0x391B78);
    Target::sub_3915A8 = reinterpret_cast<Target::t_sub_3915A8>(GameData::baseAddress + 0x3915A8);
    Target::sub_3923E4 = reinterpret_cast<Target::t_sub_3923E4>(GameData::baseAddress + 0x3923E4);
    Target::sub_3ADF20 = reinterpret_cast<Target::t_sub_3ADF20>(GameData::baseAddress + 0x3ADF20);
    Target::sub_173550 = reinterpret_cast<Target::t_sub_173550>(GameData::baseAddress + 0x173550);


    Target::sub_1B5C09C = reinterpret_cast<Target::t_sub_1B5C09C>(GameData::baseAddress + 0x1B5C09C);
    Target::sub_112F1D8 = reinterpret_cast<Target::t_sub_112F1D8>(GameData::baseAddress + 0x112F1D8);
    Target::sub_1489360 = reinterpret_cast<Target::t_sub_1489360>(GameData::baseAddress + 0x1489360);
    Target::sub_2C3F13C = reinterpret_cast<Target::t_sub_2C3F13C>(GameData::baseAddress + 0x2C3F13C);
    Target::sub_11A5EBC = reinterpret_cast<Target::t_sub_11A5EBC>(GameData::baseAddress + 0x11A5EBC);
    Target::sub_2C3F0FC = reinterpret_cast<Target::t_sub_2C3F0FC>(GameData::baseAddress + 0x2C3F0FC);
    Target::sub_3B83FC = reinterpret_cast<Target::t_sub_3B83FC>(GameData::baseAddress + 0x3B83FC);



    Target::qword_571A380 = (int64_t*)(GameData::baseAddress + 0x571A380);
    Target::dword_39806BC = (int64_t*)(GameData::baseAddress + 0x39806BC);
    Target::dword_39806FC = (int64_t*)(GameData::baseAddress + 0x39806FC);
    Target::dword_3973128 = (int64_t*)(GameData::baseAddress + 0x3973128);
    Target::dword_39ECE40 = (int64_t*)(GameData::baseAddress + 0x39ECE40);
    Target::qword_39ECE48 = (int64_t*)(GameData::baseAddress + 0x39ECE48);
        
    GameData::thirdPersonHookAddress = GameData::baseAddress + 0x395E18;
    MH_CreateHook((LPVOID)GameData::thirdPersonHookAddress, &Hooked_sub_395E18, NULL);
    MH_EnableHook((LPVOID)GameData::thirdPersonHookAddress);

    GameData::firstPersonHookAddress = GameData::baseAddress + 0x395EE0;
    MH_CreateHook((LPVOID)GameData::firstPersonHookAddress, &Hooked_sub_395EE0, NULL);
    MH_EnableHook((LPVOID)GameData::firstPersonHookAddress);

}

void UnhookGameFunctions()
{
    MH_DisableHook((LPVOID)GameData::thirdPersonHookAddress);
    MH_RemoveHook((LPVOID)GameData::thirdPersonHookAddress);
    MH_DisableHook((LPVOID)GameData::firstPersonHookAddress);
    MH_RemoveHook((LPVOID)GameData::firstPersonHookAddress);
    LOG_DEBUG("Game functions unhooked.");
}

