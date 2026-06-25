// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepSetRenderState.h"
#include "TriConstants.h"

BLUE_DEFINE( TriStepSetRenderState );

#if BLUE_WITH_PYTHON
static PyObject* py__init__( PyObject* self, PyObject* args )
{
	TriStepSetRenderState* pThis = BluePythonCast<TriStepSetRenderState*>( self );

	PyObject* pyState = NULL;
	PyObject* pyStateValue = NULL;

	if( !PyArg_ParseTuple( args, "|OO", &pyState, &pyStateValue ) )
	{
		return NULL;
	}

	if( pyState && pyStateValue )
	{
		unsigned int state = 0;
		if( !BlueExtractArgument( pyState, state, 0 ) )
		{
			return NULL;
		}
		else
		{
			unsigned int stateValue = 0;
			if( !BlueExtractArgument( pyStateValue, stateValue, 0 ) )
			{
				return NULL;
			}
			else
			{
				pThis->SetStateAndValue( state, stateValue );
			}
		}
	}
	else if( pyState || pyStateValue )
	{
		PyErr_SetString( PyExc_Exception, "You must set both the state and the value." );
		return NULL;
	}

	Py_RETURN_NONE;
}
#endif

const Be::ClassInfo* TriStepSetRenderState::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepSetRenderState, "" )
		MAP_INTERFACE( TriStepSetRenderState )
		MAP_INTERFACE( TriRenderStep )

		MAP_ATTRIBUTE_WITH_CHOOSER( "state", m_state, "", Be::READWRITE | Be::PERSIST | Be::ENUM, TriD3DRenderState )
		MAP_ATTRIBUTE( "value", m_value, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD(
			"__init__",
			py__init__,
			"Creates a render step to set a specific D3D state"
			"\nIf one of the optional arguments is set, then both must be"
			"\n:param state: A TriD3DRenderState value (unsigned int)"
			"\n:type state: int"
			"\n:param stateValue: A valid value to set (unsigned int)"
			"\n:type stateValue: int" )

	EXPOSURE_CHAINTO( TriRenderStep )
}
