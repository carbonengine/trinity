// Copyright © 2000 CCP ehf.

#ifndef _TRIRECT_H_
#define _TRIRECT_H_

#include "include/Rect.h"

#if BLUE_WITH_PYTHON

BLUE_CLASS( TriRect ) :
	public IPythonMethods,
	public Tr2Rect
{
public:
	EXPOSE_TO_BLUE();

	TriRect( IRoot* lockobj = NULL );
	~TriRect();

	/////////////////////////////////////////////////////////////////////////////////////
	// IPythonMethods
	void Destroy();
	PyObject* GetAttr( const char* name, bool* handled );
	bool SetAttr( const char* name, PyObject* v, bool* handled );
	PyObject* Repr( bool* handled );

	void SetDimentions( int32_t _left, int32_t _top, int32_t _right, int32_t _bottom );
	void SetRect( const ::Tr2Rect* c );
	const ::Tr2Rect* GetRect() const;
	::Tr2Rect* CopyRect( ::Tr2Rect * in ) const;
	::Tr2Rect* Rect();

	void PySetRect( Be::Optional<int32_t> l, Be::Optional<int32_t> t, Be::Optional<int32_t> r, Be::Optional<int32_t> b );
};
TYPEDEF_BLUECLASS( TriRect );

#endif

#endif
