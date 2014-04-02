/* 
	*************************************************************************************

	TriRect.h

	Author:    Hilmar Veigar Pétursson
	Created:   May 2002
	OS:        Win32
	Project:   Trinity

	Description:   

		See TRIRECT_Description


	Dependencies:

		DirectX 9.0, Blue

	(c) CCP 2000

	*************************************************************************************
*/


#ifndef _TRIRECT_H_
#define _TRIRECT_H_

#define TRIRECT_Description \
"Simple mapping of Rect over to Blue\r\n\
function"

#include "include/ITriRect.h"
#include "include/Rect.h"

#if BLUE_WITH_PYTHON

BLUE_CLASS( TriRect ) :
	public ITriRect,
	public IPythonMethods,
	public Rect
{
public:
	EXPOSE_TO_BLUE();

	TriRect(IRoot* lockobj = NULL);
	~TriRect();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriRect
	/////////////////////////////////////////////////////////////////////////////////////
	void SetDimentions(
		int32_t _left, 
		int32_t _top, 
		int32_t _right, 
		int32_t _bottom
		);

	void SetRect(
		const ::Rect* c
		);

	const ::Rect* GetRect(
		) const;

	::Rect* CopyRect(
		::Rect* in
		) const;

	::Rect* Rect(
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
	void PySetRect(
		Be::Optional<int32_t> l,
		Be::Optional<int32_t> t,
		Be::Optional<int32_t> r,
		Be::Optional<int32_t> b );
};
TYPEDEF_BLUECLASS(TriRect);

#endif

#endif
