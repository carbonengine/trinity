// Copyright © 2020 CCP ehf.

#include "StdAfx.h"

#ifdef _WIN32

#include "Tr2MainWindow.h"
#include "WindowIcon.h"
#include "Tr2MouseCursor.h"
#include "TriDevice.h"


extern CcpMeanStatisticsEntry g_activeFrametimeMean;
extern CcpStdDevStatisticsEntry g_activeFrametimeStdDev;
CCP_STATS_DECLARED_ELSEWHERE( frameTime );

#define USE_PROPER_FIXED_WINDOW 0


namespace
{
struct MonitorRects
{
	std::vector<RECT> rcMonitors;
	RECT rcCombined;

	static BOOL CALLBACK MonitorEnum( HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData )
	{
		MonitorRects* pThis = reinterpret_cast<MonitorRects*>( pData );
		pThis->rcMonitors.push_back( *lprcMonitor );
		UnionRect( &pThis->rcCombined, &pThis->rcCombined, lprcMonitor );
		return TRUE;
	}

	MonitorRects()
	{
		SetRectEmpty( &rcCombined );
		EnumDisplayMonitors( 0, 0, MonitorEnum, (LPARAM)this );
	}
};
}


void Tr2MainWindow::SetWindowTitle( const wchar_t* title )
{
	m_title = title;
	if( m_hwnd )
	{
		::SetWindowTextW( m_hwnd, title );
	}
}

bool Tr2MainWindow::HasFocus() const
{
	return ::GetFocus() == m_hwnd;
}

bool Tr2MainWindow::HasWindow() const
{
	return m_hwnd != nullptr;
}

void Tr2MainWindow::CreateOSWindow( Tr2MainWindowState::State& state )
{
	if( m_hwnd )
	{
		return;
	}

	WNDCLASSEXW wclass = { sizeof( wclass ) };
	wclass.lpfnWndProc = StaticWndProc;
	wclass.hInstance = reinterpret_cast<HINSTANCE>( &__ImageBase );
	wclass.lpszClassName = L"trinityWindow";
	wclass.hIcon = GetWindowIcon();
	auto wndClass = RegisterClassExW( &wclass );

	auto width = LONG( state.width );
	auto height = LONG( state.height );
#if USE_BORDERLESS_WINDOW
	if( state.windowMode == Tr2WindowMode::FULL_SCREEN )
	{
		auto monitorRect = GetMonitorRect( state.adapter );
		auto monitorWidth = monitorRect.right - monitorRect.left;
		auto monitorHeight = monitorRect.bottom - monitorRect.top;
		state.left = monitorRect.left;
		state.top = monitorRect.top;
		width = monitorWidth;
		height = monitorHeight;
	}
#endif
	RECT rect = { 0, 0, width, height };
	auto style = GetWindowStyle( state.windowMode );
	::AdjustWindowRect( &rect, style, FALSE );

	if( state.windowMode == Tr2WindowMode::WINDOWED && state.showState == Tr2WindowShowState::MAXIMIZED )
	{
		style |= WS_MAXIMIZE;
	}
	m_hwnd = ::CreateWindowW(
		(LPWSTR)wndClass,
		m_title.c_str(),
		style,
		state.left,
		state.top,
		( rect.right - rect.left ),
		( rect.bottom - rect.top ),
		0L,
		NULL,
		wclass.hInstance,
		this );
	if( state.windowMode == Tr2WindowMode::WINDOWED && m_hwnd )
	{
		RECT clientRect = {};
		if( GetClientRect( m_hwnd, &clientRect ) )
		{
			auto width = uint32_t( clientRect.right - clientRect.left );
			auto height = uint32_t( clientRect.bottom - clientRect.top );
			if( width > 0 && height > 0 )
			{
				state.width = width;
				state.height = height;
			}
		}
	}
	SetPropW( m_hwnd, L"Tr2MainWindow", reinterpret_cast<HANDLE>( this ) );
}

void Tr2MainWindow::DestroyOSWindow()
{
	if( m_hwnd )
	{
		::DestroyWindow( m_hwnd );
		m_hwnd = nullptr;
	}
}

DWORD Tr2MainWindow::GetWindowStyle( Tr2WindowMode::Type windowMode ) const
{
	switch( windowMode )
	{
	case Tr2WindowMode::WINDOWED:
		return WS_POPUP | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZEBOX | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX;
	default:
		return WS_POPUP | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZEBOX;
	}
}

void Tr2MainWindow::AdjustWindow( Tr2MainWindowState::State& state )
{
	auto newStyle = GetWindowStyle( state.windowMode );
	auto oldStyle = GetWindowStyle( m_state.windowMode );
	if( newStyle != oldStyle )
	{
		SetWindowLongW( m_hwnd, GWL_STYLE, GetWindowStyle( state.windowMode ) );
	}

	auto width = LONG( state.width );
	auto height = LONG( state.height );
#if USE_BORDERLESS_WINDOW
	if( state.windowMode == Tr2WindowMode::FULL_SCREEN )
	{
		auto monitorRect = GetMonitorRect( state.adapter );
		auto monitorWidth = monitorRect.right - monitorRect.left;
		auto monitorHeight = monitorRect.bottom - monitorRect.top;
		state.left = monitorRect.left;
		state.top = monitorRect.top;
		width = monitorWidth;
		height = monitorHeight;
	}
#endif
	RECT rect = { 0, 0, width, height };
	::AdjustWindowRect( &rect, newStyle, FALSE );
	::SetWindowPos( m_hwnd, HWND_NOTOPMOST, state.left, state.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE );
	if( state.showState == Tr2WindowShowState::MAXIMIZED )
	{
		::ShowWindow( m_hwnd, SW_MAXIMIZE );
	}
}

Tr2MainWindow::Rect Tr2MainWindow::GetDesktopRect() const
{
	MonitorRects rects;
	Rect rect = { rects.rcCombined.left, rects.rcCombined.top, rects.rcCombined.right, rects.rcCombined.bottom };
	return rect;
}

Tr2MainWindow::Rect Tr2MainWindow::GetMonitorRect( uint32_t adapter ) const
{
	Rect rect = {};
	HMONITOR hMonitor = nullptr;
	if( SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterMonitor( adapter, (void*&)hMonitor ) ) )
	{
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof( MONITORINFO );

		GetMonitorInfo( hMonitor, &monitorInfo );
		rect = { monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right, monitorInfo.rcMonitor.bottom };
	}
	return rect;
}

Tr2MainWindow::Size Tr2MainWindow::GetWindowSize( const Size& clientSize, Tr2WindowMode::Type mode ) const
{
	RECT rect = { 0, 0, LONG( clientSize.width ), LONG( clientSize.height ) };
	::AdjustWindowRect( &rect, GetWindowStyle( mode ), FALSE );

	Size size;
	size.width = int32_t( rect.right - rect.left );
	size.height = int32_t( rect.bottom - rect.top );
	return size;
}

Tr2MainWindow::Size Tr2MainWindow::GetClientSize( const Size& windowSize, Tr2WindowMode::Type mode ) const
{
	RECT rc;
	SetRectEmpty( &rc );
	::AdjustWindowRect( &rc, GetWindowStyle( mode ), FALSE );

	Size size;
	size.width = int32_t( windowSize.width - rc.right + rc.left );
	size.height = int32_t( windowSize.height - rc.bottom + rc.top );
	return size;
}

bool Tr2MainWindow::SupportsFullscreen() const
{
	return GetSystemMetrics( SM_REMOTESESSION ) == 0;
}

void Tr2MainWindow::ClipCursor( int32_t left, int32_t top, int32_t right, int32_t bottom )
{
	if( !m_hwnd )
	{
		return;
	}

	RECT rect;
	::GetClientRect( m_hwnd, &rect );

	RECT rc = {
		LONG( double( left ) / m_state.width * ( rect.right - rect.left ) + 0.5 ),
		LONG( double( top ) / m_state.height * ( rect.bottom - rect.top ) + 0.5 ),
		LONG( double( right ) / m_state.width * ( rect.right - rect.left ) + 0.5 ),
		LONG( double( bottom ) / m_state.height * ( rect.bottom - rect.top ) + 0.5 )
	};

	POINT offs = { 0, 0 };
	ClientToScreen( m_hwnd, &offs );
	OffsetRect( &rc, offs.x, offs.y );

	::ClipCursor( &rc );
}

void Tr2MainWindow::UnclipCursor()
{
	::ClipCursor( nullptr );
}

void Tr2MainWindow::SetCursorPos( int32_t x, int32_t y )
{
	if( !m_hwnd )
	{
		return;
	}

	RECT rect;
	::GetClientRect( m_hwnd, &rect );

	POINT offs = { 0, 0 };
	ClientToScreen( m_hwnd, &offs );
	::SetCursorPos(
		LONG( double( x ) / m_state.width * ( rect.right - rect.left ) + 0.5 ) + offs.x,
		LONG( double( y ) / m_state.height * ( rect.bottom - rect.top ) + 0.5 ) + offs.y );
}

std::pair<int32_t, int32_t> Tr2MainWindow::GetCursorPos() const
{
	if( !m_hwnd )
	{
		return { 0, 0 };
	}
	POINT pt;
	::GetCursorPos( &pt );
	::ScreenToClient( m_hwnd, &pt );

	RECT rect;
	::GetClientRect( m_hwnd, &rect );
	return {
		int32_t( double( pt.x ) / ( rect.right - rect.left ) * m_state.width + 0.5 ),
		int32_t( double( pt.y ) / ( rect.bottom - rect.top ) * m_state.height + 0.5 )
	};
}

bool Tr2MainWindow::IsKeyToggled( uint32_t keyCode ) const
{
	return ( GetKeyState( keyCode ) & 1 ) ? 1 : 0;
}

bool Tr2MainWindow::IsKeyPressed( uint32_t keyCode ) const
{
	return ( GetKeyState( keyCode ) & 0x8000 ) ? 1 : 0;
}

std::string Tr2MainWindow::GetKeyName( uint32_t keyCode ) const
{
	auto scanCode = ::MapVirtualKeyW( keyCode, MAPVK_VK_TO_VSC ) << 16;

	// Set the extended keyboard bit for the following virtual keys
	switch( keyCode )
	{
	case VK_LEFT:
	case VK_UP:
	case VK_RIGHT:
	case VK_DOWN:
	case VK_PRIOR:
	case VK_NEXT:
	case VK_END:
	case VK_HOME:
	case VK_INSERT:
	case VK_DELETE:
	case VK_DIVIDE:
	case VK_NUMLOCK: {
		scanCode |= ( 1 << 24 );
	}
	}

	// Make no distinction between left and right control keys
	scanCode |= ( 1 << 25 );

	const int bufferSize = 64;
	wchar_t buffer[bufferSize];

	// Lookup the key name and return true if it succeeds
	if( ::GetKeyNameTextW( scanCode, buffer, bufferSize ) )
	{
		return WideToUTF8( buffer );
	}
	return "";
}


bool Tr2MainWindow::ProcessMessages()
{
	MSG msg;
	bool returnValue = true;
	while( PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ) )
	{
		if( msg.message == WM_QUIT )
		{
			returnValue = false;
		}
		TranslateMessage( &msg );
		DispatchMessageW( &msg );
	}

	return returnValue;
}



LRESULT Tr2MainWindow::StaticWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2MainWindow* window = nullptr;
	if( msg == WM_CREATE )
	{
		window = reinterpret_cast<Tr2MainWindow*>( reinterpret_cast<CREATESTRUCTW*>( lParam )->lpCreateParams );
	}
	else
	{
		window = reinterpret_cast<Tr2MainWindow*>( GetPropW( hwnd, L"Tr2MainWindow" ) );
	}

	if( window )
	{
		return window->WndProc( hwnd, msg, wParam, lParam );
	}
	else
	{
		return DefWindowProcW( hwnd, msg, wParam, lParam );
	}
}

LRESULT Tr2MainWindow::WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	std::pair<int32_t, bool> returnValue = { 0, false };

	int32_t mouseX, mouseY;

	auto mouseCoords = [&]() {
		RECT rect;
		::GetClientRect( m_hwnd, &rect );
		mouseX = int32_t( double( int32_t( lParam & 0xffff ) ) / ( rect.right - rect.left ) * m_state.width + 0.5 );
		mouseY = int32_t( double( int32_t( lParam >> 16 ) ) / ( rect.bottom - rect.top ) * m_state.height + 0.5 );
	};

	switch( msg )
	{
	case WM_LBUTTONDOWN:
		::SetCapture( hwnd );
		mouseCoords();
		m_onMouseDown.CallVoid( int32_t( 0 ), mouseX, mouseY );
		returnValue = std::make_pair( 0, m_onMouseDown.IsValid() );
		break;
	case WM_LBUTTONUP:
		if( !::GetAsyncKeyState( VK_RBUTTON ) )
		{
			::ReleaseCapture();
		}
		mouseCoords();
		m_onMouseUp.CallVoid( int32_t( 0 ), mouseX, mouseY );
		returnValue = std::make_pair( 0, m_onMouseUp.IsValid() );
		break;
	case WM_RBUTTONDOWN:
		::SetCapture( hwnd );
		mouseCoords();
		m_onMouseDown.CallVoid( int32_t( 1 ), mouseX, mouseY );
		returnValue = std::make_pair( 0, m_onMouseDown.IsValid() );
		break;
	case WM_RBUTTONUP:
		if( !::GetAsyncKeyState( VK_LBUTTON ) )
		{
			::ReleaseCapture();
		}
		mouseCoords();
		m_onMouseUp.CallVoid( int32_t( 1 ), mouseX, mouseY );
		returnValue = std::make_pair( 0, m_onMouseUp.IsValid() );
		break;
	case WM_MBUTTONDOWN:
		mouseCoords();
		m_onMouseDown.CallVoid( int32_t( 2 ), mouseX, mouseY );
		returnValue = std::make_pair( 0, m_onMouseDown.IsValid() );
		break;
	case WM_MBUTTONUP:
		mouseCoords();
		m_onMouseUp.CallVoid( int32_t( 2 ), mouseX, mouseY );
		returnValue = std::make_pair( 0, m_onMouseUp.IsValid() );
		break;
	case WM_XBUTTONDOWN:
		mouseCoords();
		m_onMouseDown.CallVoid( int32_t( ( wParam & 0x10000 ) ? 3 : 4 ), mouseX, mouseY );
		returnValue = std::make_pair( 0, m_onMouseDown.IsValid() );
		break;
	case WM_XBUTTONUP:
		mouseCoords();
		m_onMouseUp.CallVoid( int32_t( ( wParam & 0x10000 ) ? 3 : 4 ), mouseX, mouseY );
		returnValue = std::make_pair( 0, m_onMouseUp.IsValid() );
		break;
	case WM_MOUSEWHEEL:
		m_onMouseWheel.CallVoid( int32_t( GET_WHEEL_DELTA_WPARAM( wParam ) ) );
		returnValue = std::make_pair( 0, m_onMouseWheel.IsValid() );
		break;
	case WM_MOUSEMOVE:
		mouseCoords();
		m_onMouseMove.CallVoid( mouseX, mouseY ).ReportException();
		returnValue = std::make_pair( 0, m_onMouseMove.IsValid() );
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		m_onKeyDown.CallVoid( int32_t( wParam ), (int32_t)lParam );
		returnValue = std::make_pair( 0, m_onKeyDown.IsValid() );
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		m_onKeyUp.CallVoid( int32_t( wParam ), (int32_t)lParam );
		returnValue = std::make_pair( 0, m_onKeyUp.IsValid() );
		break;
	case WM_CHAR:
		m_onChar.CallVoid( int32_t( wParam ), (int32_t)lParam, false );
		returnValue = std::make_pair( 0, m_onChar.IsValid() );
		break;
	case WM_DEADCHAR:
		m_onChar.CallVoid( int32_t( wParam ), (int32_t)lParam, true );
		returnValue = std::make_pair( 0, m_onChar.IsValid() );
		break;

	case WM_GETMINMAXINFO: {
		auto size = GetWindowSize( m_minimumSize, m_state.windowMode );
		( (MINMAXINFO*)lParam )->ptMinTrackSize.x = size.width;
		( (MINMAXINFO*)lParam )->ptMinTrackSize.y = size.height;
		break;
	}
	case WM_ENTERSIZEMOVE:
		m_isResizing = true;
		break;
	case WM_EXITSIZEMOVE:
		m_isResizing = false;
		if( m_state.windowMode == Tr2WindowMode::WINDOWED )
		{
			RECT rect;
			::GetClientRect( hwnd, &rect );
			auto prevWidth = m_state.width;
			auto prevHeight = m_state.height;
			auto newMode = m_state;
			newMode.width = uint32_t( rect.right - rect.left );
			newMode.height = uint32_t( rect.bottom - rect.top );

			::GetWindowRect( hwnd, &rect );
			newMode.left = int32_t( rect.left );
			newMode.top = int32_t( rect.top );

			WINDOWPLACEMENT placement = { sizeof( WINDOWPLACEMENT ) };
			GetWindowPlacement( hwnd, &placement );
			if( placement.showCmd == SW_MAXIMIZE )
			{
				newMode.showState = Tr2WindowShowState::MAXIMIZED;
			}
			else if( placement.showCmd == SW_MINIMIZE )
			{
				newMode.showState = Tr2WindowShowState::MINIMIZED;
				newMode.width = prevWidth;
				newMode.height = prevHeight;
			}
			else
			{
				newMode.showState = Tr2WindowShowState::NORMAL;
			}

			SetState( false, newMode );
		}
		break;
	case WM_SIZE:
		if( !m_isResizing )
		{
			DefWindowProcW( hwnd, msg, wParam, lParam );

			RECT rect;
			::GetClientRect( hwnd, &rect );
			auto prevWidth = m_state.width;
			auto prevHeight = m_state.height;
			auto newMode = m_state;
#if USE_BORDERLESS_WINDOW
			if( newMode.windowMode != Tr2WindowMode::FULL_SCREEN )
#endif
			{
				newMode.width = uint32_t( rect.right - rect.left );
				newMode.height = uint32_t( rect.bottom - rect.top );
			}
			if( wParam == SIZE_MAXIMIZED )
			{
				newMode.showState = Tr2WindowShowState::MAXIMIZED;
				gTriDev->SetThrottling( TriDevice::WINDOW_HIDDEN, false );
			}
			else if( wParam == SIZE_MINIMIZED )
			{
				newMode.showState = Tr2WindowShowState::MINIMIZED;
				newMode.width = prevWidth;
				newMode.height = prevHeight;
				gTriDev->SetThrottling( TriDevice::WINDOW_HIDDEN, true );
			}
			else
			{
				newMode.showState = Tr2WindowShowState::NORMAL;
				gTriDev->SetThrottling( TriDevice::WINDOW_HIDDEN, false );
			}

			if( !g_renderContextIsBeingDestroyed )
			{
				SetState( false, newMode );
			}

			std::pair<int32_t, bool> ret;
			m_onWindowsMessage.Call( ret, msg, wParam, lParam ).ReportException();
			return 1;
		}
		break;
	case WM_SETCURSOR: {
		bool useWindowsCursor = false;
		if( m_state.windowMode == Tr2WindowMode::WINDOWED )
		{
			POINT mouse;
			::GetCursorPos( &mouse );

			RECT client;
			POINT clientPosition = { 0, 0 };
			::GetClientRect( m_hwnd, &client );
			ClientToScreen( m_hwnd, &clientPosition );
			OffsetRect( &client, clientPosition.x, clientPosition.y );

			useWindowsCursor = !PtInRect( &client, mouse );
		}

		// use D3D or Windows cursor?
		if( !useWindowsCursor )
		{
			if( m_cursor )
			{
				m_cursor->Apply();
			}
			return TRUE; // prevent Windows from setting cursor to window class cursor
		}
	}
	break;


	case WM_NCHITTEST:
		if( m_state.windowMode != Tr2WindowMode::WINDOWED )
		{
			// Prevent the user from selecting the menu in fullscreen mode
			return HTCLIENT;
		}
		break;
	case WM_ACTIVATE:
		if( wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE )
		{
			if( gTriDev )
			{
				gTriDev->ApplicationActivated( TriDevice::APP_ACTIVATED );
				gTriDev->SetThrottling( TriDevice::WINDOW_OUT_OF_FOCUS, false );
			}
			g_activeFrametimeMean.SetSource( &g_ccpStatistics_frameTime );
			g_activeFrametimeStdDev.SetSource( &g_ccpStatistics_frameTime );
			if( m_state.windowMode == Tr2WindowMode::FIXED_WINDOW )
			{
				SetWindowPos( m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
			}
		}
		else
		{
			::ClipCursor( NULL );
			if( gTriDev )
			{
				gTriDev->ApplicationActivated( TriDevice::APP_DEACTIVATED );
				gTriDev->SetThrottling( TriDevice::WINDOW_OUT_OF_FOCUS, true );
			}
			g_activeFrametimeMean.SetSource( nullptr );
			g_activeFrametimeStdDev.SetSource( nullptr );
		}
		m_onFocusChange.CallVoid( wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE );
		returnValue = std::make_pair( 0, m_onFocusChange.IsValid() );
		break;
	case WM_SYSCOMMAND:
		if( wParam == SC_KEYMENU )
		{
			return 0; //catch all Alt-something combination messages.  Specifically alt enter.
		}
		break;
	case WM_CLOSE: {
		bool canClose = true;
		m_onClose.Call( canClose );
		returnValue = std::make_pair( canClose ? 0 : 1, m_onClose.IsValid() );
		break;
	}
	}

	if( !m_hasMessageFilter || m_messageFilter.find( msg ) != end( m_messageFilter ) )
	{
		std::pair<int32_t, bool> ret = { 0, false };
		m_onWindowsMessage.Call( ret, msg, wParam, lParam );
		if( ret.second )
		{
			returnValue = ret;
		}
	}

	if( !returnValue.second )
	{
		return DefWindowProcW( hwnd, msg, wParam, lParam );
	}
	else
	{
		return returnValue.first;
	}
}

uintptr_t Tr2MainWindow::GetHwnd() const
{
	return uintptr_t( m_hwnd );
}

std::pair<bool, std::vector<UINT>> Tr2MainWindow::GetWindowsMessageFilter() const
{
	std::pair<bool, std::vector<UINT>> result;
	result.first = m_hasMessageFilter;
	result.second.insert( end( result.second ), begin( m_messageFilter ), end( m_messageFilter ) );
	return result;
}

void Tr2MainWindow::SetWindowsMessageFilter( bool enabled, const std::vector<UINT>& filter )
{
	m_hasMessageFilter = enabled;
	m_messageFilter.clear();
	m_messageFilter.insert( begin( filter ), end( filter ) );
}

void Tr2MainWindow::OnTick( Be::Time, Be::Time, void* )
{
}

void Tr2MainWindow::SanitizeWindowedResolution( Tr2MainWindowState::State& state ) const
{
	if( state.width == 0 && state.height == 0 )
	{
		// use window size that spans default display, adjust for window frame
		Tr2DisplayModeInfo mi;
		Tr2VideoAdapterInfo::GetAdapterDisplayMode( state.adapter, mi );

		Size size = { mi.width, mi.height };
		size = GetClientSize( size, state.windowMode );

		state.width = size.width;
		state.height = size.height;
	}
	else
	{
		// clamp size between window min size and desktop size, adjusted by window frame
		state.width = std::max( m_minimumSize.width, state.width );
		state.height = std::max( m_minimumSize.height, state.height );

		auto desktop = GetDesktopRect();
		Size desktopSize;
		desktopSize.width = uint32_t( desktop.right - desktop.left );
		desktopSize.height = uint32_t( desktop.bottom - desktop.top );

		state.width = std::min( desktopSize.width, state.width );
		state.height = std::min( desktopSize.height, state.height );
	}
}

void Tr2MainWindow::SanitizeWindowPosition( Tr2MainWindowState::State& state ) const
{
	if( state.windowMode == Tr2WindowMode::FIXED_WINDOW )
	{
		// fixed windows are always at the top left corner of the display
		auto rect = GetMonitorRect( state.adapter );
		state.left = rect.left;
		state.top = rect.top;
	}
	else
	{
		// window should intersect desktop with some margin
		Size size = { state.width, state.height };
		size = GetWindowSize( size, state.windowMode );

		auto margin = int32_t( std::max( std::min( size.width / 2, size.height / 2 ), 50u ) );

		auto desktop = GetDesktopRect();

		state.left = std::max( state.left, int32_t( desktop.left - size.width + margin ) );
		state.top = std::max( state.top, int32_t( desktop.top - size.height + margin ) );
		state.left = std::min( state.left, int32_t( desktop.right - margin ) );
		state.top = std::min( state.top, int32_t( desktop.bottom - margin ) );
	}
}

Tr2MainWindow::Size Tr2MainWindow::GetLargestWindowSize( uint32_t adapter, Tr2WindowMode::Type mode ) const
{
	auto desktopRect = GetDesktopRect();
	return { uint32_t( desktopRect.right - desktopRect.left ), uint32_t( desktopRect.bottom - desktopRect.top ) };
}

Tr2WindowHandle Tr2MainWindow::GetOutputWindow() const
{
	return m_hwnd;
}

#endif