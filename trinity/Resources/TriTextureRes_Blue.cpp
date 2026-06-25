// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriTextureRes.h"
#include "Tr2RenderTarget.h"
#include "Tr2HostBitmap.h"
#include "TexturePipeline/Tr2TexturePipeline.h"


BLUE_DEFINE_INTERFACE( ITr2TextureProvider );

BLUE_DEFINE( TriTextureRes );

using namespace Tr2RenderContextEnum;

#if BLUE_WITH_PYTHON
static PyObject* PyInit( PyObject* self, PyObject* args )
{
	TriTextureRes* pThis = BluePythonCast<TriTextureRes*>( self );


	if( PyTuple_Size( args ) >= 4 )
	{
		// Option 1: constructor that makes a new, undefined-contents TriTextureRes of a given
		// size, presumably to use locking or UpdateSubResource later on.
		unsigned width = 0, height = 0, mipCount = 0, format = 0, usage = USAGE_CPU_READ;

		if( !PyArg_ParseTuple( args, "iiii|i", &width, &height, &mipCount, &format, &usage ) )
		{
			PyErr_SetString( PyExc_RuntimeError, "Excpected width, height, mipCount, format, optional usage" );
			return nullptr;
		}

		if( width && height && mipCount && format )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			pThis->Create( width,
						   height,
						   mipCount,
						   static_cast<PixelFormat>( format ),
						   static_cast<BufferUsageFlags>( usage ),
						   renderContext );
		}
		Py_RETURN_NONE;
	}

	if( PyTuple_Size( args ) == 1 )
	{
		// Option 2: a live view of a (readable) renderTarget
		PyObject* pySurface = NULL;

		if( !PyArg_ParseTuple( args, "|O", &pySurface ) )
		{
			PyErr_SetString( PyExc_RuntimeError, "Excpected Tr2RenderTarget" );
			return nullptr;
		}

		if( pySurface )
		{
			Tr2RenderTarget* renderTarget = nullptr;
			if( BluePythonCast<Tr2RenderTarget*>( pySurface ) && BlueExtractArgument( pySurface, renderTarget, 0 ) )
			{
				pThis->SetTextureFromRT( renderTarget );
				Py_RETURN_NONE;
			}
		}
		Py_RETURN_NONE;
	}

	// Option 3: empty constructor, do nothing.

	Py_RETURN_NONE;
}

static PyObject* PythonSave( PyObject* self, PyObject* args, bool async )
{
	TriTextureRes* pThis = BluePythonCast<TriTextureRes*>( self );

	PyObject* file = nullptr;

	if( !PyArg_ParseTuple( args, "O", &file ) )
	{
		PyErr_SetString( PyExc_TypeError, "Expected a filepath string" );
		return nullptr;
	}

	std::wstring wcstr;
	std::string cstr;
	bool OK = false;
	if( PyUnicode_Check( file ) && BlueExtractArgument( file, wcstr, 1 ) )
	{
		if( async )
		{
			OK = pThis->SaveAsync( wcstr.c_str() );
		}
		else
		{
			OK = pThis->Save( wcstr.c_str() );
		}
	}
#if PY_MAJOR_VERSION == 2
	else if( PyString_Check( file ) && BlueExtractArgument( file, cstr, 1 ) )
	{
		if( async )
		{
			OK = pThis->SaveAsync( CA2W( cstr.c_str() ) );
		}
		else
		{
			OK = pThis->Save( CA2W( cstr.c_str() ) );
		}
	}
#endif
	else
	{
		PyErr_SetString( PyExc_TypeError, "Expected a filepath string" );
		return nullptr;
	}

	return PyBool_FromLong( OK );
}

static PyObject* PySaveAsync( PyObject* self, PyObject* args )
{
	return PythonSave( self, args, true );
}

static PyObject* PySave( PyObject* self, PyObject* args )
{
	return PythonSave( self, args, false );
}

#endif

const Be::ClassInfo* TriTextureRes::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriTextureRes, "" )
		MAP_INTERFACE( ITriTextureRes )
		MAP_INTERFACE( TriTextureRes )
		MAP_INTERFACE( ITr2TextureProvider )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_ICACHEABLE_METHODS()

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"Name for logging/debugging",
			Be::READ | Be::WRITE )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"type",
			m_type,
			"",
			Be::READ | Be::PERSIST | Be::ENUM,
			Tr2RenderContextEnum_TextureType_Chooser )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"format",
			m_format,
			"Platform independent Pixel format (trinity.PIXEL_FORMAT.foo)",
			Be::READ | Be::ENUM,
			Tr2RenderContextEnum_PixelFormat_Chooser )
		MAP_ATTRIBUTE(
			"width",
			m_width,
			"Width of loaded image in pixels.",
			Be::READ )
		MAP_ATTRIBUTE(
			"height",
			m_height,
			"Height of loaded image in pixels.",
			Be::READ )
		MAP_ATTRIBUTE(
			"depth",
			m_volumeDepth,
			"Depth of loaded image in pixels.",
			Be::READ )
		MAP_PROPERTY_READONLY(
			"mipCount",
			GetTrueMipCount,
			"Number of mip levels in this resource" )
		MAP_ATTRIBUTE(
			"arraySize",
			m_arraySize,
			"Number of elements in texture array.",
			Be::READ )
		MAP_PROPERTY_READONLY(
			"mipCount",
			GetTrueMipCount,
			"Number of mip levels in this resource" )
		MAP_PROPERTY_READONLY( "multiSampleType", GetMsaaType, "" );
		MAP_PROPERTY_READONLY( "multiSampleQuality", GetMsaaQuality, "" );
		MAP_ATTRIBUTE(
			"averageColor",
			m_averageColor,
			"Average color of the image",
			Be::READ )

		MAP_ATTRIBUTE( "lodEnabled", m_lodEnabled, "Is LOD enabled for this texture\n:jessica-group: LOD", Be::READ )
		MAP_PROPERTY_READONLY( "hadLodRequests", HadLodRequests, "If the textued received any LOD requests\n:jessica-group: LOD" )
		MAP_ATTRIBUTE( "originalResolution", m_originalResolution, "Max width or height of the original texture size\n:jessica-group: LOD", Be::READ )
		MAP_ATTRIBUTE( "gpuMip", m_gpuMip, "Largest mip level loaded into GPU memory\n:jessica-group: LOD", Be::READ )
		MAP_ATTRIBUTE( "cpuMip", m_cpuMip, "Largest mip level loaded into CPU memory\n:jessica-group: LOD", Be::READ )


		MAP_METHOD(
			"SaveAsync",
			PySaveAsync,
			"Asynchronously save the contents of this TriTextureRes.\n"
			"Use WaitForSave if you want to wait -- or use the synchronous\n"
			"version Save()\n"
			":para path: absolute system file path and name with extension.\n"
			"        The format will be decided from the extension (dds, jpg, ...)\n"
			":type path: basestring\n"
			":rtype: bool" )

		MAP_METHOD(
			"Save",
			PySave,
			"Save the contents of this TriTextureRes.\n"
			":param path: absolute system file path and name with extension.\n"
			"        The format will be decided from the extension (dds, jpg, ...)\n"
			":type path: basestring\n"
			":rtype: bool" )

		MAP_METHOD_AND_WRAP( "PrepareResources", PrepareResources, "Initiates load of resources from disk. Returns immediately." )
		MAP_METHOD_AND_WRAP( "IsPrepared", IsPrepared, "True if preparation has completed (not necessarily successfully)" )
		MAP_METHOD_AND_WRAP( "IsLoading", IsLoading, "True if the resource is in the process of loading" )
		MAP_METHOD_AND_WRAP( "IsGood", IsGood, "True if preparation completed successfully" )
		MAP_METHOD_AND_WRAP( "IsSaving", IsSaving, "Is an asynchronous save in progress?" );
		MAP_METHOD_AND_WRAP( "IsSaveCompleted", IsSaveCompleted, "True if the save operation is completed" )
		MAP_METHOD_AND_WRAP( "IsSaveSucceeded", IsSaveSucceeded, "True if the save operation completed successfully" )
		MAP_METHOD_AND_WRAP( "WaitForSave", WaitForSave, "Waits for SaveAsync() to complete. Alternatively use Save()" )

#if 0
		MAP_METHOD_AND_WRAP
		( 
			"DebugCountMipSkipOwners", 
			DebugCountMipSkipOwners, 
			""
		)
#endif

		MAP_ATTRIBUTE( "cutoutX", m_cutoutX, "X coordinate of cutout rectangle, range 0...1", Be::READWRITE )
		MAP_ATTRIBUTE( "cutoutY", m_cutoutY, "Y coordinate of cutout rectangle, range 0...1", Be::READWRITE )
		MAP_ATTRIBUTE( "cutoutWidth", m_cutoutWidth, "width of cutout rectangle, range 0...1", Be::READWRITE )
		MAP_ATTRIBUTE( "cutoutHeight", m_cutoutHeight, "height of cutout rectangle, range 0...1", Be::READWRITE )

		MAP_ATTRIBUTE( "wrappedRenderTarget", m_wrappedRenderTarget, "Live view renderTarget being wrapped with SetFromRenderTarget", Be::READ );

		MAP_PROPERTY_READONLY( "srvIndex", GetSrvIndexInHeap, "" )

		MAP_METHOD_AND_WRAP(
			"SetFromRenderTarget",
			SetTextureFromRT,
			"Set a texture from a renderTarget. User needs to keep the RT alive.\n"
			"The TriTextureRes will now represent a live view on the RT to be textured from.\n"
			"\n:param rt: render target" )

		MAP_METHOD_AND_WRAP(
			"CreateAndCopyFromRenderTarget",
			CreateAndCopyFromRenderTarget,
			"Create a new TriTextureRes with the dimensions and pixelFormat from the specified\n"
			"renderTarget.  A copy of the contents is made.\n"
			"\n:param rt: render target" )

		MAP_METHOD_AND_WRAP(
			"CreateEmptyTexture",
			CreateEmptyTexture,
			"Creates a new texture of the given size and fills it with zeroes\n"
			":param width: texture width\n"
			":param height: texture height\n"
			":param mipCount: number of mip levels\n"
			":param format: pixel format (trinity.PIXEL_FORMAT)" )

		MAP_METHOD_AND_WRAP(
			"CreateFromHostBitmap",
			CreateFromHostBitmap,
			"Creates a new TriTextureRes width the dimensionts and pixelFormat from specified\n"
			"host bitmap and copies the contents of the host bitmap into the texture.\n"
			":param hostBitmap: valid Tr2HostBitmap" )

		MAP_METHOD_AND_WRAP(
			"CreateFromTexture",
			CreateFromTexture,
			"Creates a new TriTextureRes with a copy of provided TriTextureRes argument.\n"
			":param texture: valid TriTextureRes" )

		MAP_METHOD_AND_WRAP(
			"HasALObject",
			HasALObject,
			"Returns True iff TriTextureRes contains a reference to passed AL object ID.\n"
			"Used for debugging along with trinity.GetLiveALResources.\n"
			":param alType: AL object type (trinity.AL_OBJECT_TYPE)\n"
			":param alObject: AL object ID" )

		MAP_METHOD(
			"__init__",
			PyInit,
			"Constructs a new texture. There are three possible overrides:\n"
			"(1) TriTextureRes() - constructs an empty texture\n"
			"(2) TriTextureRes(other) - wraps existing render target\n"
			"(3) TriTextureRes(width, height, mipCount, format [, usage]) - creates a new unfilled texture\n"
			":param otherOrWidth: render target / depth stencil for the (2) override or width for (3) override\n"
			":type otherOrWidth: Optional[Tr2RenderTarget | int]\n"
			":param height: texture height\n"
			":type height: Optional[int]\n"
			":param mipCount: number of mip levels\n"
			":type mipCount: Optional[int]\n"
			":param format: pixel format (trinity.PIXEL_FORMAT)\n"
			":type format: Optional[int]\n"
			":param usage: texture usage (trinity.BUFFER_USAGE_FLAGS)\n"
			":type usage: Optional[int]\n" )

		MAP_METHOD_AND_WRAP( "GetPipeline", GetPipeline, "Returns pipeline associated with this texture if any" )

		MAP_METHOD_AND_WRAP( "GetOriginalMemoryUsage", GetOriginalMemoryUsage, "Returns memory size for the non-LODed texture (used for stats and tools)" )
	EXPOSURE_CHAINTO( BlueAsyncRes )
}

namespace
{
ALResult OpenSharedTexture( uint32_t handle, TriTextureRes*& result )
{
	result = nullptr;

	TriTextureResPtr ptr;
	ptr.CreateInstance();
	CR_RETURN_HR( ptr->OpenShared( handle ) );
	result = ptr.Detach();
	return ALResult( S_OK );
}
}

MAP_FUNCTION_AND_WRAP(
	"OpenSharedTexture",
	OpenSharedTexture,
	"Creates a TriTextureRes object that has access to a previously created shared texture.\n"
	":param handle: shared handle of the texture\n" );
