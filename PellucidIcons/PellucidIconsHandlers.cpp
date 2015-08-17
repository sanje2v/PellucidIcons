#include "PellucidIconsHandlers.h"
#include "Settings.h"
#include "Utility.h"
#include "resource.h"
#include <strsafe.h>
#include <Shlwapi.h>
#include <process.h>
#include <windowsx.h>

#define MOUSEPOS_HISTORYDEPTH		4

// Variables from external .cpp
extern HINSTANCE g_hInst;
extern long g_cDllRef;

// Static variables
bool PellucidHandlers::s_bIsMouseOnShellWindow = false;
sized_queue<POINT> PellucidHandlers::s_queueMousePos(MOUSEPOS_HISTORYDEPTH);
bool PellucidHandlers::s_bPellucidIcons = true;
LONG PellucidHandlers::s_quarterWidthShellWindow = 0;
HBITMAP PellucidHandlers::s_hbitmapPellucidIcon = NULL;
HANDLE PellucidHandlers::s_heventExitThreadFunc = NULL;
HANDLE PellucidHandlers::s_hWaitThread = NULL;
HWND PellucidHandlers::s_hwndShellWindow = NULL;
LONG_PTR PellucidHandlers::s_hPrevShellWindowWndProc = NULL;


// Member functions for 'PellucidHandlers' class
PellucidHandlers::PellucidHandlers(void) : m_cRef(1)
{
	InterlockedIncrement(&g_cDllRef);
}

PellucidHandlers::~PellucidHandlers(void)
{
	InterlockedDecrement(&g_cDllRef);
}


#pragma region IUnknown

// Query to the interface the component supported.
IFACEMETHODIMP PellucidHandlers::QueryInterface(REFIID riid, void **ppv)
{
	static const QITAB qit[] =
	{
		QITABENT(PellucidHandlers, IShellIconOverlayIdentifier),
		QITABENT(PellucidHandlers, IContextMenu),
		QITABENT(PellucidHandlers, IShellExtInit),
		{ NULL },
	};

	return QISearch(this, qit, riid, ppv);
}

// Increase the reference count for an interface on an object.
IFACEMETHODIMP_(ULONG) PellucidHandlers::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

// Decrease the reference count for an interface on an object.
IFACEMETHODIMP_(ULONG) PellucidHandlers::Release()
{
	ULONG cRef = InterlockedDecrement(&m_cRef);
	if (cRef == 0)
	{
		delete this;
	}

	return cRef;
}

#pragma endregion

#pragma region IShellIconOverlayIdentifier

IFACEMETHODIMP PellucidHandlers::GetOverlayInfo(PWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
	HRESULT hr;

	// Force settings read from registry
	Settings::ForceSettingsRefreshFromRegistry();	// IMPORTANT: Must be called before any get settings function

	// Find desktop ListView
	auto hwndProgMan = FindWindow(L"Progman", L"Program Manager");
	auto hwndShellView = FindWindowEx(hwndProgMan, NULL, L"ShellDLL_DefView", L"");
	auto hwndFolderView = FindWindowEx(hwndShellView, NULL, L"SysListView32", L"FolderView");
	if (!hwndFolderView)
		return E_UNEXPECTED;

	s_hwndShellWindow = hwndFolderView;	// We use this in our subclassing window procedure
	// Calculate one third region, we may use this later
	RECT rectShellWindow = { 0 };
	GetWindowRect(s_hwndShellWindow, &rectShellWindow);
	s_quarterWidthShellWindow = (rectShellWindow.right - rectShellWindow.left) / 3;

	// IMPORTANT: Add 'WS_EX_LAYERED' to ListView's extended window style, so that we can
	//			  using 'SetLayeredAttributes()'
	auto currentExStyle = GetWindowLongPtr(hwndFolderView, GWL_EXSTYLE);
	if ((currentExStyle & WS_EX_LAYERED) == 0)
	{
		SetWindowLongPtr(hwndFolderView, GWL_EXSTYLE, currentExStyle | WS_EX_LAYERED);

		if (SetLayeredWindowAttributes(hwndFolderView, NULL, 0xFF, LWA_ALPHA) == FALSE)
			return E_UNEXPECTED;
	}

	// Subclass listview's window procedure
	s_hPrevShellWindowWndProc = SetWindowLongPtr(s_hwndShellWindow, GWLP_WNDPROC, (LONG_PTR)ShellWindow_WndProc);
	if (!s_hPrevShellWindowWndProc)
		return E_FAIL;

	// Create a timer thread if this extension is enabled
	if (Settings::getIsEnabled())
		ResetTimer();

	// We return a dummy icon index in this module's resource table
	hr = (GetModuleFileName(g_hInst, pwszIconFile, cchMax) != 0 ? S_OK : E_UNEXPECTED);
	if (SUCCEEDED(hr))
	{
		*pIndex = 0;
		*pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;
	}

	return hr;
}

IFACEMETHODIMP PellucidHandlers::GetPriority(int *pPriority)
{
	*pPriority = 0;			// First, priority (paint on top) - like it matters (:

	return S_OK;
}

IFACEMETHODIMP PellucidHandlers::IsMemberOf(PCWSTR pwszPath, DWORD dwAttrib)
{
	return S_FALSE;			// We never use the overlay
}

#pragma endregion

#pragma region IContextMenu

IFACEMETHODIMP PellucidHandlers::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
	return E_INVALIDARG;	// We don't have anything to say here
}

// IMPORTANT: We add our menu here
// Menu comes from resource
IFACEMETHODIMP PellucidHandlers::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
	// Do nothing if 'CMF_DEFAULTONLY' flag is set
	// NOTE: If 'CMF_EXPLORE' flag is set, it means user right clicked
	//		 on a Explorer window opened at Desktop folder. We are not
	//		 interested in showing menu here. We only show menu when the
	//		 user right clicks on the actual desktop.
	if ((uFlags & CMF_DEFAULTONLY) || (uFlags & CMF_EXPLORE))
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);

	// Load submenu from resource
	auto hSubMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_PELLUCIDICONSMENU));
	if (!hSubMenu)
		return E_UNEXPECTED;

	hSubMenu = GetSubMenu(hSubMenu, 0);	// We need to get the sub menu of the main root
	if (!hSubMenu)
		return E_UNEXPECTED;

	HRESULT hr;

	// NOTE: We don't need the variable 'idCmdLast' and so will reuse to keep track
	//		 of the number of items added to the menu. This is 'cause we will need to return a count + 1.
	idCmdLast = idCmdFirst;

	// Get current settings
	auto isEnabled = Settings::getIsEnabled();
	auto inSetting = Settings::getInSetting();
	auto restoreWhenSetting = Settings::getRestoreWhenSetting();
	auto toSetting = Settings::getToSetting();

	// Before we add sub menu, we will need to set proper menu item id starting from 'idCmdFirst'
	MENUITEMINFO menuItemInfo = { 0 };
	menuItemInfo.cbSize = sizeof(menuItemInfo);

	// Set radiocheck and command ID for menu items
	// 'are Enabled' submenu item
	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::Are_Enabled; // Keep track of command offset and respective handler's function pointer
	menuItemInfo.fMask = MIIM_ID | MIIM_STATE;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (isEnabled ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_PELLUCIDICONS_AREENABLED, FALSE, &menuItemInfo);

	menuItemInfo.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STATE;
	menuItemInfo.fType = MFT_RADIOCHECK;

	// 'In' submenu items
	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::In_5secs;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (inSetting == Settings::In::secs5 ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_IN_5SECS, FALSE, &menuItemInfo);

	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::In_10secs;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (inSetting == Settings::In::secs10 ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_IN_10SECS, FALSE, &menuItemInfo);

	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::In_20secs;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (inSetting == Settings::In::secs20 ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_IN_20SECS, FALSE, &menuItemInfo);

	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::In_30secs;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (inSetting == Settings::In::secs30 ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_IN_30SECS, FALSE, &menuItemInfo);

	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::In_1min;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (inSetting == Settings::In::min1 ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_IN_1MIN, FALSE, &menuItemInfo);

	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::In_2mins;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (inSetting == Settings::In::mins2 ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_IN_2MINS, FALSE, &menuItemInfo);

	// 'Get restored when' submenu items
	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::Get_restoredwhen_mousemoved;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (restoreWhenSetting == Settings::RestoreWhen::mousedMoved ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_GET_RESTORED_WHEN_MOUSEMOVED, FALSE, &menuItemInfo);

	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::Get_restoredwhen_mouseenterquarterregiononleft;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (restoreWhenSetting == Settings::RestoreWhen::mousedEntersQuarterRegionOnLeft ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_GET_RESTORED_WHEN_MOUSEENTERSQUARTERREGIONONLEFT, FALSE, &menuItemInfo);

	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::Get_restoredwhen_doubleclicked;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (restoreWhenSetting == Settings::RestoreWhen::doubleClicked ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_GET_RESTORED_WHEN_DOUBLECLICKED, FALSE, &menuItemInfo);

	// To submenu items
	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::To_fulltransparency;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (toSetting == Settings::To::fullTransparency ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_TO_FULLTRANSPARENCY, FALSE, &menuItemInfo);

	m_mapHandlers[idCmdLast - idCmdFirst] = &PellucidHandlers::To_semitransparency;
	menuItemInfo.wID = idCmdLast++;
	menuItemInfo.fState = (toSetting == Settings::To::semiTransparency ? MFS_CHECKED : MFS_UNCHECKED);
	SetMenuItemInfo(hSubMenu, ID_TO_SEMITRANSPARENCY, FALSE, &menuItemInfo);

	// Add our submenu to 'hmenu' from function parameters
	MENUITEMINFO mi = { 0 };
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_STRING | MIIM_SUBMENU | MIIM_BITMAP;
	mi.dwTypeData = L"Pellucid icons";
	mi.hSubMenu = hSubMenu;
	if (!s_hbitmapPellucidIcon)	// Create bitmap just once
	{
		auto hiconPeullicdIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_PELLUCIDICONSICON), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
		if (hiconPeullicdIcon)
		{
			s_hbitmapPellucidIcon = Utility::ConvertIconHandleToBitmapHandle(hiconPeullicdIcon);	// Convert icon to premultiplied bitmap handle
			DestroyIcon(hiconPeullicdIcon);
		}
	}
	
	mi.hbmpItem = s_hbitmapPellucidIcon;	// NOTE: According to 'SetMenuItemBitmaps()' doc, it is up to this DLL to destroy this bitmap

	hr = (InsertMenuItem(hmenu, indexMenu, TRUE, &mi) != FALSE ? S_OK : E_UNEXPECTED);

	return (SUCCEEDED(hr) ? MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, (idCmdLast - idCmdFirst + 1)) : E_UNEXPECTED);
}

IFACEMETHODIMP PellucidHandlers::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
	// We only need offset of menu item, verb string should not be passed
	if (HIWORD(pici->lpVerb) != NULL)
		return E_FAIL;		// IMPORTANT: According to docs, if we don't handle this menu item offset return 'E_FAIL' so that anothers may handle it

	HRESULT hr;
	int indexMenuItem = LOWORD(pici->lpVerb);

	// Do Action
	if (m_mapHandlers.count(indexMenuItem) == 0)	// NOTE: 'map::count()' is a quick way of finding if given index if among the keys
		return E_FAIL;

	(this->*m_mapHandlers[indexMenuItem])();	// Call associated handler

	return S_OK;
}

#pragma endregion

#pragma region IShellExtInit

IFACEMETHODIMP PellucidHandlers::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
	return S_OK;
}

#pragma endregion

#pragma region Context menu handlers
void PellucidHandlers::In_5secs()
{
	if (Settings::getInSetting() == Settings::In::secs5)
		return;

	Settings::setInSetting(Settings::In::secs5);

	ResetTimer();
}

void PellucidHandlers::In_10secs()
{
	if (Settings::getInSetting() == Settings::In::secs10)
		return;

	Settings::setInSetting(Settings::In::secs10);

	ResetTimer();
}

void PellucidHandlers::In_20secs()
{
	if (Settings::getInSetting() == Settings::In::secs20)
		return;

	Settings::setInSetting(Settings::In::secs20);

	ResetTimer();
}

void PellucidHandlers::In_30secs()
{
	if (Settings::getInSetting() == Settings::In::secs30)
		return;

	Settings::setInSetting(Settings::In::secs30);

	ResetTimer();
}

void PellucidHandlers::In_1min()
{
	if (Settings::getInSetting() == Settings::In::min1)
		return;

	Settings::setInSetting(Settings::In::min1);

	ResetTimer();
}

void PellucidHandlers::In_2mins()
{
	if (Settings::getInSetting() == Settings::In::mins2)
		return;

	Settings::setInSetting(Settings::In::mins2);

	ResetTimer();
}

void PellucidHandlers::Get_restoredwhen_mousemoved()
{
	if (Settings::getRestoreWhenSetting() == Settings::RestoreWhen::mousedMoved)
		return;

	Settings::setRestoreWhenSetting(Settings::RestoreWhen::mousedMoved);

	ResetTimer();
}

void PellucidHandlers::Get_restoredwhen_mouseenterquarterregiononleft()
{
	if (Settings::getRestoreWhenSetting() == Settings::RestoreWhen::mousedEntersQuarterRegionOnLeft)
		return;

	Settings::setRestoreWhenSetting(Settings::RestoreWhen::mousedEntersQuarterRegionOnLeft);

	ResetTimer();
}

void PellucidHandlers::Get_restoredwhen_doubleclicked()
{
	if (Settings::getRestoreWhenSetting() == Settings::RestoreWhen::doubleClicked)
		return;

	Settings::setRestoreWhenSetting(Settings::RestoreWhen::doubleClicked);

	ResetTimer();
}

void PellucidHandlers::To_fulltransparency()
{
	if (Settings::getToSetting() == Settings::To::fullTransparency)
		return;

	Settings::setToSetting(Settings::To::fullTransparency);

	ResetTimer();
}

void PellucidHandlers::To_semitransparency()
{
	if (Settings::getToSetting() == Settings::To::semiTransparency)
		return;

	Settings::setToSetting(Settings::To::semiTransparency);

	ResetTimer();
}

void PellucidHandlers::Are_Enabled()
{
	// Toggle with current setting
	auto toEnable = !Settings::getIsEnabled();	// NOTE: 'toEnable' is new requested setting
	Settings::setIsEnabled(toEnable);

	// Depending on current setting
	if (toEnable == true)
		ResetTimer();
	else
		KillTimer();
}

void PellucidHandlers::KillTimer()
{
	if (s_heventExitThreadFunc)
	{
		if (s_hWaitThread)
		{
			SetEvent(s_heventExitThreadFunc);									// Ask thread to terminate
			WaitForSingleObject(s_hWaitThread, INFINITE);						// Wait for thread to terminate
			UnregisterWait(s_hWaitThread), s_hWaitThread = NULL;				// Close handle
		}
	}

	// Reset window opacity
	SetLayeredWindowAttributes(s_hwndShellWindow, NULL, 0xFF, LWA_ALPHA);
}

void PellucidHandlers::ResetTimer()
{
	KillTimer();

	s_heventExitThreadFunc = CreateEvent(NULL, TRUE, FALSE, NULL);

	auto interval = Settings::convertInToMillisecs(Settings::getInSetting());
	RegisterWaitForSingleObject(&s_hWaitThread,
								s_heventExitThreadFunc,
								&PellucidIconsTimer_ThreadFunc,
								NULL,
								interval,
								WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE);
}

#pragma endregion

VOID CALLBACK PellucidHandlers::PellucidIconsTimer_ThreadFunc(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	if (TimerOrWaitFired)	// If FALSE, exit event has been set so do nothing
	{
		// Timer tick
		auto in = Settings::getInSetting();
		auto restoreWhen = Settings::getRestoreWhenSetting();
		auto to = Settings::getToSetting();

		// For 'RestoreWhen::mousedEntersQuarterRegionOnLeft' setting, if mouse is still under third of the screen
		// don't change icon transparency. Let it be as is.
		if (restoreWhen == Settings::RestoreWhen::mousedEntersQuarterRegionOnLeft &&
			s_bIsMouseOnShellWindow)
		{
			try
			{
				if (s_queueMousePos.back().x <= s_quarterWidthShellWindow)
					return;
			}
			catch (...) {}
		}

		// Change icon opacity depending on 'to' setting
		auto to_transparency = (to == Settings::To::fullTransparency ? 0x00 : 0x5A);	// NOTE: About 35% opacity on semi-transparency setting

		for (int i = 0xFF; i >= to_transparency; i -= 0x0F)
		{
			// CAUTION: We don't let the transparency to be zero because then we will not receive
			//			window events in our window procedure. Hence, the 'max()' below.
			SetLayeredWindowAttributes(s_hwndShellWindow, NULL, (BYTE)max(i, 0x01), LWA_ALPHA);
			Sleep(40);
		}
	}
}

LRESULT CALLBACK PellucidHandlers::ShellWindow_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto isEnabled = Settings::getIsEnabled();
	
	if (isEnabled)
	{
		switch (uMsg)
		{
			case WM_MOUSEMOVE:
			{
				s_bIsMouseOnShellWindow = true;	// Mouse entered event
				s_queueMousePos.push(POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });

				auto settingRestoreWhen = Settings::getRestoreWhenSetting();
				switch (settingRestoreWhen)
				{
					case Settings::RestoreWhen::mousedMoved:
					{
						// We have a history of mouse points so we use those to calculate a
						// distance and compare with threshold
						if (s_queueMousePos.size() < 2)
							break;

						auto last_it = s_queueMousePos.cbegin();
						auto it = ++last_it;
						int distance_x = 0, distance_y = 0;
						for (; it != s_queueMousePos.cend(); last_it = it++)
						{
							distance_x += abs(it->x - last_it->x);
							distance_y += abs(it->y - last_it->y);
						}

						const int DISTANCE_THRESHOLD = 50;
						if (distance_x >= DISTANCE_THRESHOLD || distance_y >= DISTANCE_THRESHOLD)
						{
							// Ask timer thread to activate
							ResetTimer();
						}
					}
					break;

					case Settings::RestoreWhen::mousedEntersQuarterRegionOnLeft:
					{
						// Check if mouse entered quarter width for ShellWindow
						if (GET_X_LPARAM(lParam) <= s_quarterWidthShellWindow)
						{
							// Ask timer thread to activate
							ResetTimer();
						}
					}
					break;

					default:
						break;
				}

				// If opacity is set at 0x01, don't let mouse move pass through
				BYTE opacityShellWindow;
				if (GetLayeredWindowAttributes(s_hwndShellWindow, NULL, &opacityShellWindow, NULL) != FALSE)
				{
					if (opacityShellWindow == 0x01)
						return DefWindowProc(s_hwndShellWindow, uMsg, wParam, lParam);
				}
			}
			break;

			case WM_MOUSELEAVE:	// Mouse has left the desktop window and probably on some application window
			{
				s_bIsMouseOnShellWindow = false;
				s_queueMousePos.clear();
			}
			break;

			case WM_LBUTTONDBLCLK:
			{
				auto settingRestoreWhen = Settings::getRestoreWhenSetting();
				if (settingRestoreWhen == Settings::RestoreWhen::doubleClicked)
				{
					bool bCallDefProc = false;
					BYTE opacityShellWindow;
					if (GetLayeredWindowAttributes(s_hwndShellWindow, NULL, &opacityShellWindow, NULL) != FALSE)
					{
						if (opacityShellWindow == 0x01)
							bCallDefProc = true;
					}

					// Ask timer thread to activate
					ResetTimer();

					if (bCallDefProc)
						return DefWindowProc(s_hwndShellWindow, uMsg, wParam, lParam);
				}

				// If opacity is set at 0x01, don't let double click pass through
				/*BYTE opacityShellWindow;
				if (GetLayeredWindowAttributes(s_hwndShellWindow, NULL, &opacityShellWindow, NULL) != FALSE)
				{
					if (opacityShellWindow == 0x01)
						return DefWindowProc(s_hwndShellWindow, uMsg, wParam, lParam);
				}*/
			}
			break;

			case WM_RBUTTONDOWN:
			{
				// User is trying to invoke context menu
				// If opacity is set at 0x01, don't let right click pass through
				BYTE opacityShellWindow;
				if (GetLayeredWindowAttributes(s_hwndShellWindow, NULL, &opacityShellWindow, NULL) != FALSE)
				{
					if (opacityShellWindow == 0x01)
					{
						// NOTE: The following is needed because if anything was selected before the icons
						//		 were transparent, it invokes their context menu. We don't want this.
						if (ListView_GetSelectedCount(s_hwndShellWindow) > 0)
							ListView_SetItemState(s_hwndShellWindow, -1, FALSE, LVIS_SELECTED);

						return DefWindowProc(s_hwndShellWindow, uMsg, wParam, lParam);
					}
				}

				// Ask timer thread to activate
				ResetTimer();
			}
			break;

			case WM_DESTROY:
			{
				// Possibly windows is shutting down, so cleanup
				KillTimer();

				if (s_hbitmapPellucidIcon)
					DeleteBitmap(s_hbitmapPellucidIcon);

				SetWindowLongPtr(s_hwndShellWindow, GWLP_WNDPROC, s_hPrevShellWindowWndProc);
			}
			break;

			default:
				break;
		}
	}

	return CallWindowProc((WNDPROC)s_hPrevShellWindowWndProc, hwnd, uMsg, wParam, lParam);
}