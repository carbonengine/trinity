#include "StdAfx.h"

#include "App.h"
#include "Tr2MouseCursor.h"
#include "../TriDevice.h"

static CcpLogChannel_t s_appChannel = CCP_LOG_DEFINE_CHANNEL( "App" );

#ifdef _WIN32
#if BLUE_WITH_PYTHON
#include "CcpUtils/PyCpp.h"
#endif
extern bool isWinNT;

LRESULT App::WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
#if BLUE_WITH_PYTHON
	Ccp::PyGilEnsure gilWrapper;
#endif
	switch(msg)
	{
	case WM_LBUTTONDOWN:
		{
			RECT rc;
			::GetClientRect(hwnd, &rc);
			POINT offs = {0};
			ClientToScreen(hwnd, &offs);
			OffsetRect(&rc, offs.x, offs.y);
			::ClipCursor(&rc);
		}
		break;

	case WM_LBUTTONUP:
		::ClipCursor(NULL);
		break;

    case WM_GETMINMAXINFO:
        ((MINMAXINFO*)l)->ptMinTrackSize = reinterpret_cast<const POINT&>( GetMinBounds() );
		((MINMAXINFO*)l)->ptMaxTrackSize = reinterpret_cast<const POINT&>( GetMaxBounds() );
        break;

    case WM_ENTERSIZEMOVE:
		mIsResizing = true;
        break;

    case WM_EXITSIZEMOVE:
		mIsResizing = false;
        if( mResizeEventListener )
        {
		    mSendResizeEvent = true;
        }
        break;

    case WM_SIZE:
        // Check to see if we are losing our window...
		mActive = true;
        if (SIZE_MAXHIDE == w || SIZE_MINIMIZED == w)
		{
            mActive = false;
		}
        else if( ( w == SIZE_RESTORED || w == SIZE_MAXIMIZED ) && !mIsResizing && !mFixedWindow )
		{
            // Widnows will sometimes resize our window(locking computer etc.) which means we
            // we need to send a resize event.
			mSendResizeEvent = true;
		}
		mIsHidden = !mActive;
        break;

    case WM_SETCURSOR:
        {
			bool useWindowsCursor = false;
			if( mWindowed )
			{
				POINT mouse;
				GetCursorPos(&mouse);

				RECT client;
				POINT clientPosition = {0, 0};
				::GetClientRect( mHwnd, &client );
				ClientToScreen( mHwnd, &clientPosition );
				OffsetRect( &client, clientPosition.x, clientPosition.y );

				useWindowsCursor = !PtInRect( &client, mouse );
			}

			// use D3D or Windows cursor?
			if( !useWindowsCursor )
			{
#if TRINITY_PLATFORM == TRINITY_DIRECTX9
				USE_MAIN_THREAD_RENDER_CONTEXT();
				if( renderContext.m_d3dDevice9 ) 
				{
					SetCursor( NULL );
					renderContext.m_d3dDevice9->ShowCursor( TRUE );
				}
#else
				if( m_cursor )
				{
					m_cursor->Apply();
				}
#endif
				return TRUE; // prevent Windows from setting cursor to window class cursor
			}
        }
        break;


    case WM_NCHITTEST:
        // Prevent the user from selecting the menu in fullscreen mode
        if (!mWindowed)
		{
            return HTCLIENT;
		}
		else
		{
			if (mHideTitle)
			{
				POINT pt = {LOWORD(l), HIWORD(l)};
				ScreenToClient(mHwnd, &pt);
				if (pt.x < 620 && pt.y < 30)
					return HTCAPTION;
				else
					return HTCLIENT;
			}
		}

        break;

	case WM_ACTIVATE:		
		if (w == WA_ACTIVE || w == WA_CLICKACTIVE)
		{
			mActive = true;
			if( gTriDev )
			{
				gTriDev->ApplicationActivated( TriDevice::APP_ACTIVATED );
			}
		}
		else
		{
			::ClipCursor(NULL);
			mActive = false;
			if( gTriDev )
			{
				gTriDev->ApplicationActivated( TriDevice::APP_DEACTIVATED );
			}
		}

		if (mReady && BeOS)
		{
			BeOS->GetInfo()->mSleepTime = mActive ? 0 : 1;
		}
		break;

	case WM_DESTROY:
		mHwnd = NULL;
		Destroy();
		break;

	case WM_DISPLAYCHANGE:
		break;

	case WM_SYSCOMMAND:
		if (w == SC_KEYMENU)
			return 0;	//catch all Alt-something combination messages.  Specifically alt enter.
		break;

#if BLUE_WITH_PYTHON
	case WM_WTSSESSION_CHANGE:
		if( m_sessionChangeHandler )
		{			
			PyObject* ret = PyOS->CallMethodWithTrap(m_sessionChangeHandler, 0, "DoSessionChange", "(ii)", w, l);
			LRESULT lRes = 0L;
			bool bHandled = ret && ret != Py_None;
			if (bHandled) lRes = PyInt_AS_LONG(ret);

			Py_XDECREF(ret);
			if (!ret) PyErr_Print();
			if (bHandled) return lRes;
		}
		break;
#endif

	}

    long lRes = 0;
    if( CallEventHandler( msg, w, l, lRes ) )
    {
        return lRes;
    }

	if( isWinNT )
	{
		return DefWindowProcW(hwnd, msg, w, l);
	}
	else
	{
		return DefWindowProcA(hwnd, msg, w, l);
	}
}

// -------------------------------------------------------------
// Description:
//   Calculates the actual minimum window rect size using the 
//   object's minimum width and height values.
// Return Value:
//   The actual minimum size bounds of the window
// -------------------------------------------------------------
Point App::GetMinBounds()
{
	int xSizeFrame = GetSystemMetrics( SM_CXSIZEFRAME );
	int ySizeFrame = GetSystemMetrics( SM_CYSIZEFRAME );

	int yCaption = GetSystemMetrics( SM_CYCAPTION );

	Point p;
	if( mFixedWindow )
	{
		p.x = mMinimumWidth;
		p.y = mMinimumHeight;
	}
	else
	{
		p.x = xSizeFrame * 2 + mMinimumWidth;
		p.y = ySizeFrame * 2 + yCaption + mMinimumHeight;
	}
	return p;
}

// -------------------------------------------------------------
// Description:
//   Calculates the actual maximum window rect size using the 
//   the bounding rect of active monitors.
// Return Value:
//   Maximum size bounds of the window rect
// -------------------------------------------------------------
Point App::GetMaxBounds()
{
	int maxWidth = GetSystemMetrics( SM_CXVIRTUALSCREEN );
	int maxHeight = GetSystemMetrics( SM_CYVIRTUALSCREEN );

	int xSizeFrame = GetSystemMetrics( SM_CXSIZEFRAME );
	int ySizeFrame = GetSystemMetrics( SM_CYSIZEFRAME );

	int yCaption = GetSystemMetrics( SM_CYCAPTION );

	Point p;
	if( mFixedWindow )
	{
		p.x = maxWidth;
		p.y = maxHeight;
	}
	else
	{
		p.x = xSizeFrame * 2 + maxWidth;
		p.y = ySizeFrame * 2 + yCaption + maxHeight;
	}
	return p;
}

// -------------------------------------------------------------
// Description:
//   Get the virtual screen width(total desktop width)
// Return Value:
//   Total desktop width
// -------------------------------------------------------------
int App::GetVirtualScreenWidth() const
{
	return GetSystemMetrics( SM_CXVIRTUALSCREEN );
}

// -------------------------------------------------------------
// Description:
//   Get the virtual screen width(total desktop height)
// Return Value:
//   Total desktop height
// -------------------------------------------------------------
int App::GetVirtualScreenHeight() const
{
	return GetSystemMetrics( SM_CYVIRTUALSCREEN );
}

uint32_t App::GetKeyState( uint32_t vKeyCode )
{
    return ::GetKeyState( vKeyCode );
}

bool App::GetKeyName( uint32_t vk, char* buffer, size_t bufferSize )
{
	UINT sc = ::MapVirtualKey( vk, 0 );
    
	int asc = 0;
	if( vk > 32 && vk != VK_DIVIDE )
	{
		BYTE buf[256];
		memset(buf, 0, 256);
		unsigned short int temp;
		asc = ToAscii(vk, sc, buf, &temp, 1);
	}
    
	sc <<= 16;
	sc |= 0x1 << 25;  // <- don't care
	if( asc != 0 )
	{
		sc |= 0x1 << 24; // <- extended bit
	}
    
	int result = ::GetKeyNameText( sc, buffer, int( bufferSize ) );
	// GetKeyNameText is polluting the keyboard buffer whenever it's called with a dead key (for example, RING on 
	// Icelandic keyboard layouts, so we need to perform a dummy call to prevent that. See issue EVE-97662.
	// asc < 0 means that the requested key was a dead key.
	if (asc < 0 && vk != 65)
	{
		// 65 == the letter 'A' -- flushing with other values such as 0 or spacebar doesn't seem to work, it seems to need an ascii letter.
		char dummy[64];
		App::GetKeyName( 65, dummy, 63 );
	}

	return result != 0;
}

#endif
