/* 
	*************************************************************************

	ITriRect.h

	Author:    Hilmar Veigar Pétursson
	Created:   May 2002
	OS:        Win32
	Project:   Trinity

	Description:   

		Yeap


	Dependencies:

		DirectX 9.0, Probably more, ytbd.

	(c) CCP 2000

	*************************************************************************
*/


#ifndef _ITRIRECT_H_
#define _ITRIRECT_H_

struct Rect;

BLUE_INTERFACE( ITriRect ) : public IRoot	
{
	// Construction and data access
	virtual void SetDimentions(
		int32_t _left, 
		int32_t _top, 
		int32_t _right, 
		int32_t _bottom
		) = 0;

	virtual void SetRect(
		const ::Rect* c
		) = 0;

	virtual const ::Rect* GetRect(
		) const =0;

	virtual ::Rect* CopyRect(
		::Rect* in
		) const =0;

	virtual ::Rect* Rect(
		) = 0;
};

#endif