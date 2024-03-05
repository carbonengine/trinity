/* 
	*************************************************************************

	ITriColor.h

	Author:    Hilmar Veigar Pétursson
	Created:   November 2000
	OS:        Win32
	Project:   Trinity

	Description:   

		Yeap


	Dependencies:

		DirectX 9.0, Probably more, ytbd.

	(c) CCP 2000

	*************************************************************************
*/


#ifndef _ITRICOLOR_H_
#define _ITRICOLOR_H_

#define TRICOLOR_NOALPHA -9.0f

struct ITriVector;

BLUE_INTERFACE( ITriColor ) : public IRoot	
{

	// Construction and data access
	virtual void SetRGB(
		float r, 
		float g, 
		float b,
		float a
		) = 0;

	// if 'alpha' is TRICOLOR_NOALPHA, it's left as is
	virtual void SetHSV(
		float hue, 
		float saturation, 
		float value, 
		float alpha
		) = 0;

	virtual void SetColor(
		const Color* c
		) = 0;

	virtual const ::Color* GetColor(
		) const =0;

	virtual ::Color* CopyColor(
		::Color* in
		) const =0;

	virtual ::Color* Color(
		) = 0;
	
	// if 'alpha' is NULL then nothing is done to that pointer
	virtual void GetHSV(
		float* hue, 
		float* saturation, 
		float* value, 
		float* alpha
		) = 0;

	// Math functions
	virtual void Scale(
		float s
		) = 0;

	// Turns a normalized vector into RGBA form. Used to encode vectors into a height map. 
	//virtual void SetVector(
	//	ITriVector* vec,
	//	float height
	//	) = 0;
};

#endif