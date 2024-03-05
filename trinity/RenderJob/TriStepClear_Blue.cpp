#include "StdAfx.h"
#include "TriStepClear.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepClear );

#if BLUE_WITH_PYTHON
// We can't use MAP_METHOD_AND_WRAP because we need to support optional arguments
PyObject* TriStepClear::py__init__( PyObject* self, PyObject* args )
{
	TriStepClear* pThis = BluePythonCast<TriStepClear*>( self );
	// the above can't fail on type, self is always of type TriStepClear

	PyObject* color = NULL;
	PyObject* depth = NULL;
	PyObject* stencil = NULL;

	if( !PyArg_ParseTuple( args, "|OOO", &color, &depth, &stencil) )
	{
		return NULL;
	}

	if( !color || color == Py_None )
	{
		pThis->m_isColorCleared = false;
	}
	else
	{
		if( !BlueExtractArgument( color, pThis->m_color, 1 ) )
		{
			return NULL;
		}
	}

	if( !depth || depth == Py_None )
	{
		pThis->m_isDepthCleared = false;
	}
	else
	{
		if( !BlueExtractArgument( depth, pThis->m_depth, 2 ) )
		{
			return NULL;
		}
	}

	if( !stencil || stencil == Py_None )
	{
		pThis->m_isStencilCleared = false;
	}
	else
	{
		if( !BlueExtractArgument( stencil, pThis->m_stencil, 3 ) )
		{
			return NULL;
		}
		pThis->m_isStencilCleared = true;
	}

	Py_RETURN_NONE;
}
#endif

const Be::ClassInfo* TriStepClear::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepClear, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepClear )

		MAP_ATTRIBUTE( "color", m_color, "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depth", m_depth, "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "stencil", m_stencil, "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isColorCleared", m_isColorCleared, "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isDepthCleared", m_isDepthCleared, "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isStencilCleared", m_isStencilCleared, "na", Be::READWRITE | Be::PERSIST )

		MAP_METHOD(
			"__init__", 
			py__init__,
			"Creates a render step that performs a Clear() on the currently set"
			"\nRT back-buffer / depth-stencil. When optional arguments are not set"
			"\nor are set to None, they will not be cleared."
			"\n:param color: (r,g,b,a) or None (default None)\n"
			"\n:type color: Optional[(float, float, float, float) | None]"
			"\n:param depth: float or None (default None)"
			"\n:type depth: Optional[float | None]"
			"\n:param stencil: uint or None (default None)"
			"\n:type stencil: Optional[int | None]"
			)

	EXPOSURE_CHAINTO( TriRenderStep )
}