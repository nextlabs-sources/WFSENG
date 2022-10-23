// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

#include <iostream>

HMODULE nlwfseInstance;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    nlwfseInstance = hModule;

    switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
    std::cout << "call dll process attach \n";
    break;
	case DLL_THREAD_ATTACH:
    std::cout << "call dll thread attach \n";
    break;
	case DLL_THREAD_DETACH:
    std::cout << "call dll thread detach \n";
    break;
	case DLL_PROCESS_DETACH:
    std::cout << "call dll process detach \n";
    break;
	}
	return TRUE;
}

