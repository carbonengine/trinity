// Copyright © 2000 CCP ehf.

#ifndef _TRICOLOR_H_
#define _TRICOLOR_H_

#define TRICOLOR_Description \
	"Simple mapping of Color over to Blue, with the addition of HSV \r\n\
function"

#include "ITriColor.h"

#pragma warning( disable : 4275 )

#if BLUE_WITH_PYTHON
class TriColor : public ITriColor,
				 public IPythonMethods,
				 public Color
{
public:
	EXPOSE_TO_BLUE();

	TriColor( IRoot* lockobj = NULL );
	~TriColor();



	/////////////////////////////////////////////////////////////////////////////////////
	// ITriColor
	/////////////////////////////////////////////////////////////////////////////////////
	void SetRGB(
		float r,
		float g,
		float b,
		float a = TRICOLOR_NOALPHA );

	void SetHSV(
		float hue,
		float saturation,
		float value,
		float alpha = TRICOLOR_NOALPHA );

	void SetVector(
		const Vector3* in,
		float alpha );

	void GetHSV(
		float* hue,
		float* saturation,
		float* value,
		float* alpha = NULL );

	void SetColor(
		const ::Color* c );

	const ::Color* GetColor() const;

	::Color* CopyColor(
		::Color* in ) const;

	::Color* Color();


	void Scale(
		float s );

	/////////////////////////////////////////////////////////////////////////////////////
	// IPythonMethods
	/////////////////////////////////////////////////////////////////////////////////////
	void Destroy();

	PyObject* GetAttr(
		const char* name,
		bool* handled );

	bool SetAttr(
		const char* name,
		PyObject* v,
		bool* handled );

	PyObject* Repr(
		bool* handled );

public:
	void Py__init__( float r, float g, float b, float a );
	void PySetRGB( float r, float g, float b, Be::Optional<float> a );
	void PySetHSV( float hue, float saturation, float value, Be::Optional<float> a );
	void PySetVector( ITriVector* vector, float alpha );
	Vector3 PyGetHSV();
	void PyFromInt( int color );
	int32_t PyAsInt();
	PyObject* PyAdd( PyObject* args );
	PyObject* PyLerp( PyObject* args );
};
TYPEDEF_BLUECLASS( TriColor );

#endif
#endif
