// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "include/TriColor.h"
#include "Include/ITriVector.h"

#if BLUE_WITH_PYTHON
BLUE_DEFINE( TriColor );

const Be::ClassInfo* TriColor::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriColor, TRICOLOR_Description )
		MAP_INTERFACE( ITriColor )
		MAP_INTERFACE( IPythonMethods )

		////////////////////////////////////////////////////////////////////////////
		//               r
		MAP_ATTRIBUTE(
			"r",
			r,
			"",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               g
		MAP_ATTRIBUTE(
			"g",
			g,
			"",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               b
		MAP_ATTRIBUTE(
			"b",
			b,
			"",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               a
		MAP_ATTRIBUTE(
			"a",
			a,
			"",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               __init__
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			Py__init__,
			4,
			"Constructor arguments\n"
			":param r: red\n"
			":param g: green\n"
			":param b: blue\n"
			":param a: alpha\n" )

		////////////////////////////////////////////////////////////////////////////
		//               SetRGB
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"SetRGB",
			PySetRGB,
			4,
			"Sets the RGB values of the color\n"
			":param r: red\n"
			":param g: green\n"
			":param b: blue\n"
			":param a: alpha\n" )

		////////////////////////////////////////////////////////////////////////////
		//               SetHSV
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"SetHSV",
			PySetHSV,
			4,
			"Sets the HSV values of the color \r\n"
			"h = [0,360], s = [0,1], v = [0,1]\n"
			"if s == 0, then h = -1 (undefined)\n"
			":param hue: \n"
			":param saturation: \n"
			":param value: \n"
			":param alpha: \n" )

		////////////////////////////////////////////////////////////////////////////
		//               SetVector
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"SetVector",
			PySetVector,
			1,
			"Finds the color of the vector and uses that.\n"
			":param vec: vector\n"
			":param alpha: alpha" )

		////////////////////////////////////////////////////////////////////////////
		//               GetHSV
		MAP_METHOD_AND_WRAP(
			"GetHSV",
			PyGetHSV,
			"Returns the HSV values of the color\r\n"
			"h = [0,360], s = [0,1], v = [0,1]\r\n"
			"if s == 0, then h = -1 (undefined)" )

		////////////////////////////////////////////////////////////////////////////
		//               FromInt
		MAP_METHOD_AND_WRAP(
			"FromInt",
			PyFromInt,
			"Changes thie color to the one reprecented by the integer\n"
			":param color: " )

		////////////////////////////////////////////////////////////////////////////
		//               AsInt
		MAP_METHOD_AND_WRAP(
			"AsInt",
			PyAsInt,
			"Returns an integer reprecenting the color" )

		////////////////////////////////////////////////////////////////////////////
		//               Add
		MAP_METHOD_AS_METHOD(
			"Add",
			PyAdd,
			"BROKEN! Adds a color to this color.\n"
			":rtype: None" )

		////////////////////////////////////////////////////////////////////////////
		//               Lerp
		MAP_METHOD_AS_METHOD(
			"Lerp",
			PyLerp,
			"BROKEN! Uses linear interpolation to modify this color value\n"
			":rtype: None" )

		////////////////////////////////////////////////////////////////////////////
		//               Scale
		MAP_METHOD_AND_WRAP(
			"Scale",
			Scale,
			"Scales the value of the color.\n"
			":param s: scale" )
	EXPOSURE_END()
}
#endif
