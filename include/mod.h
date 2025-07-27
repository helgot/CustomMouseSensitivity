#pragma once
#include <memory>
#include <string>

#include "i_renderhook.h"

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
private:
    std::unique_ptr<IRenderHook> m_rendererHook;
};