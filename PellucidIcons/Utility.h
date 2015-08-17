#pragma once
#include <Windows.h>


class Utility
{
public:
	static HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp);
	static HBITMAP ConvertIconHandleToBitmapHandle(HICON hIcon);
};