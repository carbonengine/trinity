/* 
	*************************************************************************

	UIChoosers.h

	Created:   May 2001
	OS:        Win32
	Project:   TriUI

	Description:   

		UI stuff


	Dependencies:

		Blue, Trinity

	(c) CCP 2000, 2001

	*************************************************************************
*/

#ifndef _UICHOOSERS_H_
#define _UICHOOSERS_H_



extern const Be::VarChooser UIAlignChooser[];
extern const Be::VarChooser UIAnchorChooser[];
extern const Be::VarChooser UIEventTypeChooser[];
extern const Be::VarChooser UIIDChooser[];
extern const Be::VarChooser UIIDBoxChooser[];
extern const Be::VarChooser UIStateChooser[];
extern const Be::VarChooser UIPosChooser[];
extern const Be::VarChooser UICtlTypeChooser[];
extern const Be::VarChooser UIFontSizeChooser[];
extern const Be::VarChooser UIFontStyleChooser[];
extern const Be::VarChooser UICursorChooser[];



#if BLUE_WITH_PYTHON
void AddUIChoosersToDict(PyObject* dict);
#endif


#endif

