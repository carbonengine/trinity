#include "StdAfx.h"

#include "Tr2RenderContext.h"
#include "Tr2RenderTarget.h"
#include "TriSettingsRegistrar.h"

#pragma warning(push, 4)

using namespace Tr2RenderContextEnum;

const Be::VarChooser Tr2RenderContextEnum_ObjectType_Chooser[] =
{
#define OP_ITEM(x)	{ #x, BeCast( OT_ ## x ), #x " AL object type" },
	OP_ITEM( CONSTANT_BUFFER )
	OP_ITEM( RENDER_CONTEXT )
	OP_ITEM( SHADER )
	OP_ITEM( SAMPLER_STATE )
	OP_ITEM( TEXTURE )
	OP_ITEM( VERTEX_LAYOUT )
	OP_ITEM( OCCLUSION_QUERY )
	OP_ITEM( SWAP_CHAIN )
	OP_ITEM( FENCE )
	OP_ITEM( TIMER )
	OP_ITEM( RESOURCE_SET )
	OP_ITEM( BUFFER )
#undef OP_ITEM
	{0}
};

BLUE_REGISTER_ENUM_EX( 
    "AL_OBJECT_TYPE", 
	ObjectType, 
    Tr2RenderContextEnum_ObjectType_Chooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);

const Be::VarChooser Tr2RenderContextEnum_ConstantType_Chooser[] =
{
	// Name		   Value		    Docstring
	{ "VERTEX_SHADER", BeCast( VERTEX_SHADER ), "Bind the constants to the vertex shader" }, 
	{ "PIXEL_SHADER" , BeCast( PIXEL_SHADER  ), "Bind the constants to the pixel shader" }, 	
	{0}
};

BLUE_REGISTER_ENUM_EX( 
    "CONSTANT_TYPE", 
	ShaderType, 
    Tr2RenderContextEnum_ConstantType_Chooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);

const Be::VarChooser Tr2RenderContextEnum_ClearFlags_Chooser[] =
{
	// Name		   Value		    Docstring
	{ "TARGET"  , BeCast( CLEARFLAGS_TARGET  ), "Clear rendertarget" }, 
	{ "ZBUFFER" , BeCast( CLEARFLAGS_ZBUFFER ), "Clear depth" }, 	
	{ "STENCIL" , BeCast( CLEARFLAGS_STENCIL ), "Clear stencil" }, 	
	{0}
};

BLUE_REGISTER_ENUM_EX( 
    "CLEAR_FLAGS", 
	ClearFlags, 
    Tr2RenderContextEnum_ClearFlags_Chooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);

const Be::VarChooser Tr2RenderContextEnum_DepthStencilFormat_Chooser[] =
{
#define DS_ITEM(x)	{ #x, BeCast( DSFMT_ ## x ), #x " depthStencil format" },

		DS_ITEM(D24S8)
		DS_ITEM(D24X8)
		DS_ITEM(D24FS8)
		DS_ITEM(D32F)
		DS_ITEM(D32)
		DS_ITEM(READABLE)

		DS_ITEM(AUTO)

		DS_ITEM(D16_LOCKABLE)
		DS_ITEM(D15S1)
	    DS_ITEM(D24X4S4)
		DS_ITEM(D16)
		DS_ITEM(D32F_LOCKABLE)
		DS_ITEM(D24FS8)

	{0}
#undef DS_ITEM
};

BLUE_REGISTER_ENUM_EX( 
    "DEPTH_STENCIL_FORMAT", 
	DepthStencilFormat, 
    Tr2RenderContextEnum_DepthStencilFormat_Chooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);

const Be::VarChooser Tr2RenderContextEnum_TextureType_Chooser[] = 
{
	{	"TEX_TYPE_1D",		BeCast( TEX_TYPE_1D ),		"1D texture" },
	{	"TEX_TYPE_2D",		BeCast( TEX_TYPE_2D ),		"2D texture" },
	{	"TEX_TYPE_3D",		BeCast( TEX_TYPE_3D ),		"3D texture" },
	{	"TEX_TYPE_CUBE",	BeCast( TEX_TYPE_CUBE ),		"Cube texture" },
	{	"TEX_TYPE_INVALID",	BeCast( TEX_TYPE_INVALID ),	"Invalid texture" },
	{0}
};

BLUE_REGISTER_ENUM_EX( 
    "TEXTURE_TYPE", 
	TextureType, 
    Tr2RenderContextEnum_TextureType_Chooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);

const Be::VarChooser Tr2RenderContextEnum_BufferUsageFlags_Chooser[] = 
{
	{	"CPU_READ",			BeCast( USAGE_CPU_READ ),			"USAGE_CPU_READ" },
	{	"CPU_WRITE",		BeCast( USAGE_CPU_WRITE ),			"USAGE_CPU_WRITE" },
	{	"LOCK_FREQUENTLY",	BeCast( USAGE_LOCK_FREQUENTLY ),	"USAGE_LOCK_FREQUENTLY" },
	{	"IMMUTABLE",		BeCast( USAGE_IMMUTABLE ),			"USAGE_IMMUTABLE" },
	{	"HINT_MANAGED",		BeCast( TEX_TYPE_INVALID ),			"USAGE_HINT_MANAGED" },
	{0}
};

BLUE_REGISTER_ENUM_EX( 
    "BUFFER_USAGE_FLAGS", 
	BufferUsageFlags, 
    Tr2RenderContextEnum_BufferUsageFlags_Chooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);


const Be::VarChooser Tr2RenderContextEnum_ExFlag_Chooser[] = 
{
	{	"CREATE_SHARED",			BeCast( EX_CREATE_SHARED ),				"" },	
	{	"BIND_UNORDERED_ACCESS",	BeCast( EX_BIND_UNORDERED_ACCESS ),		"" },	
	{	"DRAW_INDIRECT",			BeCast( EX_DRAW_INDIRECT ),				"" },	
	{0}
};

BLUE_REGISTER_ENUM_EX( 
    "EX_FLAG", 
	ExFlag, 
    Tr2RenderContextEnum_ExFlag_Chooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);


#define VAL(x)	{ #x, BeCast( PIXEL_FORMAT_ ## x ), #x },

const Be::VarChooser Tr2RenderContextEnum_PixelFormat_Chooser[] =
{
	VAL(UNKNOWN)
	VAL(R32G32B32A32_TYPELESS)
	VAL(R32G32B32A32_FLOAT)
	VAL(R32G32B32A32_UINT)
	VAL(R32G32B32A32_SINT)
	VAL(R32G32B32_TYPELESS)
	VAL(R32G32B32_FLOAT)
	VAL(R32G32B32_UINT)
	VAL(R32G32B32_SINT)
	VAL(R16G16B16A16_TYPELESS)
	VAL(R16G16B16A16_FLOAT)
	VAL(R16G16B16A16_UNORM)
	VAL(R16G16B16A16_UINT)
	VAL(R16G16B16A16_SNORM)
	VAL(R16G16B16A16_SINT)
	VAL(R32G32_TYPELESS)
	VAL(R32G32_FLOAT)
	VAL(R32G32_UINT)
	VAL(R32G32_SINT)
	VAL(R32G8X24_TYPELESS)
	VAL(D32_FLOAT_S8X24_UINT)
	VAL(R32_FLOAT_X8X24_TYPELESS)
	VAL(X32_TYPELESS_G8X24_UINT)
	VAL(R10G10B10A2_TYPELESS)
	VAL(R10G10B10A2_UNORM)
	VAL(R10G10B10A2_UINT)
	VAL(R11G11B10_FLOAT)
	VAL(R8G8B8A8_TYPELESS)
	VAL(R8G8B8A8_UNORM)
	VAL(R8G8B8A8_UNORM_SRGB)
	VAL(R8G8B8A8_UINT)
	VAL(R8G8B8A8_SNORM)
	VAL(R8G8B8A8_SINT)
	VAL(R16G16_TYPELESS)
	VAL(R16G16_FLOAT)
	VAL(R16G16_UNORM)
	VAL(R16G16_UINT)
	VAL(R16G16_SNORM)
	VAL(R16G16_SINT)
	VAL(R32_TYPELESS)
	VAL(D32_FLOAT)
	VAL(R32_FLOAT)
	VAL(R32_UINT)
	VAL(R32_SINT)
	VAL(R24G8_TYPELESS)
	VAL(D24_UNORM_S8_UINT)
	VAL(R24_UNORM_X8_TYPELESS)
	VAL(X24_TYPELESS_G8_UINT)
	VAL(R8G8_TYPELESS)
	VAL(R8G8_UNORM)
	VAL(R8G8_UINT)
	VAL(R8G8_SNORM)
	VAL(R8G8_SINT)
	VAL(R16_TYPELESS)
	VAL(R16_FLOAT)
	VAL(D16_UNORM)
	VAL(R16_UNORM)
	VAL(R16_UINT)
	VAL(R16_SNORM)
	VAL(R16_SINT)
	VAL(R8_TYPELESS)
	VAL(R8_UNORM)
	VAL(R8_UINT)
	VAL(R8_SNORM)
	VAL(R8_SINT)
	VAL(A8_UNORM)
	VAL(R1_UNORM)
	VAL(R9G9B9E5_SHAREDEXP)
	VAL(R8G8_B8G8_UNORM)
	VAL(G8R8_G8B8_UNORM)
	VAL(BC1_TYPELESS)
	VAL(BC1_UNORM)
	VAL(BC1_UNORM_SRGB)
	VAL(BC2_TYPELESS)
	VAL(BC2_UNORM)
	VAL(BC2_UNORM_SRGB)
	VAL(BC3_TYPELESS)
	VAL(BC3_UNORM)
	VAL(BC3_UNORM_SRGB)
	VAL(BC4_TYPELESS)
	VAL(BC4_UNORM)
	VAL(BC4_SNORM)
	VAL(BC5_TYPELESS)
	VAL(BC5_UNORM)
	VAL(BC5_SNORM)
	VAL(B5G6R5_UNORM)
	VAL(B5G5R5A1_UNORM)
	VAL(B8G8R8A8_UNORM)
	VAL(B8G8R8X8_UNORM)
	VAL(R10G10B10_XR_BIAS_A2_UNORM)
	VAL(B8G8R8A8_TYPELESS)
	VAL(B8G8R8A8_UNORM_SRGB)
	VAL(B8G8R8X8_TYPELESS)
	VAL(B8G8R8X8_UNORM_SRGB)
	VAL(BC6H_TYPELESS)
	VAL(BC6H_UF16)
	VAL(BC6H_SF16)
	VAL(BC7_TYPELESS)
	VAL(BC7_UNORM)
	VAL(BC7_UNORM_SRGB)
	{0}
};

BLUE_REGISTER_ENUM_EX( 
    "PIXEL_FORMAT", 
	PixelFormat, 
    Tr2RenderContextEnum_PixelFormat_Chooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);

const Be::VarChooser Tr2RenderContextEnum_ScanlineOrdering_Chooser[] = 
{
	{	"UNSPECIFIED",			BeCast( SCANLINE_ORDER_UNSPECIFIED ),		"" },
	{	"PROGRESSIVE",			BeCast( SCANLINE_ORDER_PROGRESSIVE ),		"" },
	{	"UPPER_FIELD_FIRST",	BeCast( SCANLINE_ORDER_UPPER_FIELD_FIRST ),	"" },
	{	"LOWER_FIELD_FIRST",	BeCast( SCANLINE_ORDER_LOWER_FIELD_FIRST ),	"" },
	{0}
};

const Be::VarChooser Tr2RenderContextEnum_DisplayScaling_Chooser[] = 
{
	{	"UNSPECIFIED",	BeCast( DISPLAY_SCALING_UNSPECIFIED ),	"" },
	{	"CENTERED",		BeCast( DISPLAY_SCALING_CENTERED ),		"" },
	{	"STRETCHED",	BeCast( DISPLAY_SCALING_STRETCHED ),	"" },
	{0}
};

const Be::VarChooser Tr2RenderContextEnum_SwapEffect_Chooser[] = 
{
	{	"DISCARD",		BeCast( SWAP_EFFECT_DISCARD ),			"" },
	{	"SEQUENTIAL",	BeCast( SWAP_EFFECT_SEQUENTIAL ),		"" },
	{0}
};

BLUE_REGISTER_ENUM_EX( 
    "SWAP_EFFECT", 
	SwapEffect, 
    Tr2RenderContextEnum_SwapEffect_Chooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);

const Be::VarChooser Tr2RenderContextEnum_PresentInterval_Chooser[] = 
{
	{	"IMMEDIATE",	BeCast( PRESENT_INTERVAL_IMMEDIATE ),	"" },
	{	"ONE",			BeCast( PRESENT_INTERVAL_ONE ),			"" },
	{0}
};

BLUE_REGISTER_ENUM_EX( 
    "PRESENT_INTERVAL", 
	PresentInterval, 
    Tr2RenderContextEnum_PresentInterval_Chooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);


BLUE_DEFINE( Tr2RenderContext );

#if TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
BLUE_DEFINE( Tr2PrimaryRenderContext );
#endif

extern uint32_t g_forceAnisotropy;
TRI_REGISTER_SETTING( "forceAnisotropy", g_forceAnisotropy );

extern bool g_preloadTextureToDeviceOnPrepare;
TRI_REGISTER_SETTING( "preloadTextureToDeviceOnPrepare", g_preloadTextureToDeviceOnPrepare );


extern bool g_gatherPipelineStatistics;
TRI_REGISTER_SETTING( "gatherPipelineStatistics", g_gatherPipelineStatistics );


const Be::ClassInfo* Tr2RenderContext::ExposeToBlue()
{
	/////////////////////////////////////////
	// Blue class info
    EXPOSURE_BEGIN( Tr2RenderContext, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( Tr2RenderContext )		

#if TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
	EXPOSURE_END()
}

const Be::ClassInfo* Tr2PrimaryRenderContext::ExposeToBlue()
{
	/////////////////////////////////////////
	// Blue class info
    EXPOSURE_BEGIN( Tr2PrimaryRenderContext, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( Tr2PrimaryRenderContext )
		//MAP_INTERFACE( Tr2RenderContext )
#endif

		MAP_METHOD_AND_WRAP
		( 
			"GetDefaultBackBuffer",
			GetBackBuffer,
			"Returns a Tr2RenderTarget representing the default back buffer"
		)

		MAP_METHOD_AND_WRAP
		( 
			"GetBackBufferFormat",
			GetBackBufferFormat,
			"Returns the PixelFormat of the default back buffer"
		)

	EXPOSURE_END()
}


#if BLUE_WITH_PYTHON

namespace
{
	PyObject* result;

	void GetDesc( Tr2ALMemoryType, const Tr2DeviceResourceDescriptionAL& description )
	{
		std::string key = "";
		auto found = description.find( "type" );
		if( found != end( description ) )
		{
			key = found->second;
		}
		PyObject* list = PyDict_GetItemString( result, key.c_str() );
		if( !list )
		{
			list = PyList_New( 0 );
			PyDict_SetItemString( result, key.c_str(), list );
			Py_DECREF( list );
		}

		PyObject* item = PyDict_New();
		if( item )
		{
			for( auto it = description.begin(); it != description.end(); ++it )
			{
				PyObject* value = PyString_FromString( it->second.c_str() );
				PyDict_SetItemString( item, it->first.c_str(), value );
				Py_DECREF( value );
			}
			PyList_Append( list, item );
			Py_DECREF( item );
		}
	};

	// --------------------------------------------------------------------------------------
	// Description:
	//   Returns all live AL resources as Python structure.
	// Arguments:
	//   self - Python self reference (unused)
	//   args - Python call arguments (unused)
	// Return value:
	//   Python dictionary describing live AL objects
	// --------------------------------------------------------------------------------------
	PyObject* PyGetLiveResources( PyObject* /*self*/, PyObject* /*args*/ )
	{
		result = PyDict_New();
		DescribeDeviceResources( GetDesc );
		return result;
	}
}

MAP_FUNCTION( 
	"GetLiveALResources", 
	PyGetLiveResources, 
	"Returns a per-AL-type dictionary of lists of live AL objects. The function\n"
	"is expensive and is not supposed to be used every frame.\n"
	":rtype: dict[str, list[dict[str, str]]]"
	);

#endif

MAP_FUNCTION_AND_WRAP( 
	"GetPixelFormatSize", 
	GetBytesPerPixel, 
	"Returns size of pixel (or block for compressed formats) in bytes of a given\n"
	"pixel format.\n"
	":param pixelFormat: pixel format (member of trinity.PIXEL_FORMAT)" 
	);