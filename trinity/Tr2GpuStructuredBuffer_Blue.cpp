#include "StdAfx.h"
#include "Tr2GpuStructuredBuffer.h"

BLUE_DEFINE( Tr2GpuStructuredBuffer );

#if BLUE_WITH_PYTHON

using namespace Tr2RenderContextEnum;

namespace
{

	PyObject* PyGetData( PyObject* self, PyObject* args )
	{
		if( !PyArg_ParseTuple( args, "" ) )
		{
			return nullptr;
		}

		Tr2GpuStructuredBuffer* pThis = BluePythonCast<Tr2GpuStructuredBuffer*>( self );
		if( !pThis )
		{
			return nullptr;
		}
		if( !pThis->IsValid() )
		{
            PyErr_SetString( PyExc_RuntimeError, "buffer is not valid" );
			return nullptr;
		}

		auto buffer = pThis->GetGpuBuffer( 0 );

		USE_MAIN_THREAD_RENDER_CONTEXT();

		const void* data = nullptr;
		auto hr = buffer->MapForReading( data, renderContext );
		if( FAILED( hr ) )
		{
            PyErr_SetString( BeGetException( hr ), BeGetErrorMessage( hr ) );
			return nullptr;
		}
		ON_BLOCK_EXIT( [&]() { buffer->UnmapForReading( renderContext ); } );

		return PyString_FromStringAndSize( static_cast<const char*>( data ), buffer->GetSize() );
	}
}
#endif

const Be::ClassInfo* Tr2GpuStructuredBuffer::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2GpuStructuredBuffer, "" )

		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2GpuBuffer )
		MAP_INTERFACE( Tr2GpuStructuredBuffer )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			3,
			":param count: number of elements in the buffer\n"
			":param stride: size of buffer element in bytes\n"
			":param cpuWritable: if CPU should have write access to the buffer\n" )
		MAP_ATTRIBUTE( 
			"count", 
			m_count, 
			"Number of elements in the buffer", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( 
			"stride", 
			m_stride, 
			"Size of buffer element in bytes", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( 
			"creationFlags", 
			m_creationFlags, 
			"If CPU should have write access to the buffer", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_PROPERTY_READONLY( 
			"isValid",
			IsValid,
			"Is the underlying GPU object valid" )

#if BLUE_WITH_PYTHON
		MAP_METHOD(
			"DebugGetData",
			PyGetData,
			"Returns contents of the buffer as a string.\n"
			"WARNING: the method is slow and should not be used for anything but debugging.\n"
			":rtype: str" )
#endif
		EXPOSURE_END()
}
