# Description
A mod to allow for arbitrary selection of first person and third person mouse sensitivities in Red Dead Redemption 2 PC.

[Nexus Page](https://www.nexusmods.com/reddeadredemption2/mods/5463)

# Building

This project can be built using **CMake**, **Ninja**, and **Visual Studio 2022 (MSVC)**.

---

## Prerequisites

Ensure the following tools are installed:

- [CMake](https://cmake.org/download/)
- [Ninja](https://github.com/ninja-build/ninja/releases)
- [Visual Studio 2022](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload

---

## Build Instructions (Windows / MSVC)

### 1. Clone the Repository
```powershell
git clone --recurse-submodules https://github.com/helgot/CustomMouseSensitivity.git
cd CustomMouseSensitivity
```

### 2. Setup MSVC Environment
```
.\start-msvc-env.ps1
```

### 3. Configure the build
```
mkdir build
cd build
cmake -G Ninja ..
```
### 4. Build
```
ninja
```
CustomMouseSensitivity.asi will be built in the build/ directory.

### Notes
If cl.exe cannot be found, make sure you are running inside a Visual Studio Developer Command Prompt or that start-msvc-env.ps1 executed successfully.

If submodules are missing, rerun:
```
git submodule update --init --recursive
```

# Credits
[ImGui](https://github.com/ocornut/imgui)

[MinHook](https://github.com/TsudaKageyu/minhook)

[Nlohmann JSON](https://github.com/nlohmann/json)
