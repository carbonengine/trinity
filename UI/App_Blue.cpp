#include "StdAfx.h"
#include "App.h"
#include "TriRect.h"
#include "Tr2PresentParameters.h"

BLUE_DEFINE( App );


const Be::ClassInfo* App::ExposeToBlue()
{
	EXPOSURE_BEGIN(App, NULL)

		MAP_INTERFACE(INotify)

		MAP_ATTRIBUTE("left", mCreationLeft, "Left", Be::READWRITE)
		MAP_ATTRIBUTE("top", mCreationTop, "Top", Be::READWRITE)

		MAP_ATTRIBUTE("width", mCreationWidth, "Width", Be::READWRITE)
		MAP_ATTRIBUTE("height", mCreationHeight, "Height", Be::READWRITE)

		MAP_ATTRIBUTE("refreshrate", mRefreshRate, "Refresh rate, 0=default", Be::READWRITE)
		MAP_ATTRIBUTE("title", mWindowTitle, "Window Title", Be::READWRITE | Be::NOTIFY)
		MAP_ATTRIBUTE("hideTitle", mHideTitle, "Hide title bar and borders", Be::READWRITE)
		MAP_ATTRIBUTE("fullscreen", mFullscreen, "Use fullscreen mode", Be::READWRITE | Be::NOTIFY)
		MAP_ATTRIBUTE("windowed", mWindowed, "Use windowed mode", Be::READWRITE | Be::NOTIFY)
		MAP_ATTRIBUTE("active", mActive, "is app active?", Be::READ)
		MAP_ATTRIBUTE("hwnd", mHwnd, "Window handle", Be::READ)
		MAP_ATTRIBUTE("fixedWindow", mFixedWindow, "Use fixed window mode", Be::READ)
		MAP_ATTRIBUTE( "minimumWidth", mMinimumWidth, "Sets the minimum window width allowed", Be::READWRITE )
		MAP_ATTRIBUTE( "minimumHeight", mMinimumHeight, "Sets the minimum window height allowed", Be::READWRITE )
		MAP_ATTRIBUTE( "isMaximized", mIsMaximized, "Indicates weather the window is maximized", Be::READ )

#if BLUE_WITH_PYTHON
		MAP_ATTRIBUTE(
			"eventHandler", 
			m_eventHandler, 
			"Generic Windows event handler",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"sessionChangeHandler", 
			m_sessionChangeHandler, 
			"OnSessionChangeHandler handler", 
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"resizeEventListener",
			mResizeEventListener,
			"An event listener for handling when the client has been resized",
			Be::READWRITE
		)
		
		MAP_ATTRIBUTE
		(
			"tgToggleEventListener",
			mTGToggleEventListener,
			"An event listener for handling notifications of fullscreen toggling from the TransGaming api",
			Be::READWRITE
		)
#endif
		
		MAP_ATTRIBUTE
		(
			"mouseCursor",
			m_cursor,
			"Cursor for mouse pointer",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_METHOD_AND_WRAP
		(
			"Initialize",
			CreateImpl, 
			"Sets up the Application, ready for device creation" 
		)
		
		MAP_METHOD_AND_WRAP
		(
			"Create",
			Create,	
			"Creates the Application" 
		)
		
		MAP_METHOD_AS_METHOD
		(
			"Destroy",
			PyDestroy,	
			"Destroyes the Application, hopefully" 
		)
		
		MAP_METHOD_AND_WRAP
		(
			"Quit",
			Quit,	
			"Quits the application" 
		)


		MAP_METHOD_AND_WRAP
		(
			"MoveWindow",
			MoveWindow,	
			"See Win32 MoveWindow" 
		)
		
		MAP_METHOD_AND_WRAP
		(
			"Minimize",
			Minimize,	
			"blah" 
		)
		
#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"GetClientRect",
			GetClientRect,	
			1,
			"blah" 
		)
		
		MAP_METHOD_AND_WRAP
		(
			"GetWindowRect",
			GetWindowRect,	
			"blahblah" 
		)
#endif
		
		MAP_METHOD_AS_METHOD
		(
			"GetHwnd",
			PyGetHwnd, 
			"Get the wrapped window handle of this thing" 
		)
		
		MAP_METHOD_AS_METHOD
		(
			"GetHwndAsLong",
			PyGetHwndAsLong, 
			"Get the window handle of the application as a long" 
		)


		MAP_METHOD_AS_METHOD
		(
			"AdjustWindowForChange",
			PyAdjustWindowForChange, 
			"Call before applying a device change" 
		)
		
		MAP_METHOD_AND_WRAP
		(
			"CreateDevice",
			CreateDevice,
			"Create the graphics device"
		)
		
		MAP_METHOD_AS_METHOD
		(
			"ChangeDevice",
			PyChangeDevice, 
			"Modify or set the device" 
		)
		
		MAP_METHOD_AS_METHOD
		(
			"SetWindowPos",
			PySetWindowPos, 
			"Set the window position" 
		)
		
		MAP_METHOD_AND_WRAP
		(
			"IsActive",
			IsActive, 
			"is the window active?" 
		)
		
		MAP_METHOD_AND_WRAP
		(
			"SetActive",
			SetActive, 
			"sets the window to be active" 
		)
		
		MAP_METHOD_AND_WRAP( "IsHidden", IsHidden, "Returns if the window is hidden." )

		MAP_METHOD_AND_WRAP( "GetVirtualScreenWidth", GetVirtualScreenWidth, "Returns the virtual screen width which is the total desktop width." )
		MAP_METHOD_AND_WRAP( "GetVirtualScreenHeight", GetVirtualScreenHeight, "Returns the virtual screen height which is the total desktop height." )

		MAP_METHOD_AND_WRAP( "Key", IsKeyPressed, "" )
		MAP_METHOD_AND_WRAP( "GetKeyState", GetKeyState, "" )
		MAP_METHOD_AND_WRAP( "SetKey", SetKey, "" )
		
		MAP_METHOD_AND_WRAP
		(
			"GetKeyNameText",
			GetKeyNameText,
			"Calls the Win32 function GetKeyNameText\n"
			"\n"
			"Arguments:\n"
			"  vk - virtual key code\n"
			"Returns:\n"
			"  Name of the key corresponding to vk"
		)
		
		MAP_METHOD_AND_WRAP
		( 
			"ClipCursor", 
			ClipCursor, 
			"Confines the cursor to a rectangular area on the screen. If a subsequent cursor position\n"
			"(set by the SetCursorPos function or the mouse) lies outside the rectangle, the system\n"
			"automatically adjusts the position to keep the cursor inside the rectangular area.\n"
			"\n"
			"Arguments:\n"
			"  left, top, bottom, right"
		)

		MAP_METHOD_AND_WRAP
		( 
			"UnclipCursor",
			UnclipCursor,
			"Lifts the confinement set on the cursor with ClipCursor."
		)

		MAP_METHOD_AND_WRAP
		(
			"SetCursorPos",
			SetCursorPos,
			"Set cursor position"
		)

		MAP_METHOD_AND_WRAP
		(
			"ProcessMessages",
			ProcessMessages,
			"Processes messages (one iteration of a typical message loop). Returns false if WM_QUIT is\n"
			"received, otherwise returns true."
		)
	EXPOSURE_END()

}
