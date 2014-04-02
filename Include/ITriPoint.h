/* 
	*************************************************************************

	ITriPoint.h

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


#ifndef _ITRIPOINT_H_
#define _ITRIPOINT_H_

#include "Point.h"

BLUE_INTERFACE( ITriPoint ) : public IRoot	
{
	// Construction and data access
	virtual void SetXY(
		int32_t _x, 
		int32_t _y
		) = 0;

	virtual void SetPoint(
        const ::Point* c
		) = 0;

	virtual const ::Point* GetPoint(
		) const =0;

	virtual ::Point* CopyPoint(
        ::Point* in
		) const =0;

	virtual ::Point* Point(
		) = 0;
};

#endif