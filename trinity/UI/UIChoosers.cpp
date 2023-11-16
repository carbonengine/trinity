#include "StdAfx.h"
#include "UIChoosers.h"
#include "IUILib.h"


#if BLUE_WITH_PYTHON
void AddUIChoosersToDict(PyObject* dict)
{
	const Be::VarChooser* kvs [] = 
	{	
		UIAlignChooser,
		UIAnchorChooser,
		UIIDChooser,
		UIIDBoxChooser,
		UIStateChooser,
		UIPosChooser,
		UICtlTypeChooser,
		UIFontSizeChooser,
		UIFontStyleChooser,
		UICursorChooser,
		UIEventTypeChooser
	};

	for (int i = 0; i < sizeof kvs / sizeof kvs[0]; i++)
	{
		const Be::VarChooser* kv = kvs[i];

		while (kv->mKey)
		{
			PyObject* value = PyInt_FromLong(kv->mValue.mLong);
			PyDict_SetItemString(dict, (char*)(kv->mKey ), value);
			Py_DECREF(value);
			kv++;
		}
	}
}
#endif



#define VAL(v) BeCast(v)

#define KV(_k, _d)\
	{#_k, VAL(_k), _d}



//--------------------------------------------------------------------
// UIAlignChooser
//--------------------------------------------------------------------
const Be::VarChooser UIAlignChooser[] =
{
	KV(UI_ALNONE, "None"),
	KV(UI_ALLEFT, "Left"),
	KV(UI_ALRIGHT, "Right"),
	KV(UI_ALTOP, "Top"),
	KV(UI_ALBOTTOM, "Bottom"),
	KV(UI_ALCLIENT, "Client"),
	KV(UI_ALABSOLUTE, "Absolute"),
	{0}
};

//--------------------------------------------------------------------
// UIEventTypeChooser
//--------------------------------------------------------------------
const Be::VarChooser UIEventTypeChooser[] =
{
	KV(UI_CREATE,"OnCreate"),
	KV(UI_UNHOOK,"OnUnHook"),
	KV(UI_DESTROY,"OnDestroy"),
	KV(UI_SETPARENT,"OnSetParent"),
	KV(UI_CLOSE,"OnClose"),
	KV(UI_RESIZE,"OnResize"),

	// Window state
	KV(UI_ACTIVATE,"OnActivate"),
	KV(UI_DEACTIVATE,"OnDeactivate"),
	KV(UI_SETFOCUS,"OnSetFocus"),
	KV(UI_KILLFOCUS,"OnKillFocus"),
	// Mouse events
	KV(UI_MOUSEENTER,"OnMouseEnter"),
	KV(UI_MOUSEEXIT,"OnMouseLeave"),
	KV(UI_MOUSEHOVER,"OnMouseHover"),
	KV(UI_MOUSEDOWN,"OnMouseDown"),
	KV(UI_MOUSEUP,"OnMouseUp"),
	KV(UI_MOUSEMOVE,"OnMouseMove"),
	KV(UI_MOUSEWHEEL,"OnMouseWheel"),
	KV(UI_CLICK,"OnClick"),
	KV(UI_DBLCLICK,"OnDblClick"),
	KV(UI_CONTEXTPOPUP,"OnContextPopup"),
	//{"OnHitTest",			UI_HITTEST},
	

	// Keyboard events
	KV(UI_KEYDOWN,"OnKeyDown"),
	KV(UI_KEYUP,"OnKeyUp"),
	KV(UI_CHAR,"OnKeyPress"),

	// Drag/drop
	KV(UI_STARTDRAG,"OnStartDrag"),
	KV(UI_DRAGOVER,"OnDragOver"), //(source, x, y, state[enter, leave, move]), return 1 to accept
	KV(UI_DRAGDROP,"OnDragDrop"), //(source, x, y)
	KV(UI_ENDDRAG,"OnEndDrag"),

	// Other
	KV(UI_SETTEXT,"OnSetText"),

	KV(UI_TRACKINGCHANGE, "OnTrackingChange"),
	KV(UI_STATECHANGE, "OnStateChange"),

	KV(UI_ACTIVE, "OnActive"),

	{0}
};



//--------------------------------------------------------------------
// UIAnchorChooser
//--------------------------------------------------------------------
const Be::VarChooser UIAnchorChooser[] =
{
	KV(UI_ANCHLEFT, 
		"The control\'s position is fixed with respect to the left edge of its parent."),
	KV(UI_ANCHRIGHT, 
		"The control\'s position is fixed with respect to the right edge of its parent."),
	KV(UI_ANCHTOP, 
		"The control\'s position is fixed with respect to the top edge of its parent."),
	KV(UI_ANCHBOTTOM, 
		"The control\'s position is fixed with respect to the bottm edge of its parent."),
	{0}
};


//--------------------------------------------------------------------
// UIIDChooser
//--------------------------------------------------------------------
const Be::VarChooser UIIDChooser[] =
{
	KV(ID_NONE,		"None. The default value before the user exits."),
	KV(ID_OK,		"The user exited with OK button." ),
	KV(ID_CANCEL,	"The user exited with the CANCEL button." ),
	KV(ID_YES,		"The user exited with the YES button." ),
	KV(ID_NO,		"The user exited with the NO button." ),
	KV(ID_CLOSE,	"The user exited with the CLOSE button." ),
	KV(ID_HELP,		"The user exited with the HELP button." ),
	{0}
};


//--------------------------------------------------------------------
// UIIDBoxChooser
//--------------------------------------------------------------------
const Be::VarChooser UIIDBoxChooser[] =
{
	KV(OK,					"OK"),
	KV(OKCANCEL,			"OK, Cancel"),
	KV(YESNOCANCEL,			"Yes, No, Cancel"),
	KV(YESNO,				"Yes, No"),

	KV(INFO,				"Yes, No"),
	KV(WARNING,				"Yes, No"),
	KV(QUESTION,			"Yes, No"),
	KV(ERROR,				"Yes, No"),
	KV(FATAL,				"Yes, No"),
	{0}
};


//--------------------------------------------------------------------
// UIStateChooser
//--------------------------------------------------------------------
const Be::VarChooser UIStateChooser[] =
{
	KV(UI_NORMAL,				"(Default)"),
	KV(UI_DISABLED,				"Disabled"),
	KV(UI_HIDDEN,				"Hidden"),
	KV(UI_PICKCHILDREN,			"PickChildren"),
	{0}
};


//--------------------------------------------------------------------
// UIPosChooser
//--------------------------------------------------------------------
const Be::VarChooser UIPosChooser[] =
{
	KV(
		UI_POSDESIGNED,
		"The form appears positioned on the screen and with the same "
		"height and width as it had at design time."
	),

	KV(
		UI_POSDEFAULT,	
		"The form appears in a position on the screen and with a height "
		"and width determined by UILib. Each time you create the window, "
		"the form moves slightly down and to the right. The right side "
		"of the form is always near the far right side of the screen, and "
		"the bottom of the form is always near the bottom of the screen, "
		"regardless of the screen's resolution."
	),

	KV(
		UI_POSCENTER,	
		"The form remains the size you left it at design time, but is "
		"positioned in the center of the screen."
	),

	KV(
		UI_POSCENTER2,	
		"Same as above, but moved just a tad upwards (ca. 15% of top margin)."
	),

	{0}
};


//--------------------------------------------------------------------
// UICtlTypeChooser
//--------------------------------------------------------------------
const Be::VarChooser UICtlTypeChooser[] =
{	
	KV(UI_IMAGE,	"No behaviour - act normal."),
	KV(UI_PUSHBTN,	"Behave as push button."),
	KV(UI_RADIOBTN,	"Behave as radio button."),
	KV(UI_CHECKBTN,	"Behave as check box."),
	KV(UI_IMAGEBTN,	"Does UV coord. adjustments."),
	{0}
};


//--------------------------------------------------------------------
// UIFontSizeChooser
//--------------------------------------------------------------------
const Be::VarChooser UIFontSizeChooser[] =
{
	KV(UIFONT_6X10,		"Font 6X10"),
	KV(UIFONT_9X12,		"Font 9X12"),
	KV(UIFONT_9X17,		"Font 9X17"),
	{0}
};


//--------------------------------------------------------------------
// UIFontStyleChooser
//--------------------------------------------------------------------
const Be::VarChooser UIFontStyleChooser[] =
{
	KV(UIFONT_NORMAL,		"Normal"),
	KV(UIFONT_DROPSHADOW,	"Drop shadow"),
	KV(UIFONT_HIGHLIGHTED,	"Highlighted"),
	{0}
};


//--------------------------------------------------------------------
// UICursorChooser
//--------------------------------------------------------------------
const Be::VarChooser UICursorChooser[] =
{
	KV(UICURSOR_DEFAULT,	"Default cursor"),
	KV(UICURSOR_CROSS,		"Crosshair"),
	KV(UICURSOR_SELECT,		"Selectable item"),
	KV(UICURSOR_SELECTDOWN,	"Selectable item with mouse button down"),
	KV(UICURSOR_DRAGGABLE,	"Draggable item"),
	KV(UICURSOR_TARGET1,	"Selectable target - blink state 1"),
	KV(UICURSOR_TARGETDOWN,	"Selectable target with mouse button down"),
	KV(UICURSOR_TARGET2,	"Selectable target - blink state 2"),
	KV(UICURSOR_IBEAM,		"I-beam"),
	{0}
};