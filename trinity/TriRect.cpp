#include "StdAfx.h"
#include "TriRect.h"

/////////////////////////////////////////////////////////////////////////////////////////
// IPythonMethods Impl
/////////////////////////////////////////////////////////////////////////////////////////

#if BLUE_WITH_PYTHON
void TriRect::Destroy()
{
	//delete this;
}

PyObject* TriRect::GetAttr( const char* name, bool* handled	)
{
	return 0;
}

bool TriRect::SetAttr( const char* name, PyObject* v, bool* handled )
{
	return true;
}

PyObject* TriRect::Repr( bool* handled )
{
	*handled = true;
	char buf[120];
	sprintf_s(buf, "<Instance of 'TriRect' (left=%d top=%d right=%d bottom=%d)>", left, top, right, bottom);
	return PyUnicode_FromString(buf);
}


TriRect::TriRect(IRoot* lockobj)	
{
	SetDimentions(0,0,0,0);
}

TriRect::~TriRect()
{
}

void TriRect::SetDimentions( int32_t _left, int32_t _top, int32_t _right, int32_t _bottom )
{
	left = _left;
	top = _top;
	right = _right;	
	bottom = _bottom;
}

void TriRect::SetRect( const ::Tr2Rect* c )
{
	left = c->left;
	top = c->top;
	right = c->right;	
	bottom = c->bottom;
}

const ::Tr2Rect* TriRect::GetRect() const
{
	return this;
}

::Tr2Rect* TriRect::CopyRect( ::Tr2Rect* in ) const
{
	// would this work?
	//return &(*in = *this);
	*in = *this;
	return in;
}

::Tr2Rect* TriRect::Rect()
{
	return this;
}

void TriRect::PySetRect( Be::Optional<int32_t> l, Be::Optional<int32_t> t, Be::Optional<int32_t> r, Be::Optional<int32_t> b ) 
{
	if( l.IsAssigned() )
	{
		left = l;
	}
	if( t.IsAssigned() )
	{
		top = t;
	}
	if( r.IsAssigned() )
	{
		right = r;
	}
	if( b.IsAssigned() )
	{
		bottom = b;
	}
}


#endif
