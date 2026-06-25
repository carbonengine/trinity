// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#ifdef _WIN32
#include "RenderWindow.h"

RenderWindow::RenderWindow( uint32_t width, uint32_t height )
{
	static WNDCLASS wndClass;
	static bool wndClassInitialized = false;
	if( !wndClassInitialized )
	{
		wndClass.style = CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc = DefWindowProc;
		wndClass.cbClsExtra = 0;
		wndClass.cbWndExtra = 0;
		wndClass.hInstance = GetModuleHandle( nullptr );
		wndClass.hIcon = nullptr;
		wndClass.hCursor = nullptr;
		wndClass.hbrBackground = nullptr;
		wndClass.lpszMenuName = nullptr;
		wndClass.lpszClassName = g_moduleName;

		RegisterClass( &wndClass );
		wndClassInitialized = true;
	}
	DWORD style = WS_OVERLAPPEDWINDOW;
	RECT rect = { 0, 0, (LONG)width, (LONG)height };
	::AdjustWindowRect( &rect, style, FALSE );
	m_handle = ::CreateWindow(
		wndClass.lpszClassName,
		g_moduleName,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0L,
		NULL,
		wndClass.hInstance,
		0L );
	::ShowWindow( m_handle, SW_SHOW );
}

RenderWindow::~RenderWindow()
{
	::DestroyWindow( m_handle );
}

uint32_t RenderWindow::GetClientWidth() const
{
	RECT rect = { 0, 0, 0, 0 };
	::GetClientRect( m_handle, &rect );
	return rect.right - rect.left;
}

uint32_t RenderWindow::GetClientHeight() const
{
	RECT rect = { 0, 0, 0, 0 };
	::GetClientRect( m_handle, &rect );
	return rect.bottom - rect.top;
}

bool RenderWindow::Resize( uint32_t width, uint32_t height )
{
	DWORD style = WS_OVERLAPPEDWINDOW;
	RECT rect = { 0, 0, (LONG)width, (LONG)height };
	::AdjustWindowRect( &rect, style, FALSE );
	::SetWindowPos( m_handle, HWND_BOTTOM, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
	return true;
}

#endif