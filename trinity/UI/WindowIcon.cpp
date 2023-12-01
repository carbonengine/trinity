////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"

#ifdef _WIN32

#include "WindowIcon.h"


HICON GetWindowIcon()
{
	HMODULE hModule = GetModuleHandle(0);
	return (HICON)LoadImage(hModule, MAKEINTRESOURCE(101), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
}

#endif