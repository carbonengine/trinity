// Copyright © 2000 CCP ehf.

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
void AddUIChoosersToDict( PyObject* dict );
#endif


#endif
