/* 
	*************************************************************************

	ITriConstants.h

	Author:    Hilmar Veigar Pétursson
	Created:   August 2001
	OS:        Win32
	Project:   Trinity

	Description:   

		Yeap


	Dependencies:

		DirectX 9.0, Probably more, ytbd.

	(c) CCP 2000

	*************************************************************************
*/

#pragma once

#ifndef _ITriConstants_H_
#define _ITriConstants_H_



////////////////////////////////////////////////////////////////////////////
// Curve constants
////////////////////////////////////////////////////////////////////////////

enum TRIEXTRAPOLATION { 
	TRIEXT_NONE = 0,
	TRIEXT_CONSTANT = 1,
	TRIEXT_GRADIENT = 2,
	TRIEXT_CYCLE = 3,
};


////////////////////////////////////////////////////////////////////////////
// Transform constants
////////////////////////////////////////////////////////////////////////////

// large used for picking
const float HUGE_NUMBER = 1.0e+36F;

// smallest such that 1.0+FLT_EPSILON != 1.0 
const float TRI_FLT_EPSILON = 1.192092896e-07F;
const float TRI_FLT_MAX     = 3.402823466e+38F;


enum TRITRANSFORMBASE{ 
	TRITB_OBJECT = 0,
	TRITB_CAMERA_ROTATION = 1,
	TRITB_CAMERA_TRANSLATION = 2,
	TRITB_CAMERA = 3,
	TRITB_CAMERA_ROTATION_ALIGNED = 4,
	TRITB_FIXED = 5,
	TRITB_CAMERA_ROTATION_FALLOFF = 6,
	TRITB_CAMERA_ROTATION_ALIGNED_SYMMETRY = 7,
	TRITB_CAMERA_ROTATION_FALLOFF_SYMMETRY = 8,
	TRITB_BOOSTER = 9,
	TRITB_SIMPLE_HALO = 10,
	TRITB_SIMPLE_HALO_SYMMETRY = 11,
	TRITB_BOOSTER_FALLOFF = 12,
	TRITB_WORLD = 13,
	TRITB_SIMPLE_HALO_FALLOFF = 14,
	TRITB_SIMPLE_SPRITE = 15,
	TRITB_SIMPLE_SPRITE_FALLOFF = 16,
	TRITB_SIMPLE_SPRITE_CONSTANT = 17,
};

enum TRITRANSFORMAXIS{ 
	TRITA_X = 0,
	TRITA_Y = 1,
	TRITA_Z = 2
};

enum TRIOPERATOR{ 
	TRIOP_MULTIPLY = 0,
	TRIOP_ADD = 1,
	TRIOP_AVERAGE = 2,
};

#endif