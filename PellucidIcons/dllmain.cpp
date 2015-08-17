/****************************** Module Header ******************************\
Module Name:  dllmain.cpp
Project:      Pellucid
Copyright (c) Sanjeev Sharma
Copyright (c) Microsoft Corporation.

The file implements DllMain, and the DllGetClassObject, DllCanUnloadNow, 
DllRegisterServer, DllUnregisterServer functions that are necessary for a COM 
DLL. 

DllGetClassObject invokes the class factory defined in ClassFactory.h/cpp and 
queries to the specific interface.

DllCanUnloadNow checks if we can unload the component from the memory.

DllRegisterServer registers the COM server in 
the registry by invoking the helper functions defined in Reg.h/cpp. The 
context menu handler is associated with the .cpp file class.

DllUnregisterServer unregisters the COM server. 

This source is subject to the Microsoft Public License.
See http://www.microsoft.com/opensource/licenses.mspx#Ms-PL.
All other rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include <windows.h>
#include <Guiddef.h>
#include <atlbase.h>
#include <atlcom.h>
#include "ClassFactory.h"           // For the class factory
#include "Reg.h"


// {BBB60B71-54FB-4FCB-8537-689BEC7256B3}
static const CLSID CLSID_Pellucid = { 0xbbb60b71, 0x54fb, 0x4fcb, 0x85, 0x37, 0x68, 0x9b, 0xec, 0x72, 0x56, 0xb3 };
#define APP_NAME		L"Pellucid Icons"
#define CLASS_NAME		APP_NAME L" Class"


HINSTANCE   g_hInst     = NULL;
long        g_cDllRef   = 0;


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
        // Hold the instance of this DLL module, we will use it to get the 
        // path of the DLL to register the component.
        g_hInst = hModule;
        DisableThreadLibraryCalls(hModule);
        break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


//
//   FUNCTION: DllGetClassObject
//
//   PURPOSE: Create the class factory and query to the specific interface.
//
//   PARAMETERS:
//   * rclsid - The CLSID that will associate the correct data and code.
//   * riid - A reference to the identifier of the interface that the caller 
//     is to use to communicate with the class object.
//   * ppv - The address of a pointer variable that receives the interface 
//     pointer requested in riid. Upon successful return, *ppv contains the 
//     requested interface pointer. If an error occurs, the interface pointer 
//     is NULL. 
//
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

	if (IsEqualCLSID(CLSID_Pellucid, rclsid))
    {
        hr = E_OUTOFMEMORY;

        ClassFactory *pClassFactory = new ClassFactory();
        if (pClassFactory)
        {
            hr = pClassFactory->QueryInterface(riid, ppv);
            pClassFactory->Release();
        }
    }

    return hr;
}


//
//   FUNCTION: DllCanUnloadNow
//
//   PURPOSE: Check if we can unload the component from the memory.
//
//   NOTE: The component can be unloaded from the memory when its reference 
//   count is zero (i.e. nobody is still using the component).
// 
STDAPI DllCanUnloadNow(void)
{
    return g_cDllRef > 0 ? S_FALSE : S_OK;
}


//
//   FUNCTION: DllRegisterServer
//
//   PURPOSE: Register the COM server and the context menu handler.
// 
STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    wchar_t szModule[MAX_PATH];
    if (GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule)) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    // Register the component.
	hr = RegisterInprocServer(szModule,
								CLSID_Pellucid,
								CLASS_NAME,
								L"Apartment");
    if (SUCCEEDED(hr))
    {
        // Register Explorer desktop context handler.
		hr = RegisterExplorerDesktopContextMenuHandler(CLSID_Pellucid, APP_NAME);

		// Register Explorer icon overlay handler.
		if (SUCCEEDED(hr))
		{
			hr = RegisterExplorerIconOverlayHandler(CLSID_Pellucid, APP_NAME);
		}
		else
			UnregisterExplorerDesktopContextMenuHandler(APP_NAME);
    }

    return hr;
}


//
//   FUNCTION: DllUnregisterServer
//
//   PURPOSE: Unregister the COM server and the context menu handler.
// 
STDAPI DllUnregisterServer(void)
{
    HRESULT hr = S_OK;

    wchar_t szModule[MAX_PATH];
    if (GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule)) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    // Unregister the component.
	hr = UnregisterInprocServer(CLSID_Pellucid);
    if (SUCCEEDED(hr))
    {
        // Unregister Explorer desktop context handler.
		hr = UnregisterExplorerDesktopContextMenuHandler(APP_NAME);

		if (SUCCEEDED(hr))
		{
			hr = UnregisterExplorerIconOverlayHandler(APP_NAME);
		}
		else
			UnregisterExplorerIconOverlayHandler(APP_NAME);
    }

    return hr;
}