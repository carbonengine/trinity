////////////////////////////////////////////////////////////
//
//    Created:   February 2013
//    Copyright: CCP 2013
//

#include "StdAfx.h"
#include "TriStepClearUav.h"
#include "include/ITr2GpuBuffer.h"

BLUE_DEFINE( TriStepClearUav );

#if BLUE_WITH_PYTHON
// --------------------------------------------------------------------------------------
// Description:
//   Python initializer function for TriStepClearUav. We can't use MAP_METHOD_AND_WRAP 
//   because we need to support optional arguments.
// Arguments:
//   self - This pointer
//   args - Arguments tuple
// Return Value:
//   None or NULL on error
// --------------------------------------------------------------------------------------
PyObject* TriStepClearUav::py__init__( PyObject* self, PyObject* args )
{
	TriStepClearUav* pThis = BluePythonCast<TriStepClearUav*>( self );

	PyObject* buffer = NULL;
	PyObject* values = NULL;

	if( !PyArg_ParseTuple( args, "|OO", &buffer, &values ) )
	{
		return NULL;
	}

	if( buffer && buffer != Py_None )
	{
		ITr2GpuBuffer* uavBuffer = BluePythonCast<ITr2GpuBuffer*>( buffer );
		if( uavBuffer )
		{
			pThis->m_buffer = uavBuffer;
		}
		else
		{
			PyErr_SetString( PyExc_TypeError, "Expected an ITr2GpuBuffer argument." );
			return NULL;
		}
	}
	if( values )
	{
		if( !PyTuple_Check( values ) || PyTuple_Size( values ) != 4 )
		{
			PyErr_SetString( PyExc_TypeError, "Expected a 4-tuple." );
			return NULL;
		}
		if( PyFloat_Check( PyTuple_GetItem( values, 0 ) ) )
		{
			pThis->m_clearWithFloat = true;
			pThis->m_floatValue.x = float( PyFloat_AsDouble( PyTuple_GetItem( values, 0 ) ) );
			pThis->m_floatValue.y = float( PyFloat_AsDouble( PyTuple_GetItem( values, 1 ) ) );
			pThis->m_floatValue.z = float( PyFloat_AsDouble( PyTuple_GetItem( values, 2 ) ) );
			pThis->m_floatValue.w = float( PyFloat_AsDouble( PyTuple_GetItem( values, 3 ) ) );
		}
		else
		{
			pThis->m_clearWithFloat = false;
			pThis->m_uintValue[0] = uint32_t( PyInt_AsSsize_t( PyTuple_GetItem( values, 0 ) ) );
			pThis->m_uintValue[1] = uint32_t( PyInt_AsSsize_t( PyTuple_GetItem( values, 1 ) ) );
			pThis->m_uintValue[2] = uint32_t( PyInt_AsSsize_t( PyTuple_GetItem( values, 2 ) ) );
			pThis->m_uintValue[3] = uint32_t( PyInt_AsSsize_t( PyTuple_GetItem( values, 3 ) ) );
		}
	}

	Py_RETURN_NONE;
}
#endif

const Be::ClassInfo* TriStepClearUav::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepClearUav, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepClearUav )

		MAP_ATTRIBUTE( "buffer", m_buffer, "buffer to clear", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "clearWithFloat", m_clearWithFloat, "clear with float values?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "floatValue", m_floatValue, "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "bitValue0", m_uintValue[0], "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "bitValue1", m_uintValue[1], "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "bitValue2", m_uintValue[2], "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "bitValue3", m_uintValue[3], "na", Be::READWRITE | Be::PERSIST )

		MAP_METHOD(
			"__init__", 
			py__init__,
			"Creates a render step that clears a writable UAV buffer with a value.\n"
			":param buffer: - Tr2UavBuffer to clear\n"
			":type buffer: Optional[ITr2GpuBuffer]\n"
			":param clearValue: 4-tuple of values to clear\n"
			":type clearValue: Optional[(float, float, float, float) | (int, int, int, int)]" )

	EXPOSURE_CHAINTO( TriRenderStep )
}