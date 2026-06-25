// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2KelvinColor.h"

BLUE_DEFINE( Tr2KelvinColor );

static const float MIN_KELVIN = 1000.0f;
static const float MAX_KELVIN = 10000.0f;

static Be::VarChooser Tr2StandardIlluminantChooser[] = {
	{ "CIE_A",
	  BeCast( TR2STANDARDILLUMINANT_A ),
	  "(CIE A) Incandescent / Tungsten" },
	{ "CIE_D50",
	  BeCast( TR2STANDARDILLUMINANT_D50 ),
	  "(CIE D50) Horizon Light" },
	{ "CIE_D55",
	  BeCast( TR2STANDARDILLUMINANT_D55 ),
	  "(CIE D55) Mid-Morning / Mid-Afternoon Daylight" },
	{ "CIE_D65",
	  BeCast( TR2STANDARDILLUMINANT_D65 ),
	  "(CIE D65) Noon Daylight" },
	{ "CIE_D75",
	  BeCast( TR2STANDARDILLUMINANT_D75 ),
	  "(CIE D75) North Sky Daylight" },
	{ "CIE_E",
	  BeCast( TR2STANDARDILLUMINANT_E ),
	  "(CIE E) Equal Energy" },
	{ "CIE_F1",
	  BeCast( TR2STANDARDILLUMINANT_F1 ),
	  "(CIE F1) Daylight Fluorescent" },
	{ "CIE_F2",
	  BeCast( TR2STANDARDILLUMINANT_F2 ),
	  "(CIE F2) Cool White Fluorescent" },
	{ "CIE_F3",
	  BeCast( TR2STANDARDILLUMINANT_F3 ),
	  "(CIE F3) White Fluorescent" },
	{ "CIE_F4",
	  BeCast( TR2STANDARDILLUMINANT_F4 ),
	  "(CIE F4) Warm White Fluorescent" },
	{ "CIE_F5",
	  BeCast( TR2STANDARDILLUMINANT_F5 ),
	  "(CIE F5) Daylight Fluorescent" },
	{ "CIE_F6",
	  BeCast( TR2STANDARDILLUMINANT_F6 ),
	  "(CIE F6) Lite White Fluorescent" },
	{ "CIE_F7",
	  BeCast( TR2STANDARDILLUMINANT_F7 ),
	  "(CIE F7) D65 Fluorescent Simulator" },
	{ "CIE_F8",
	  BeCast( TR2STANDARDILLUMINANT_F8 ),
	  "(CIE F8) D50 Fluorescent Simulator" },
	{ "CIE_F9",
	  BeCast( TR2STANDARDILLUMINANT_F9 ),
	  "(CIE F9) Cool White Deluxe Fluorescent" },
	{ "CIE_F10",
	  BeCast( TR2STANDARDILLUMINANT_F10 ),
	  "(CIE F10) Philips TL85, Ultralume 50" },
	{ "CIE_F11",
	  BeCast( TR2STANDARDILLUMINANT_F11 ),
	  "(CIE F11) Philips TL84, Ultralume 40" },
	{ "CIE_F12",
	  BeCast( TR2STANDARDILLUMINANT_F12 ),
	  "(CIE F12) Philips TL83, Ultralume 30" },
	{ 0 }
};

// Register the standard illuminant enum as trinity.Tr2StandardIlluminant
BLUE_REGISTER_ENUM_EX( "Tr2StandardIlluminant",
					   Tr2StandardIlluminant,
					   Tr2StandardIlluminantChooser,
					   ENUM_REG_ENUM_OBJECT_ON_MODULE );

#if BLUE_WITH_PYTHON
static PyObject* PyTriColorFromKelvin( PyObject* self, PyObject* args )
{
	float temperature, tint;
	int intWhiteBalance;
	if( !PyArg_ParseTuple( args, "ffi", &temperature, &tint, &intWhiteBalance ) )
	{
		PyErr_SetString( PyExc_TypeError, "Provided arguments do not match function signature of ( float, float, int )" );
		return NULL;
	}

	Tr2StandardIlluminant stdIllum = (Tr2StandardIlluminant)intWhiteBalance;
	Vector3d color = TriColorFromKelvin( temperature, tint, stdIllum );

	PyObject* ret = PyTuple_New( 4 );
	PyTuple_SET_ITEM( ret, 0, PyFloat_FromDouble( color.x ) );
	PyTuple_SET_ITEM( ret, 1, PyFloat_FromDouble( color.y ) );
	PyTuple_SET_ITEM( ret, 2, PyFloat_FromDouble( color.z ) );
	PyTuple_SET_ITEM( ret, 3, PyFloat_FromDouble( 1.0 ) );

	return ret;
}

static PyObject* PyGetKelvinTemperatureBuffer( PyObject* self, PyObject* args )
{
	int width, height;
	int intWhiteBalance;
	if( !PyArg_ParseTuple( args, "iii", &width, &height, &intWhiteBalance ) )
	{
		return NULL;
	}

	Tr2StandardIlluminant stdIllum = (Tr2StandardIlluminant)intWhiteBalance;
	char* buffer = CCP_NEW( "PyGetKelvinTemperatureBuffer" ) char[3 * width * height];

	const float startK = MIN_KELVIN;
	const float deltaK = ( MAX_KELVIN - MIN_KELVIN ) / ( static_cast<float>( width ) - 1.0f );
	float curK = startK;

	// Render one row of the bitmap
	char* ptr = buffer;
	for( int i = 0; i < width; ++i )
	{
		Vector3d rgb = TriColorFromKelvin( curK, 0.5f, stdIllum );
		ptr[3 * i] = static_cast<char>( rgb.x * 255.0 );
		ptr[3 * i + 1] = static_cast<char>( rgb.y * 255.0 );
		ptr[3 * i + 2] = static_cast<char>( rgb.z * 255.0 );
		curK += deltaK;
	}

	// Copy the row into the rest of the buffer
	for( int i = 1; i < height; ++i )
	{
		memcpy( buffer + i * 3 * width, buffer, 3 * width );
	}

	PyObject* ret = NULL;
	ret = PyByteArray_FromStringAndSize( buffer, 3 * width * height );
	CCP_DELETE[] buffer;
	return ret;
}

static PyObject* PyGetKelvinTintBuffer( PyObject* self, PyObject* args )
{
	int width, height;
	if( !PyArg_ParseTuple( args, "ii", &width, &height ) )
	{
		return NULL;
	}

	char* buffer = CCP_NEW( "PyGetKelvinTintBuffer" ) char[3 * width * height];

	const float deltaT = 1.0f / ( static_cast<float>( width ) - 1.0f );
	float curT = 0.0f;

	// Render one row of the bitmap
	char* ptr = buffer;
	for( int i = 0; i < width; ++i )
	{
		ptr[3 * i] = static_cast<char>( ( 1.0f - curT ) * 255.0 );
		ptr[3 * i + 1] = static_cast<char>( curT * 255.0 );
		ptr[3 * i + 2] = static_cast<char>( ( 1.0f - curT ) * 255.0 );
		curT += deltaT;
	}

	// Copy the row into the rest of the buffer
	for( int i = 1; i < height; ++i )
	{
		memcpy( buffer + i * 3 * width, buffer, 3 * width );
	}

	PyObject* ret = NULL;
	ret = PyByteArray_FromStringAndSize( buffer, 3 * width * height );
	CCP_DELETE[] buffer;
	return ret;
}

MAP_FUNCTION( "TriColorFromKelvin", PyTriColorFromKelvin, "Converts a Kelvin color temperature to an RGBA color.\n\n"
														  ":param temperature:		Kelvin color temperature,\n"
														  ":type temperature: float\n"
														  ":param tint:			a tint value (not used),\n"
														  ":type tint: float\n"
														  ":param whiteBalance:	white balance standard illuminant"
														  ":type whiteBalance: int\n"
														  ":rtype: (float, float, float, float)" );

MAP_FUNCTION( "GetKelvinTemperatureBuffer", PyGetKelvinTemperatureBuffer, "Generates a buffer of Kelvin temperatures.\n\n"
																		  ":param width:			width of the buffer in pixels,\n"
																		  ":type width: int\n"
																		  ":param height:			height of the buffer in pixels,\n"
																		  ":type height: int\n"
																		  ":param whiteBalance:	white balance standard illuminant\n"
																		  ":type whiteBalance: int\n"
																		  ":rtype: bytearray" );

MAP_FUNCTION( "GetKelvinTintBuffer", PyGetKelvinTintBuffer, "Generates a buffer of Kelvin tint values.\n\n"
															":param  width:		width of the buffer in pixels,\n"
															":type width: int\n"
															":param  height:		height of the buffer in pixels,\n"
															":type height: int\n"
															":rtype: bytearray" );

#endif

const Be::ClassInfo* Tr2KelvinColor::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2KelvinColor, "" )
		MAP_INTERFACE( Tr2KelvinColor )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "temperature", m_temperature, "Temperature in Kelvin", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "tint", m_tint, "Tint value", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "whiteBalance", m_whiteBalance, "Reference value for white balancing", Be::ENUM | Be::READWRITE | Be::PERSIST, Tr2StandardIlluminantChooser )

	EXPOSURE_END()
}