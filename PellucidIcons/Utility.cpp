#include "Utility.h"


// NOTE: The following function comes from TortoiseSVN project
HRESULT Utility::Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp)
{
	if (psize == 0)
		return E_INVALIDARG;

	if (phBmp == 0)
		return E_POINTER;

	*phBmp = NULL;

	BITMAPINFO bmi;
	SecureZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;

	bmi.bmiHeader.biWidth = psize->cx;
	bmi.bmiHeader.biHeight = psize->cy;
	bmi.bmiHeader.biBitCount = 32;

	HDC hdcUsed = hdc ? hdc : GetDC(NULL);
	if (hdcUsed)
	{
		*phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
		if (hdc != hdcUsed)
		{
			ReleaseDC(NULL, hdcUsed);
		}
	}
	return (NULL == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

HBITMAP Utility::ConvertIconHandleToBitmapHandle(HICON hIcon)
{
	HBITMAP hbitmapRet = NULL;

	SIZE sizeIcon;
	sizeIcon.cx = GetSystemMetrics(SM_CXSMICON);
	sizeIcon.cy = GetSystemMetrics(SM_CYSMICON);

	HDC hdcDest = CreateCompatibleDC(NULL);
	if (hdcDest)
	{
		SetBkMode(hdcDest, TRANSPARENT);

		HBITMAP hbitmapDest;
		
		if (SUCCEEDED(Create32BitHBITMAP(hdcDest, &sizeIcon, NULL, &hbitmapDest)))
		{
			auto hbitmapPrevDest = SelectObject(hdcDest, hbitmapDest);

			DrawIconEx(hdcDest, 0, 0, hIcon, sizeIcon.cx, sizeIcon.cy, 0, NULL, DI_NORMAL);
			SelectObject(hdcDest, hbitmapPrevDest);	// Restore DC's original bitmap

			hbitmapRet = hbitmapDest;
		}

		DeleteDC(hdcDest);
	}

	return hbitmapRet;
}
