#pragma once
#ifndef Tr2RenderContextEnum_h
#define Tr2RenderContextEnum_h

namespace Tr2RenderContextEnum
{
	using namespace ImageIO;	

	enum ObjectType
	{
		OT_CONSTANT_BUFFER,
		OT_RENDER_CONTEXT,
		OT_SHADER,
		OT_SAMPLER_STATE,
		OT_TEXTURE,
		OT_VERTEX_LAYOUT,
		OT_OCCLUSION_QUERY,
		OT_SWAP_CHAIN,
		OT_FENCE,
		OT_TIMER,

		OT_SHADER_PROGRAM,
		OT_RESOURCE_SET,
		OT_BUFFER,

		OBJECT_TYPE_COUNT
	};

	enum ShaderType
	{
		VERTEX_SHADER,
		PIXEL_SHADER,
		COMPUTE_SHADER,
		GEOMETRY_SHADER,
		HULL_SHADER,
		DOMAIN_SHADER,

		INVALID_SHADER,
		SHADER_TYPE_FIRST = VERTEX_SHADER,
		SHADER_TYPE_COUNT = INVALID_SHADER,
	};

	/** Representation of controllable GPU pipelines */
	enum ShaderPipe
	{
		GRAPHICS_PIPE = 0,
		COMPUTE_PIPE,
		INVALID_PIPE,

		SHADER_PIPE_FIRST = GRAPHICS_PIPE,
		SHADER_PIPE_COUNT = INVALID_PIPE
	};

	// Special case constant buffers added to handle trinity
	enum
	{
		CBUFFER_GUI = SHADER_TYPE_COUNT,
		CBUFFER_COUNT	// total number of cbuffers passed to SetPerObjectToDevice
	};

	// Clearing bound renderTarget and/or depth stencil
	enum ClearFlags
	{
		CLEARFLAGS_TARGET  = 1,
		CLEARFLAGS_ZBUFFER = 2,
		CLEARFLAGS_STENCIL = 4,
	};
    
    enum CullMode
    {
        CULLMODE_NONE   = 1,
        CULLMODE_CW     = 2,
        CULLMODE_CCW    = 3,
    };

	enum ColorWriteEnable
	{
		COLORWRITEENABLE_RED	= 1 << 0,
		COLORWRITEENABLE_GREEN	= 1 << 1,
		COLORWRITEENABLE_BLUE	= 1 << 2,
		COLORWRITEENABLE_ALPHA	= 1 << 3,
	};


	// Formats for depth/stencil bufers
	enum DepthStencilFormat
	{
		DSFMT_UNKNOWN,	// invalid, zero, make sure any valid enum is > 0

		DSFMT_D24S8,
		DSFMT_D24X8,
		DSFMT_D24FS8,
		DSFMT_D32F,
		DSFMT_D32,
		DSFMT_READABLE,

		DSFMT_AUTO,		// "don't care, use what's most likely to work"

		// These are just here for completeness, probably not a great idea to try and use them.
		DSFMT_D16_LOCKABLE,		
		DSFMT_D15S1,		
	    DSFMT_D24X4S4,
		DSFMT_D16,
		DSFMT_D32F_LOCKABLE
	};

	// Locking buffers
	enum LockType
	{
		LOCK_READONLY,
		LOCK_WRITEONLY,
		LOCK_NO_OVERWRITE,
		LOCK_INVALID
	};

	// Vertex/index buffer usage flags
	enum BufferUsageFlags
	{
		// Resource is inteded to be locked for reading
		USAGE_CPU_READ			= 1 << 0,
		// Resource is inteded to be locked for writing
		USAGE_CPU_WRITE			= 1 << 1,
		// Resource is inteded to be locked more than once per frame
		USAGE_LOCK_FREQUENTLY	= 1 << 2,
		// Resource is immutable (no CPU or GPU modifications)
		USAGE_IMMUTABLE			= 1 << 3,
		// DX9-specific: put resource into managed pool
		USAGE_HINT_MANAGED		= 1 << 4,
		// Access resource as a unordered access view in shaders
		USAGE_UNORDERED_ACCESS	= 1 << 5,
		// Access resource to be accessed shaders (as shader resources); applies to buffers
		USAGE_SHADER_RESOURCE   = 1 << 6,
	};

	// Combination of BufferUsageFlags
	typedef int BufferUsage;

	bool ValidateUsage( BufferUsage );

	// Topology
	enum Topology
	{
		TOP_INVALID,

		TOP_TRIANGLES,
		TOP_TRIANGLE_STRIP,
		TOP_TRIANGLE_FAN,		// invalid on DX11!
		TOP_LINES,
		TOP_LINE_STRIP,
		TOP_POINTS,

		TOP_MAX_TOPOLOGY
	};

	// Comparison functions
	enum CompareFunc
	{
		CMP_NEVER                = 1,
		CMP_LESS                 = 2,
		CMP_EQUAL                = 3,
		CMP_LESSEQUAL            = 4,
		CMP_GREATER              = 5,
		CMP_NOTEQUAL             = 6,
		CMP_GREATEREQUAL         = 7,
		CMP_ALWAYS               = 8,

		CMP_FORCE_DWORD			 = 0xffffffff
	};
    
    enum FillMode
    {
        FM_POINT                 = 1,
        FM_WIREFRAME             = 2,
        FM_SOLID                 = 3,
        FM_FORCE_DWORD           = 0xffffffff
    };
    
    enum BlendMode
    {
        BM_ZERO                 = 1,
        BM_ONE                  = 2,
        BM_SRCCOLOR             = 3,
        BM_INVSRCCOLOR          = 4,
        BM_SRCALPHA             = 5,
        BM_INVSRCALPHA          = 6,
        BM_DESTALPHA            = 7,
        BM_INVDESTALPHA         = 8,
        BM_DESTCOLOR            = 9,
        BM_INVDESTCOLOR         = 10,
        BM_SRCALPHASAT          = 11,
        BM_BOTHSRCALPHA         = 12,
        BM_BOTHINVSRCALPHA      = 13,
        BM_BLENDFACTOR          = 14,
        BM_INVBLENDFACTOR       = 15,
        BM_FORCE_DWORD          = 0xffffffff
    };
    
    enum BlendOperation
    {
        BO_DISABLE              = 0,
        BO_ADD                  = 1,
        BO_SUBTRACT             = 2,
        BO_REVSUBTRACT          = 3,
        BO_MIN                  = 4,
        BO_MAX                  = 5,
        BO_FORCE_DWORD          = 0xffffffff
    };

	// Magical flags for CreateEx and friends
	enum ExFlag
	{
		EX_NONE						= 0,
		EX_CREATE_SHARED			= 1 << 0,		
		EX_BIND_UNORDERED_ACCESS	= 1 << 1,
		EX_DRAW_INDIRECT			= 1 << 3,
	};

	inline ExFlag operator|( ExFlag a, ExFlag b )
	{
		return ExFlag( int( a ) | int( b ) );
	}


	// Texture color space
	enum ColorSpace
	{
		COLOR_SPACE_LINEAR,
		COLOR_SPACE_SRGB,
		_COLOR_SPACE_COUNT,
	};
    
    enum StencilOperation {
        STENCILOP_KEEP         = 1,
        STENCILOP_ZERO         = 2,
        STENCILOP_REPLACE      = 3,
        STENCILOP_INCRSAT      = 4,
        STENCILOP_DECRSAT      = 5,
        STENCILOP_INVERT       = 6,
        STENCILOP_INCR         = 7,
        STENCILOP_DECR         = 8,
    };

	// Rasterization states for backward compatibility
	enum RenderState
	{
		RS_ZENABLE                   = 7,    /* D3DZBUFFERTYPE (or TRUE/FALSE for legacy) */
		RS_FILLMODE                  = 8,    /* D3DFILLMODE */
		RS_SHADEMODE                 = 9,    /* D3DSHADEMODE */
		RS_ZWRITEENABLE              = 14,   /* TRUE to enable z writes */
		RS_ALPHATESTENABLE           = 15,   /* TRUE to enable alpha tests */
		RS_LASTPIXEL                 = 16,   /* TRUE for last-pixel on lines */
		RS_SRCBLEND                  = 19,   /* D3DBLEND */
		RS_DESTBLEND                 = 20,   /* D3DBLEND */
		RS_CULLMODE                  = 22,   /* D3DCULL */
		RS_ZFUNC                     = 23,   /* D3DCMPFUNC */
		RS_ALPHAREF                  = 24,   /* D3DFIXED */
		RS_ALPHAFUNC                 = 25,   /* D3DCMPFUNC */
		RS_DITHERENABLE              = 26,   /* TRUE to enable dithering */
		RS_ALPHABLENDENABLE          = 27,   /* TRUE to enable alpha blending */
		RS_FOGENABLE                 = 28,   /* TRUE to enable fog blending */
		RS_SPECULARENABLE            = 29,   /* TRUE to enable specular */
		RS_FOGCOLOR                  = 34,   /* D3DCOLOR */
		RS_FOGTABLEMODE              = 35,   /* D3DFOGMODE */
		RS_FOGSTART                  = 36,   /* Fog start (for both vertex and pixel fog) */
		RS_FOGEND                    = 37,   /* Fog end      */
		RS_FOGDENSITY                = 38,   /* Fog density  */
		RS_RANGEFOGENABLE            = 48,   /* Enables range-based fog */
		RS_STENCILENABLE             = 52,   /* BOOL enable/disable stenciling */
		RS_STENCILFAIL               = 53,   /* D3DSTENCILOP to do if stencil test fails */
		RS_STENCILZFAIL              = 54,   /* D3DSTENCILOP to do if stencil test passes and Z test fails */
		RS_STENCILPASS               = 55,   /* D3DSTENCILOP to do if both stencil and Z tests pass */
		RS_STENCILFUNC               = 56,   /* D3DCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
		RS_STENCILREF                = 57,   /* Reference value used in stencil test */
		RS_STENCILMASK               = 58,   /* Mask value used in stencil test */
		RS_STENCILWRITEMASK          = 59,   /* Write mask applied to values written to stencil buffer */
		RS_TEXTUREFACTOR             = 60,   /* D3DCOLOR used for multi-texture blend */
		RS_WRAP0                     = 128,  /* wrap for 1st texture coord. set */
		RS_WRAP1                     = 129,  /* wrap for 2nd texture coord. set */
		RS_WRAP2                     = 130,  /* wrap for 3rd texture coord. set */
		RS_WRAP3                     = 131,  /* wrap for 4th texture coord. set */
		RS_WRAP4                     = 132,  /* wrap for 5th texture coord. set */
		RS_WRAP5                     = 133,  /* wrap for 6th texture coord. set */
		RS_WRAP6                     = 134,  /* wrap for 7th texture coord. set */
		RS_WRAP7                     = 135,  /* wrap for 8th texture coord. set */
		RS_CLIPPING                  = 136,
		RS_LIGHTING                  = 137,
		RS_AMBIENT                   = 139,
		RS_FOGVERTEXMODE             = 140,
		RS_COLORVERTEX               = 141,
		RS_LOCALVIEWER               = 142,
		RS_NORMALIZENORMALS          = 143,
		RS_DIFFUSEMATERIALSOURCE     = 145,
		RS_SPECULARMATERIALSOURCE    = 146,
		RS_AMBIENTMATERIALSOURCE     = 147,
		RS_EMISSIVEMATERIALSOURCE    = 148,
		RS_VERTEXBLEND               = 151,
		RS_CLIPPLANEENABLE           = 152,
		RS_POINTSIZE                 = 154,   /* float point size */
		RS_POINTSIZE_MIN             = 155,   /* float point size min threshold */
		RS_POINTSPRITEENABLE         = 156,   /* BOOL point texture coord control */
		RS_POINTSCALEENABLE          = 157,   /* BOOL point size scale enable */
		RS_POINTSCALE_A              = 158,   /* float point attenuation A value */
		RS_POINTSCALE_B              = 159,   /* float point attenuation B value */
		RS_POINTSCALE_C              = 160,   /* float point attenuation C value */
		RS_MULTISAMPLEANTIALIAS      = 161,  // BOOL - set to do FSAA with multisample buffer
		RS_MULTISAMPLEMASK           = 162,  // DWORD - per-sample enable/disable
		RS_PATCHEDGESTYLE            = 163,  // Sets whether patch edges will use float style tessellation
		RS_DEBUGMONITORTOKEN         = 165,  // DEBUG ONLY - token to debug monitor
		RS_POINTSIZE_MAX             = 166,   /* float point size max threshold */
		RS_INDEXEDVERTEXBLENDENABLE  = 167,
		RS_COLORWRITEENABLE          = 168,  // per-channel write enable
		RS_TWEENFACTOR               = 170,   // float tween factor
		RS_BLENDOP                   = 171,   // D3DBLENDOP setting
		RS_POSITIONDEGREE            = 172,   // NPatch position interpolation degree. D3DDEGREE_LINEAR or D3DDEGREE_CUBIC (default)
		RS_NORMALDEGREE              = 173,   // NPatch normal interpolation degree. D3DDEGREE_LINEAR (default) or D3DDEGREE_QUADRATIC
		RS_SLOPESCALEDEPTHBIAS       = 175,
		RS_ANTIALIASEDLINEENABLE     = 176,
		RS_MINTESSELLATIONLEVEL      = 178,
		RS_MAXTESSELLATIONLEVEL      = 179,
		RS_ADAPTIVETESS_X            = 180,
		RS_ADAPTIVETESS_Y            = 181,
		RS_ADAPTIVETESS_Z            = 182,
		RS_ADAPTIVETESS_W            = 183,
		RS_ENABLEADAPTIVETESSELLATION = 184,
		RS_TWOSIDEDSTENCILMODE       = 185,   /* BOOL enable/disable 2 sided stenciling */
		RS_CCW_STENCILFAIL           = 186,   /* D3DSTENCILOP to do if ccw stencil test fails */
		RS_CCW_STENCILZFAIL          = 187,   /* D3DSTENCILOP to do if ccw stencil test passes and Z test fails */
		RS_CCW_STENCILPASS           = 188,   /* D3DSTENCILOP to do if both ccw stencil and Z tests pass */
		RS_CCW_STENCILFUNC           = 189,   /* D3DCMPFUNC fn.  ccw Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
		RS_COLORWRITEENABLE1         = 190,   /* Additional ColorWriteEnables for the devices that support D3DPMISCCAPS_INDEPENDENTWRITEMASKS */
		RS_COLORWRITEENABLE2         = 191,   /* Additional ColorWriteEnables for the devices that support D3DPMISCCAPS_INDEPENDENTWRITEMASKS */
		RS_COLORWRITEENABLE3         = 192,   /* Additional ColorWriteEnables for the devices that support D3DPMISCCAPS_INDEPENDENTWRITEMASKS */
		RS_BLENDFACTOR               = 193,   /* D3DCOLOR used for a constant blend factor during alpha blending for devices that support D3DPBLENDCAPS_BLENDFACTOR */
		RS_SRGBWRITEENABLE           = 194,   /* Enable rendertarget writes to be DE-linearized to SRGB (for formats that expose D3DUSAGE_QUERY_SRGBWRITE) */
		RS_DEPTHBIAS                 = 195,
		RS_WRAP8                     = 198,   /* Additional wrap states for vs_3_0+ attributes with D3DDECLUSAGE_TEXCOORD */
		RS_WRAP9                     = 199,
		RS_WRAP10                    = 200,
		RS_WRAP11                    = 201,
		RS_WRAP12                    = 202,
		RS_WRAP13                    = 203,
		RS_WRAP14                    = 204,
		RS_WRAP15                    = 205,
		RS_SEPARATEALPHABLENDENABLE  = 206,  /* TRUE to enable a separate blending function for the alpha channel */
		RS_SRCBLENDALPHA             = 207,  /* SRC blend factor for the alpha channel when RS_SEPARATEDESTALPHAENABLE is TRUE */
		RS_DESTBLENDALPHA            = 208,  /* DST blend factor for the alpha channel when RS_SEPARATEDESTALPHAENABLE is TRUE */
		RS_BLENDOPALPHA              = 209,  /* Blending operation for the alpha channel when RS_SEPARATEDESTALPHAENABLE is TRUE */

		// from DX8?
		RS_ZBIAS					 = 47,


		RS_MAX_STATE			     = 210,

		RS_FORCE_DWORD               = 0x7fffffff, /* force 32-bit size enum */
	};

	enum TextureFilter
	{
		TF_NONE				= 0,
		TF_POINT			= 1,
		TF_LINEAR			= 2,
		TF_ANISOTROPIC		= 3,
		TF_COMPARISON		= 0x80,
	};

	enum TextureAddressMode
	{
		TA_WRAP				= 1,
		TA_MIRROR			= 2,
		TA_CLAMP			= 3,
		TA_BORDER			= 4,
		TA_MIRROR_ONCE		= 5,
	};


	enum ScanlineOrdering
	{
		SCANLINE_ORDER_UNSPECIFIED        = 0,
		SCANLINE_ORDER_PROGRESSIVE        = 1,
		SCANLINE_ORDER_UPPER_FIELD_FIRST  = 2,
		SCANLINE_ORDER_LOWER_FIELD_FIRST  = 3
	};

	enum DisplayScaling
	{
		DISPLAY_SCALING_UNSPECIFIED   = 0,
		DISPLAY_SCALING_CENTERED      = 1,
		DISPLAY_SCALING_STRETCHED     = 2
	};

	enum SwapEffect
	{
		SWAP_EFFECT_DISCARD			= 0,
		SWAP_EFFECT_SEQUENTIAL		= 1,
	};

	enum PresentInterval
	{
		PRESENT_INTERVAL_IMMEDIATE		= 0,
		PRESENT_INTERVAL_ONE			= 1,
	};


	PixelFormat ConvertD3DBackBufferFormat( /*D3DFORMAT*/ unsigned fmt );

	PixelFormat	MakeTypeless( PixelFormat fmt );
	PixelFormat MakeSrgb( PixelFormat format );

	PixelFormat ConvertDepthStencilFormat( DepthStencilFormat format );


	static const int CONSTANT_BUFFER_FOR_EFFECT_PARAMETERS = 0;
	static const int CONSTANT_BUFFER_FOR_FRAGMENT_PARAMETERS = 7;
	static const int CONSTANT_BUFFER_FOR_FRAGMENT_OP_EMULATION = 10;

	static const int VERTEX_BUFFER_ZERO_STREAM_RESERVED = 4;

// Check if shader type (Tr2RenderContextEnum::ShaderType) exists on the current platform
#define SHADER_TYPE_EXISTS( shaderType ) ( Tr2RenderContextAL::SHADER_TYPE_MASK & ( 1 << ( shaderType ) ) )

	enum RenderContextStatus
	{
		CONTEXT_STATUS_NOT_STARTED = 0,
		CONTEXT_STATUS_EXECUTING,
		CONTEXT_STATUS_FINISHED,

		CONTEXT_STATUS_INVALID,
	};

	enum FrameEvent
	{
		FRAME_EVENT_UPDATE_STARTED,
		FRAME_EVENT_UPDATE_FINISHED,
		FRAME_EVENT_PRESENT_STARTED,
		FRAME_EVENT_PRESENT_FINISHED,
		FRAME_EVENT_RENDERING_STARTED,
		FRAME_EVENT_RENDERING_FINISHED
	};
}

using Tr2BitmapDimensions = ImageIO::BitmapDimensions;


namespace Tr2CpuUsage
{
	enum Type
	{
		NONE = 0,
		READ = 1 << 0,
		WRITE = 1 << 1,
		READ_OFTEN = READ | ( 1 << 2 ),
		WRITE_OFTEN = WRITE | ( 1 << 3 ),
		NON_SYNCRONIZED_WRITE = 1 << 4,
	};

	inline Type operator|( Type a, Type b )
	{
		return Type( int( a ) | int( b ) );
	}

	inline bool HasFlag( Type value, Type flag )
	{
		return ( value & flag ) == flag;
	}
}

namespace Tr2GpuUsage
{
	enum Type
	{
		NONE = 0,
		VERTEX_BUFFER = 1 << 0,
		INDEX_BUFFER = 1 << 1,

		RENDER_TARGET = 1 << 2,
		DEPTH_STENCIL = 1 << 3,

		SHADER_RESOURCE = 1 << 4,
		UNORDERED_ACCESS = 1 << 5,
		COPY_DESTINATION = 1 << 6,

		DRAW_INDIRECT_ARGS = 1 << 7,

		ACCELERATION_STRUCTURE = 1 << 8,

		SHARED = 1 << 9,
	};

	inline Type operator|( Type a, Type b )
	{
		return Type( int( a ) | int( b ) );
	}

	inline Type& operator|=( Type& a, Type b )
	{
		a = Type( a | b );
		return a;
	}

	inline bool HasFlag( Type value, Type flag )
	{
		return ( value & flag ) == flag;
	}

	inline bool IsWritable( Type value )
	{
		return ( value & ( RENDER_TARGET | DEPTH_STENCIL | UNORDERED_ACCESS | COPY_DESTINATION ) ) != 0;
	}

	inline bool HasBufferFlags( Type value )
	{
		return ( value & ( VERTEX_BUFFER | INDEX_BUFFER | DRAW_INDIRECT_ARGS ) ) != 0;
	}

	inline bool HasTextureFlags( Type value )
	{
		return ( value & ( RENDER_TARGET | DEPTH_STENCIL ) ) != 0;
	}

}

namespace Tr2LockType
{
	enum Type
	{
		SYNCHRONIZED,
		NON_SYNCHRONIZED,
	};
}

#endif // Tr2RenderContextEnum_h
