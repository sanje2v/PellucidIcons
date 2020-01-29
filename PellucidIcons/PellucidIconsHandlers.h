#pragma once

#include "sized_queue.h"
#include <windows.h>
#include <shlobj.h>
#include <map>
#include <utility>


class PellucidHandlers : public IShellIconOverlayIdentifier, public IContextMenu, public IShellExtInit
{
public:
	PellucidHandlers(void);

	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	// IShellIconOverlayIdentifier
	IFACEMETHODIMP GetOverlayInfo(PWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
	IFACEMETHODIMP GetPriority(int *pPriority);
	IFACEMETHODIMP IsMemberOf(PCWSTR pwszPath, DWORD dwAttrib);

	// IContextMenu
	IFACEMETHODIMP GetCommandString(UINT_PTR idCmd,	UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax);
	IFACEMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	IFACEMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);

	// IShellExtInit
	IFACEMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

protected:
	~PellucidHandlers(void);

private:
	// Instance variable
	long m_cRef;					// Reference count of component

	// Mapping from menu item index to associated handler
	std::map<int, void(PellucidHandlers::*)()> m_mapHandlers;

	// Functions to handle user selections
	void In_5secs();
	void In_10secs();
	void In_20secs();
	void In_30secs();
	void In_1min();
	void In_2mins();

	void Get_restoredwhen_mousemoved();
	void Get_restoredwhen_mouseenterquarterregiononleft();
	void Get_restoredwhen_doubleclicked();

	void To_fulltransparency();
	void To_semitransparency();

	void Are_Enabled();		// Enable/Disable this extension

	// Utility functions
	static void KillTimer();
	static void ResetTimer();

	// Static variables
	static bool s_bIsMouseOnShellWindow;
	static sized_queue<POINT> s_queueMousePos;
	static bool s_bPellucidIcons;
	static HBITMAP s_hbitmapPellucidIcon;	// Application icon bitmap handle
	static LONG s_quarterWidthShellWindow;
	static HANDLE s_heventExitThreadFunc;
	static HANDLE s_hWaitThread;
	static HWND s_hwndShellWindow;
	static LONG_PTR s_hPrevShellWindowWndProc;

	// Hook for mouse procedure
	static LRESULT CALLBACK ShellWindow_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static VOID CALLBACK PellucidIconsTimer_ThreadFunc(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
};