// Copyright © 2000 CCP ehf.

#ifndef _TRICONSTANTS_H_
#define _TRICONSTANTS_H_

const char* KeyFromVal( const Be::VarChooser* i, long val );
#if BLUE_WITH_PYTHON
void AddTriConstants( PyObject* d );
#endif

extern const Be::VarChooser TriTextureChooser[];
extern const Be::VarChooser TriGR2Chooser[];
extern const Be::VarChooser TriExtrapolation[];
extern const Be::VarChooser TriOperator[];
extern const Be::VarChooser TriTransformBase[];
extern const Be::VarChooser TriBlendOp[];
extern const Be::VarChooser TriBlend[];
extern const Be::VarChooser TriTextureAddress[];
extern const Be::VarChooser TriD3DRenderState[];

#endif
