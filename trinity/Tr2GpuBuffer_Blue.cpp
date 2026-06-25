// Copyright © 2013 CCP ehf.

#include "StdAfx.h"
#include "Tr2GpuBuffer.h"

BLUE_DEFINE_INTERFACE( ITr2GpuBuffer );
BLUE_DEFINE( Tr2GpuBuffer );

Be::VarChooser Tr2GpuBufferCreationFlagChooser[] = {
	{ "CPU_WRITABLE",
	  BeCast( Tr2GpuBuffer::CPU_WRITABLE ),
	  "Can the buffer be locked with write-only access" },
	{ "GPU_WRITABLE",
	  BeCast( Tr2GpuBuffer::GPU_WRITABLE ),
	  "Is the buffer used for GPU write access" },
	{ "DRAW_INDIRECT",
	  BeCast( Tr2GpuBuffer::DRAW_INDIRECT ),
	  "Is the buffer used for indirect draw calls" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX(
	"Tr2GpuBufferFlags",
	Tr2GpuBuffer::CreationFlag,
	Tr2GpuBufferCreationFlagChooser,
	ENUM_REG_ENUM_OBJECT_ON_MODULE );

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

	Tr2GpuBuffer* pThis = BluePythonCast<Tr2GpuBuffer*>( self );
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

	return PyVerCompat::ToPyBytes( static_cast<const char*>( data ), buffer->GetSize() );
}
}
#endif

const Be::ClassInfo* Tr2GpuBuffer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GpuBuffer, "" )

		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2GpuBuffer )
		MAP_INTERFACE( Tr2GpuBuffer )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			__init__,
			3,
			":param count: number of elements in the buffer\n"
			":param format: buffer pixel format (trinity.PIXEL_FORMAT.foo)\n"
			":param creationFlags: combination of trinity.Tr2GpuBufferFlags\n" )
		MAP_ATTRIBUTE(
			"count",
			m_count,
			"Number of elements in the buffer",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"format",
			m_format,
			"Buffer pixel format (trinity.PIXEL_FORMAT.foo)",
			Be::READWRITE | Be::ENUM | Be::PERSIST | Be::NOTIFY,
			Tr2RenderContextEnum_PixelFormat_Chooser )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"creationFlags",
			m_creationFlags,
			"Miscellaneous for creating a buffer",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY,
			Tr2GpuBufferCreationFlagChooser )
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
