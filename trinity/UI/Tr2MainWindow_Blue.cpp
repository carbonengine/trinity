////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "Tr2MainWindow.h"
#include "Tr2MouseCursor.h"

namespace
{
	Be::VarChooser Tr2WindowModeChooser[] =
	{
		{
			"FULL_SCREEN",
			BeCast( Tr2WindowMode::FULL_SCREEN ),
			""
		},
		{
			"WINDOWED",
			BeCast( Tr2WindowMode::WINDOWED ),
			""
		},
		{
			"FIXED_WINDOW",
			BeCast( Tr2WindowMode::FIXED_WINDOW ),
			""
		},
		{ 0 }
	};

	Be::VarChooser Tr2WindowShowStateChooser[] =
	{
		{
			"NORMAL",
			BeCast( Tr2WindowShowState::NORMAL ),
			""
		},
		{
			"MAXIMIZED",
			BeCast( Tr2WindowShowState::MAXIMIZED ),
			""
		},
		{
			"MINIMIZED",
			BeCast( Tr2WindowShowState::MINIMIZED ),
			""
		},
		{ 0 }
	};
#if __APPLE__
	Be::VarChooser Tr2ImeStateChooser_MacOS[] =
	{
		{
			"DISABLED",
			BeCast( Tr2ImeState_MacOS::DISABLED ),
			""
		},
		{
			"READY",
			BeCast( Tr2ImeState_MacOS::READY ),
			""
		},
		{
			"BLOCKING",
			BeCast( Tr2ImeState_MacOS::BLOCKING ),
			""
		},
		{ 0 }
	};
#endif
}

BLUE_REGISTER_ENUM_EX( "Tr2WindowMode", Tr2WindowMode::Type, Tr2WindowModeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
BLUE_REGISTER_ENUM_EX( "Tr2WindowShowState", Tr2WindowShowState::Type, Tr2WindowShowStateChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
#if __APPLE__
BLUE_REGISTER_ENUM_EX( "Tr2ImeState_MacOS", Tr2ImeState_MacOS::Type, Tr2ImeStateChooser_MacOS, ENUM_REG_ENUM_OBJECT_ON_MODULE );
#endif


BLUE_DEFINE( Tr2MainWindowState );

const Be::ClassInfo* Tr2MainWindowState::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MainWindowState, "" )

		MAP_INTERFACE( Tr2MainWindowState )

		MAP_ATTRIBUTE( "windowMode", m_state.windowMode, "Window state, one of trinity.Tr2WindowState", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "adapter", m_state.adapter, "Adapter index", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "width", m_state.width, "Window client width", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "height", m_state.height, "Window client height", Be::READWRITE | Be::PERSIST )
		//MAP_ATTRIBUTE( "backBufferWidth", m_state.backBufferWidth, "Back buffer width", Be::READWRITE | Be::PERSIST )
		//MAP_ATTRIBUTE( "backBufferHeight", m_state.backBufferHeight, "Back buffer height", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "presentInterval", m_state.presentInterval, "Present interval, one of trinity.PRESENT_INTERVAL", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "left", m_state.left, "Window left position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "top", m_state.top, "Window top position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "showState", m_state.showState, "Window show (frame) state, one of trinity.Tr2WindowShowState", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "__str__", ToString, "" )
	EXPOSURE_END()
}


BLUE_DEFINE( Tr2MainWindow );

const Be::ClassInfo* Tr2MainWindow::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MainWindow, "" )

		MAP_INTERFACE( Tr2MainWindow )

		MAP_METHOD_AND_WRAP(
			"SetWindowState",
			SetStateScript,
			"Assigns new state for the window. This may trigger a device change, window resizing, etc.\n"
			"The first time it is called, the OS window is created.\n"
			":param state: new window state"
		)

		MAP_METHOD_AND_WRAP(
			"GetWindowState",
			GetStateScript,
			"Returns a current window state."
		)

		MAP_METHOD_AND_WRAP(
			"SanitizeState",
			SanitizeStateScript,
			"Sanitizes state attributes, based on the current HW, etc. Changes the parameters in place.\n"
			":param state: state to sanitize"
		)

		MAP_METHOD_AND_WRAP(
			"GetDefaultState",
			GetDefaultState,
			"Returns the default window state for a given window mode.\n"
			":param mode: window mode, one of trinity.Tr2WindowMode"
		)

		MAP_METHOD_AND_WRAP(
			"HasFocus",
			HasFocus,
			"Returns True iff the window has focus (is active)."
		)

		MAP_METHOD_AND_WRAP(
			"IsActive",
			HasFocus,
			":jessica-deprecated: use HasFocus"
		)

		MAP_METHOD_AND_WRAP( 
			"SetMinimumSize", 
			SetMinimumSize, 
			"Assigns minimum client size for the window. Should ideally be called before SetWindwoState as\n"
			"this function does not resize the window if it is smalleer than the minimum size.\n"
			":param width: minimum window client width\n"
			":param height: minimum window client height"
		)

		MAP_METHOD_AND_WRAP(
			"GetWindowTitle",
			GetWindowTitle,
			"Returns current window title"
		)

		MAP_METHOD_AND_WRAP(
			"SetWindowTitle",
			SetWindowTitle,
			"Sets new window title.\n"
			":param title: new window title"
		)

		MAP_METHOD_AND_WRAP(
			"GetBackBufferFormat",
			GetBackBufferFormat,
			"Returns format of the back buffer for the given adapter.\n"
			":param adapter: video adapter index"
		)

		MAP_METHOD_AND_WRAP(
			"GetWindowSizeOptions",
			GetWindowSizeOptions,
			"Returns a list of window acceptable sizes given the adapter and window mode. For fullscreen mode,\n"
			"this list would correspond to available display resolutions, for window mode it would also contain\n"
			"a size that covers the entire desktop.\n"
			":param adapter: video adapter index\n"
			":param windowMode: window mode (see trinity.Tr2WindowMode)"
		)

        MAP_METHOD_AND_WRAP(
            "StoreStateSettings",
            StoreStateSettings,
            "Store settings for the given mode. They may be used when a window switches to this mode using\n"
            "means other than Python (enter full screen menu on macOS for example).\n"
            ":param state: window state to store"
        )

		MAP_PROPERTY_READONLY( "width", GetBackBufferWidth, "Current window back buffer width" )
		MAP_PROPERTY_READONLY( "height", GetBackBufferHeight, "Current window back buffer height" )
		MAP_PROPERTY( "title", GetWindowTitle, SetWindowTitle, "Window title" )
		MAP_PROPERTY_READONLY( "active", HasFocus, ":jessica-deprecated: use HasFocus" )
		MAP_METHOD_AND_WRAP(
			"IsHidden",
			IsMinimized,
			"Returns True if the window is minimized"
		)


		MAP_PROPERTY( "mouseCursor", GetMouseCursor, SetMouseCursor, "Cursor to use when mouse is over the window" )
		MAP_METHOD_AND_WRAP
		(
			"GetCursorPos",
			GetCursorPos,
			"Returns current mouse cursor position in window client area coordinates"
		)
		MAP_METHOD_AND_WRAP
		(
			"SetCursorPos",
			SetCursorPos,
			"Set cursor position\n"
			":param x: horizontal cursor coordinate (window client space)\n"
			":param y: vertical cursor coordinate (window client space)\n"
		)

		MAP_METHOD_AND_WRAP
		(
			"ClipCursor",
			ClipCursor,
			"Confines the cursor to a rectangular area on the screen. If a subsequent cursor position\n"
			"(set by the SetCursorPos function or the mouse) lies outside the rectangle, the system\n"
			"automatically adjusts the position to keep the cursor inside the rectangular area.\n"
			"\n"
			":param left: left rect coordinate (window client space)\n"
			":param top: top rect coordinate (window client space)\n"
			":param right: right rect coordinate (window client space)\n"
			":param bottom: bottom rect coordinate (window client space)\n"
		)

		MAP_METHOD_AND_WRAP
		(
			"UnclipCursor",
			UnclipCursor,
			"Lifts the confinement set on the cursor with ClipCursor."
		)

		MAP_METHOD_AND_WRAP
		( 
			"Key", 
			IsKeyPressed, 
			"Returns True if the specified key is pressed\n"
			":param key: key code" 
		)
		MAP_METHOD_AND_WRAP
		( 
			"IsKeyToggled", 
			IsKeyToggled, 
			"Returns True if the specified key is toggled on\n"
			":param key: key code" 
		)
		MAP_METHOD_AND_WRAP
		(
			"GetKeyNameText",
			GetKeyName,
			"Returns the name of the secified key\n"
			":param key: virtual key code\n"
		)

		MAP_METHOD_AND_WRAP
		(
			"ProcessMessages",
			ProcessMessages,
			"Processes all accumulared OS messages. Returns false if quit message was processed.\n"
			"Should not be used when running exefile as the processing there happens in blue."
		)


		MAP_ATTRIBUTE(
			"onWindowStateChange",
			m_onWindowStateChange,
			"Called whenever window state changes. This includes, for example, window movement.\n"
			"Called with (state: trinity.Tr2MainWindowState) parameter",
			Be::READWRITE
		)

		MAP_ATTRIBUTE(
			"onBeforeSwapChainChange",
			m_onBeforeSwapChainChange,
			"Called before window state change triggers swap chain or device change.\n"
			"Called with (state: trinity.Tr2MainWindowState) parameter",
			Be::READWRITE
		)

		MAP_ATTRIBUTE(
			"onSwapChainChange",
			m_onSwapChainChange,
			"Called after swap chain / device change.\n"
			"Called with (state: trinity.Tr2MainWindowState) parameter",
			Be::READWRITE
		)


		MAP_ATTRIBUTE(
			"onMouseMove",
			m_onMouseMove,
			"Called when mouse is moved over the window.\n"
			"Called with (left: int, top: int) parameters",
			Be::READWRITE
		)

		MAP_ATTRIBUTE(
			"onMouseDown",
			m_onMouseDown,
			"Called when mouse button is pressed.\n"
			"Called with (button: int, left: int, top: int) parameters. Button parameter is 0 for left button,\n"
			"1 for right, 2 for middle and >2 for other mouse buttons.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE(
			"onMouseUp",
			m_onMouseUp,
			"Called when mouse button is depressed.\n"
			"Called with (button: int, left: int, top: int) parameters. Button parameter is 0 for left button,\n"
			"1 for right, 2 for middle and >2 for other mouse buttons.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE(
			"onMouseWheel",
			m_onMouseWheel,
			"Called when mouse wheel is rotated.\n"
			"Called with (delta: int) parameters.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE(
			"onKeyDown",
			m_onKeyDown,
			"Called when a key is pressed on the keyboard.\n"
			"Called with (key: int, flags: int) parameters.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE(
			"onKeyUp",
			m_onKeyUp,
			"Called when a key is released on the keyboard.\n"
			"Called with (key: int, flags: int) parameters.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE(
			"onChar",
			m_onChar,
			"Called when key press results in a typed character, or when a \"dead\" character is registered.\n"
			"Called with (key: int, flags: int, isDead: boolean) parameters.",
			Be::READWRITE
		)


		MAP_ATTRIBUTE(
			"onFocusChange",
			m_onFocusChange,
			"Called when the window looses or receives focus.\n"
			"Called with (inFocus: boolean) parameters.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE(
			"onClose",
			m_onClose,
			"Called when the window is requested to close. Called with no parameters and is supposed to return a boolean value.\n"
			"If return value of the callback is True, the window will close, otherwise it stays open.",
			Be::READWRITE
		)

        MAP_PROPERTY_READONLY( "hwnd", GetHwnd, "Returns managed window handle")
        MAP_METHOD_AND_WRAP
        (
            "GetHwndAsLong",
            GetHwnd,
            "Returns managed window handle"
        )

#if _WIN32
		MAP_ATTRIBUTE(
			"onWindowsMessage",
			m_onWindowsMessage,
			"Called for all Windowd-specific messages.\n"
			"Called with (msg: int, wParam: int, lParam: int) parameters, and is supposed to return a tuple\n"
			"(retValue: int, handled: boolean). If handled is True, the default window proc is not called and\n"
			"window procedure returns retValue for handled message. Otherwise it calls default window proc.",
			Be::READWRITE
		)

		MAP_METHOD_AND_WRAP
		(
			"GetWindowsMessageFilter",
			GetWindowsMessageFilter,
			"Returns message filter for onWindowsMessage callback. Returns a pair (enabled, messages). If enabled is False,\n"
			"onWindowsMessage is called for all messages, otherwise only for messages in the set \"messages\"."
		)

		MAP_METHOD_AND_WRAP
		(
			"SetWindowsMessageFilter",
			SetWindowsMessageFilter,
			"Changes message filter for onWindowsMessage callback.\n"
			":param enabled: if False, onWindowsMessage is called for all messages, otherwise only for messages in the set \"messages\".\n"
			":param messages: set of message types, for which onWindowMessage needs to be called (if \"enabled\" is True)"
		)
#elif __APPLE__
	
	MAP_ATTRIBUTE
	(
		"onInsertTextIME_MacOS",
		m_onInsertTextIME_MacOS,
		"This callback prompts insertion of text by the IME.",
		Be::READWRITE
	)
	
	MAP_ATTRIBUTE
	(
		"onSetMarkedTextIME_MacOS",
		m_onSetMarkedTextIME_MacOS,
		"This callback prompts replacement of the selected text by the IME",
		Be::READWRITE
	)

	MAP_ATTRIBUTE
	(
		"imeState_MacOS",
		m_imeState_MacOS,
		"Controls the ime state on macOS. Blocking state indicates active IME, and will block normal key events.",
		Be::READWRITE
	)

    MAP_ATTRIBUTE
    (
        "onKeyboardLayoutChange_MacOS",
        m_onKeyboardLayoutChange_MacOS,
        "This callback gets called macOS when the user changes their keyboard layout.",
        Be::READWRITE
    )

#endif
	EXPOSURE_END()
}
