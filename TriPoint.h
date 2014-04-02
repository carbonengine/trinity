/* 
	*************************************************************************************

	TriPoint.h

	Author:    Hilmar Veigar Pétursson
	Created:   May 2002
	OS:        Win32
	Project:   Trinity

	Description:   

		See TRIPOINT_Description


	Dependencies:

		DirectX 9.0, Blue

	(c) CCP 2000

	*************************************************************************************
*/


#ifndef _TRIPOINT_H_
#define _TRIPOINT_H_

#define TRIPOINT_Description \
"Simple mapping of Win32 POINT over to Blue\r\n\
function"

#include "include/ITriPoint.h"


#if BLUE_WITH_PYTHON
class TriPoint :
	public ITriPoint,
	public IPythonMethods,
	public Point
{
public:
	EXPOSE_TO_BLUE();

	TriPoint(IRoot* lockobj = NULL);
	~TriPoint();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriPoint
	/////////////////////////////////////////////////////////////////////////////////////
	void SetXY(
		int32_t _x,
		int32_t _y
		);

	void SetPoint(
        const ::Point* c
		);

	const ::Point* GetPoint(
		) const;

	::Point* CopyPoint(
        ::Point* in
		) const;

	::Point* Point(
		);
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IPythonMethods
	/////////////////////////////////////////////////////////////////////////////////////
	void Destroy(
		);

	PyObject* GetAttr( 
		const char* name, 
		bool* handled
		);

	bool SetAttr(
		const char* name,
		PyObject* v,
		bool* handled
		);

	PyObject* Repr(
		bool* handled
		);

public:
	void PySetPoint( Be::Optional<int32_t> _x, Be::Optional<int32_t> _y );
};
TYPEDEF_BLUECLASS(TriPoint);

#endif

#endif
