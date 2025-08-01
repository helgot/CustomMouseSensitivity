#pragma once

#include <memory>
#include <vector> 
#include <cstdlib>

#include "i_renderhook.h"

struct Hook
{
    void* original;
    void* detour; 
    void* trampoline;
};

namespace CustomSensitivity
{
    class Mod
    {
    public:
        Mod(const Mod&) = delete;
        Mod& operator=(const Mod&) = delete;
        Mod(Mod&&) = delete;
        Mod& operator=(Mod&&) = delete;

        static Mod& GetInstance();
        void Start();
        void Shutdown();
    private:
        Mod();
        ~Mod();
        bool HookGameFunctions();
        void UnhookGameFunctions();
        bool CreateHooks();
        bool EnableHooks();
        bool LoadConfig();

    private:
        std::vector<Hook> m_functionHooks;
        std::unique_ptr<IRenderHook> m_rendererHook;
    };
}