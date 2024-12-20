#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

#include "third_party\rdr2scripthook\inc\main.h"
#include "script.h"



BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		scriptRegister(hInstance, ScriptMain);
		break;
	case DLL_PROCESS_DETACH:
		scriptUnregister(hInstance);
		break;
	}		
	return TRUE;
}