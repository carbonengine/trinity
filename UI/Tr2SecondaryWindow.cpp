////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"

#ifdef _WIN32

#include "Tr2SecondaryWindow.h"
#include "App.h"
#include "WindowIcon.h"
#include <unordered_map>

#if BLUE_WITH_PYTHON
#include "CcpUtils/PyCpp.h"
#endif

extern HINSTANCE gInstance;

namespace
{
	static ATOM s_windowClass = NULL;

	ATOM RegisterWindowClass()
	{
		if( s_windowClass )
		{
			return s_windowClass;
		}

		HICON hIcon = GetWindowIcon();

		WNDCLASSEXW wclass = {sizeof(wclass)};
		wclass.lpfnWndProc = Tr2SecondaryWindow::WndProc;
		wclass.hInstance = gInstance;
		wclass.lpszClassName = L"Tr2SecondaryWindow";
		wclass.hIcon = hIcon;
		s_windowClass = RegisterClassExW(&wclass);
		return s_windowClass;
	}
}

Tr2SecondaryWindow::Tr2SecondaryWindow( IRoot* lockobj /*= nullptr */ ) :
	m_handle( NULL ),
	m_windowTitle( L"EVE Secondary Window" ),
	m_left( 0 ),
	m_top( 0 ),
	m_width( 400 ),
	m_height( 300 ),
	m_minWidth( 400 ),
	m_minHeight( 300 ),
	m_extendedWindowStyle( 0 ),
	m_windowStyle( WS_POPUP | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZEBOX | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX )
{
}

Tr2SecondaryWindow::~Tr2SecondaryWindow()
{
	Close();
}

void Tr2SecondaryWindow::Create()
{
	RECT rc;
	SetRect( &rc, 0, 0, m_width, m_height );
	AdjustWindowRectEx( &rc, m_windowStyle, FALSE, m_extendedWindowStyle );
	OffsetRect( &rc, m_left - rc.left, m_top - rc.top );
	
	m_handle = ::CreateWindowExW(
		m_extendedWindowStyle,
		(LPCWSTR)RegisterWindowClass(),
		m_windowTitle.c_str(),
		m_windowStyle,
		rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		g_app->GetWindowHandle(),
		NULL,
		gInstance,
		0L);

	SetWindowLongPtr( m_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( this ) );
}

LRESULT CALLBACK Tr2SecondaryWindow::WndProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l )
{
	auto obj = reinterpret_cast<Tr2SecondaryWindow*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	
	switch( msg )
	{
	case WM_CLOSE:
		if( obj )
		{
			obj->Close();
			return 0;
		}
		break;

	case WM_GETMINMAXINFO:
		if( obj )
		{
			MINMAXINFO* minMaxInfo = reinterpret_cast<MINMAXINFO*>( l );
			minMaxInfo->ptMinTrackSize.x = obj->m_minWidth;
			minMaxInfo->ptMinTrackSize.y = obj->m_minHeight;

			return 0;
		}
		break;

	}

	if( obj && obj->m_eventHandler )
	{
#if BLUE_WITH_PYTHON
		Ccp::PyGilEnsure gilWrapper;
#endif
		bool handled = false;
		obj->m_eventHandler.Call( handled, msg, w, l );
		if( handled )
		{
			return 0;
		}
	}
	
	return DefWindowProcW( hwnd, msg, w, l );
}

void Tr2SecondaryWindow::Close()
{
	if( m_handle )
	{
		DestroyWindow( m_handle );
		m_handle = NULL;
	}
}

std::wstring Tr2SecondaryWindow::GetTitle()
{
	return m_windowTitle;
}

void Tr2SecondaryWindow::SetTitle( const std::wstring& title )
{
	m_windowTitle = title;
	SetWindowTextW( m_handle, m_windowTitle.c_str() );
}

uint32_t Tr2SecondaryWindow::GetWidth() const
{
	if( m_handle != NULL )
	{
		RECT rect;
		::GetClientRect( m_handle, &rect );
		return rect.right;
	}
	else
	{
		return m_width;
	}
}

void Tr2SecondaryWindow::SetWidth( uint32_t val )
{
	m_width = val;
	if( m_handle != NULL )
	{
		SetWindowDimensions( GetLeft(), GetTop(), m_width, GetHeight() );
	}
}

uint32_t Tr2SecondaryWindow::GetHeight() const
{
	if( m_handle != NULL )
	{
		RECT rect;
		::GetClientRect( m_handle, &rect );
		return rect.bottom;
	}
	else
	{
		return m_height;
	}
}

void Tr2SecondaryWindow::SetHeight( uint32_t val )
{
	m_height = val;
	if( m_handle != NULL )
	{
		SetWindowDimensions( GetLeft(), GetTop(), GetWidth(), m_height );
	}
}

uint32_t Tr2SecondaryWindow::GetLeft() const
{
	if( m_handle != NULL )
	{
		RECT rect;
		::GetWindowRect( m_handle, &rect );

		return rect.left;
	}
	else
	{
		return m_left;
	}
}

void Tr2SecondaryWindow::SetLeft( uint32_t val )
{
	m_left = val;
	if( m_handle != NULL )
	{
		SetWindowDimensions( m_left, GetTop(), GetWidth(), GetHeight() );
	}
}

uint32_t Tr2SecondaryWindow::GetTop() const
{
	if( m_handle != NULL )
	{
		RECT rect;
		::GetWindowRect( m_handle, &rect );
	
		return rect.top;
	}
	else
	{
		return m_top;
	}
}

void Tr2SecondaryWindow::SetTop( uint32_t val )
{
	m_top = val;
	if( m_handle != NULL )
	{
		SetWindowDimensions( GetLeft(), m_top, GetWidth(), GetHeight() );
	}
}

void Tr2SecondaryWindow::SetWindowDimensions( uint32_t left, uint32_t top, uint32_t width, uint32_t height )
{
	RECT rc;
	SetRect( &rc, 0, 0, width, height );
	AdjustWindowRectEx( &rc, m_windowStyle, FALSE, m_extendedWindowStyle );
	OffsetRect( &rc, left - rc.left, top - rc.top );
	::MoveWindow( m_handle, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE );
}

#endif
