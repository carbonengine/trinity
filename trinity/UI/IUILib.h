/* 
	*************************************************************************

	IUILib.h

	Created:   Jan. 2000, May 2001
	OS:        Win32
	Project:   TriUI

	Description:   

		UI stuff


	Dependencies:

		Blue, Trinity

	(c) CCP 2000, 2001

	*************************************************************************
*/

#pragma once

#ifndef _IUILIB_H_
#define _IUILIB_H_


//////////////////////////////////////////////////////////////////////
//
//	The numerous enums
// 
//////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------
// UIALIGN
//--------------------------------------------------------------------
enum UIALIGN
{
	UI_ALNONE		= 0,
	UI_ALLEFT		= 1,
	UI_ALRIGHT		= 2,
	UI_ALTOP		= 3,
	UI_ALBOTTOM		= 4,
	UI_ALCLIENT		= 5,
	UI_ALABSOLUTE   = 6,
	_UI_ALLAST		= 7,
};


//--------------------------------------------------------------------
// UIANCHOR
//--------------------------------------------------------------------
enum UIANCHOR
{
	// When anchored the control's position is fixed with respect to the
	// anchored edge of it's parent.
	
	UI_ANCHTOP		= 0x1,
	UI_ANCHLEFT		= 0x2,	
	UI_ANCHRIGHT	= 0x4,	
	UI_ANCHBOTTOM	= 0x8,	
};


//--------------------------------------------------------------------
// UID - Those are the same values as Win32 dialog box command id's.
//--------------------------------------------------------------------
enum UID
{	
	ID_NONE		= 0,
	ID_OK		= 1,
	ID_CANCEL	= 2,
	ID_YES		= 6,
	ID_NO		= 7,
	ID_CLOSE	= 8,
	ID_HELP		= 9,
};


//--------------------------------------------------------------------
// UIBOX
//--------------------------------------------------------------------
#ifdef ERROR
#undef ERROR
#endif

enum UIBOX
{
	OK						= 0,
	OKCANCEL				= 1,
	YESNOCANCEL				= 2,
	YESNO					= 3,

	INFO					= 0,
	WARNING					= 1,
	QUESTION				= 2,
	ERROR					= 3,
	FATAL					= 4,
};

// UISTATE
//--------------------------------------------------------------------
enum UISTATE
{
	UI_NORMAL			= 0x00,
	UI_DISABLED			= 0x01, //not pickable
	UI_HIDDEN			= 0x02, //not visible. ignored completely
	UI_PICKCHILDREN		= 0x03, //not pickable, but children are.
};


//--------------------------------------------------------------------
// UIINITPOS
//--------------------------------------------------------------------
enum UIINITPOS
{
	// The form appears positioned on the screen and with the same 
	// height and width as it had at design time.
	UI_POSDESIGNED		= 0,

	// The form appears in a position on the screen and with a height 
	// and width determined by UILib. Each time you create the window, 
	// the form moves slightly down and to the right. The right side 
	// of the form is always near the far right side of the screen, and 
	// the bottom of the form is always near the bottom of the screen, 
	// regardless of the screen's resolution.
	UI_POSDEFAULT		= 1,

	// The form remains the size you left it at design time, but is 
	// positioned in the center of the screen.
	UI_POSCENTER		= 2,

	// Same as above, but moved just a tad upwards (ca. 10%)
	UI_POSCENTER2		= 3,
};


//--------------------------------------------------------------------
// UIHITAREA
//--------------------------------------------------------------------
enum UIHITAREA
{
	UIHIT_CLIENT	= 1,	// In a client area.
	UIHIT_CAPTION	= 2,	// In a title bar.	
};


//--------------------------------------------------------------------
// UICONTROLTYPE
//--------------------------------------------------------------------
enum UICONTROLTYPE
{
	UI_IMAGE		= 0,	// No behaviour - act normal
	UI_PUSHBTN		= 1,	// Behave as push button
	UI_RADIOBTN		= 2,	// Behave as radio button
	UI_CHECKBTN		= 3,	// Behave as check box
	UI_IMAGEBTN		= 4,	// Does UV coord. adjustments

	// see Button Styles in Win32:
	// mk:@MSITStore:C:\Program%20Files\Microsoft%20Visual%20Studio\MSDN\2000OCT\1033\winui.chm::/hh/winui/buttons_34c3.htm
};


//--------------------------------------------------------------------
// UIMESSAGE
//--------------------------------------------------------------------
enum UIMESSAGE
{
	UI_INVALID  = -1,

	// only 'mEvent' member is valid
	UI_RAW		= 0,


	//////////////////////////////////////////
	// Window messages
	//////////////////////////////////////////
	UI_CREATE,
	UI_UNHOOK,
	UI_DESTROY,
	UI_SETPARENT,
	UI_SETOWNER,
	UI_CLOSE,
	UI_RESIZE,

	UI_ACTIVATE,
	UI_DEACTIVATE,

	UI_STARTDRAG,
	UI_DRAGOVER,
	UI_DRAGDROP,
	UI_ENDDRAG,


	//////////////////////////////////////////
	// mouse events
	//////////////////////////////////////////
	UI_MOUSE_FIRST	= 100,
	

	// mouse movements
	UI_MOUSEMOVE,
	UI_MOUSEWHEEL,

	
	// mouse button stuff, 'mParam' is valid:
	// 0=left, 1=right, 2=middle, 3=x button 1, 4=x button 2, ...
	UI_MOUSEDOWN,			// 
	UI_MOUSEUP,

	// translated events
	UI_CAPTURECHANGED,
	UI_MOUSEENTER,
	UI_MOUSEEXIT,
	UI_MOUSEHOVER,
	UI_CLICK,
	UI_DBLCLICK,
	UI_CONTEXTPOPUP,
	UI_HITTEST,


	UI_MOUSE_LAST = UI_HITTEST,


	//////////////////////////////////////////
	// keyboard events
	//////////////////////////////////////////
	UI_KDB_FIRST	= 200,

	UI_CHAR,				// 'mWChar' is valid
	UI_KEYDOWN,				// 'mEvent' is valid
	UI_KEYUP,				// 'mEvent' is valid
	UI_SETFOCUS,
	UI_KILLFOCUS,


	//////////////////////////////////////////
	// joystick events
	//////////////////////////////////////////

	// ...

	//////////////////////////////////////////
	// Other events
	//////////////////////////////////////////
	UI_OTHER_FIRST = 300,

	UI_SETTEXT,
	
	// Target bracket fiff
	UI_TRACKINGCHANGE,

	//mState changed
	UI_STATECHANGE,

	//Application activation
	UI_ACTIVE
};


//--------------------------------------------------------------------
// INPUTTYPE
//--------------------------------------------------------------------
enum INPUTTYPE
{
	IT_KBD,
	IT_MOUSE,
	IT_JOY,
	IT_LAST,
};


//--------------------------------------------------------------------
// UIFONTSIZE
//--------------------------------------------------------------------
enum UIFONTSIZE
{
	UIFONT_6X10		= 0,
	UIFONT_9X12		= 1,
	UIFONT_9X17		= 2,
	_UIFONT_LAST	= 3,
};


//--------------------------------------------------------------------
// UIFONTSTYLE
//--------------------------------------------------------------------
enum UIFONTSTYLE
{
	UIFONT_NORMAL		= 0,
	UIFONT_DROPSHADOW	= 1,
	UIFONT_HIGHLIGHTED	= 2,
	_UIFONTSTYLE_LAST	= 3,
};


//--------------------------------------------------------------------
// UICURSOR
//--------------------------------------------------------------------
enum UICURSOR
{
	UICURSOR_DEFAULT	= -1,
	UICURSOR_CROSS		= 0,
	UICURSOR_SELECT		= 1,
	UICURSOR_SELECTDOWN	= 2,
	UICURSOR_DRAGGABLE	= 3,
	UICURSOR_TARGET1	= 4,
	UICURSOR_TARGETDOWN	= 5,
	UICURSOR_TARGET2	= 6,
	UICURSOR_IBEAM		= 7,
};

//--------------------------------------------------------------------
// UITRAVERSE
//--------------------------------------------------------------------
enum UITRAVERSE
{
	UITRAVERSE_ALIGN = 1,
	UITRAVERSE_RENDER = 2,
	UITRAVERSE_CALIGN = 4, //recursively adjust children
	UITRAVERSE_CRENDER = 8, //recursively render children
	UITRAVERSE_RECURSE = 12, //bitwise or of the above.
	UITRAVERSE_MULTIPLE = 16, //we are multiply instanced
	UITRAVERSE_STOREDALIGN = 32, //parent stores alignment

	//status from Traverse:
	UITRAVERSE_OK = 1,
	UITRAVERSE_FAIL = 0,
	UITRAVERSE_AGAIN = 2,
};


#endif