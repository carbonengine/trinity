// Copyright © 2023 CCP ehf.

#include "StdAfx.h"


#include "TriConstants.h"

#include <ITriConstants.h>

using namespace Tr2RenderContextEnum;

#if BLUE_WITH_PYTHON
void AddTriConstants( PyObject* d )
{
	const Be::VarChooser* const triConstants[] = {
#if DEPRECATED_ENABLED
		TriTextureAddress,
#endif

		TriBlendOp,
		TriBlend,
		TriTextureChooser,
		TriExtrapolation,
		TriOperator,
		TriTransformBase,
		TriGR2Chooser,
		TriD3DRenderState,
	};

	for( int i = 0; i < sizeof triConstants / sizeof triConstants[0]; i++ )
	{
		for( const Be::VarChooser* j = triConstants[i]; j->mKey; j++ )
		{
			// this obviously assumes that all values are LONG
			// later perhaps call BlueToPython to convert types
			// correctly
			PyObject* value = ToPython( j->mValue.mLong );
			PyDict_SetItemString( d, const_cast<char*>( j->mKey ), value );
			Py_DECREF( value );
		}
	}
}
#endif


const char* KeyFromVal( const Be::VarChooser* i, long val )
{
	while( i->mKey )
	{
		if( i->mValue.mLong == val )
			return i->mKey;
		i++;
	}

	return "[-not found-]";
}



#define VAL( v ) BeCast( v )
#define KV( name, doc ) \
	{                   \
		#name,          \
		VAL( name ),    \
		doc             \
	}


#define KV_RC( name, doc )                 \
	{                                      \
		"D3D" #name,                       \
		VAL( Tr2RenderContextEnum::name ), \
		doc                                \
	}

#define VAL_RC( v ) BeCast( Tr2RenderContextEnum::v )


const Be::VarChooser TriTextureChooser[] = {
	{ "SELECT_TEXTURE",
	  VAL( 0 ),
	  "Texture (.dds, .tga, .bmp)|*.dds;*.tga;*.bmp;|All Files (*.*)|*.*" },
	{ 0 }
};


const Be::VarChooser TriGR2Chooser[] = {
	{ "SELECT_GR2MODEL",
	  VAL( 0 ),
	  "Granny/CMF file(.gr2, .cmf)|*.GR2;*.CMF|All Files (*.*)|*.*" },
	{ 0 }
};

const Be::VarChooser TriExtrapolation[] = {
	{ "TRIEXT_NONE",
	  VAL( TRIEXT_NONE ),
	  "no comment" },
	{ "TRIEXT_CONSTANT",
	  VAL( TRIEXT_CONSTANT ),
	  "no comment" },
	{ "TRIEXT_GRADIENT",
	  VAL( TRIEXT_GRADIENT ),
	  "no comment" },
	{ "TRIEXT_CYCLE",
	  VAL( TRIEXT_CYCLE ),
	  "no comment" },
	{ 0 }
};


const Be::VarChooser TriTransformBase[] = {
	{ "TRITB_OBJECT",
	  VAL( TRITB_OBJECT ),
	  "This makes TriTransforms build hierachies as expected, that is a child inherits "
	  "its parent transforms.\r\n."
	  "For TriTextures this is mosly usefull for projecting textures from objects" },
	{ "TRITB_CAMERA_ROTATION",
	  VAL( TRITB_CAMERA_ROTATION ),
	  "This sets the inverted rotation part of the camera matrix as the base for the"
	  "transformations be they texture or transforms, good for environment maps and"
	  "normal projected cubemaps." },
	{ "TRITB_CAMERA_TRANSLATION",
	  VAL( TRITB_CAMERA_TRANSLATION ),
	  "This sets the inverted translation part of the camera matrix as the base for the"
	  "transformations be they texture or transforms, good for geometry that is locked,"
	  "to the camera, like nebulas and starfields, etc." },
	{ "TRITB_CAMERA",
	  VAL( TRITB_CAMERA ),
	  "This sets the inverted camera matrix as the base for the transformations"
	  "be they texture or geometry transforms, basically the same, as "
	  "TRITB_CAMERA_ROTATION except the translation is taken also"
	  "" },
	{ "TRITB_CAMERA_ROTATION_ALIGNED",
	  VAL( TRITB_CAMERA_ROTATION_ALIGNED ),
	  "Makes the transform face the camera at all times, used for billboarding."
	  "The up direction of the billboard is preserved." },
	{ "TRITB_CAMERA_ROTATION_ALIGNED_SYMMETRY",
	  VAL( TRITB_CAMERA_ROTATION_ALIGNED_SYMMETRY ),
	  "Makes the transform face the camera at all times, used for billboarding,"
	  "the rotation around the axis to the camera is not corrected, hence this"
	  "should only be used for rotationally symmetric billboards." },
	{ "TRITB_CAMERA_ROTATION_FALLOFF",
	  VAL( TRITB_CAMERA_ROTATION_FALLOFF ),
	  "Same as TRITB_CAMERA_ROTATION_ALIGNED, except the billboard is scaled"
	  "by distance. This is useful for lights that are not rotationally symmetric." },
	{ "TRITB_CAMERA_ROTATION_FALLOFF_SYMMETRY",
	  VAL( TRITB_CAMERA_ROTATION_FALLOFF_SYMMETRY ),
	  "Same as TRITB_CAMERA_ROTATION_ALIGNED_SYMMETRY, except the billboard is scaled"
	  "by distance. This is useful for lights that are rotationally symmetric." },
	{ "TRITB_BOOSTER",
	  VAL( TRITB_BOOSTER ),
	  "Makes the sprite behave as if it were an ellipsoid" },
	{ "TRITB_SIMPLE_HALO",
	  VAL( TRITB_SIMPLE_HALO ),
	  "Ask Torfi" },
	{ "TRITB_SIMPLE_HALO_FALLOFF",
	  VAL( TRITB_SIMPLE_HALO_FALLOFF ),
	  "Ask Torfi" },
	{ "TRITB_SIMPLE_HALO_SYMMETRY",
	  VAL( TRITB_SIMPLE_HALO_SYMMETRY ),
	  "Ask Torfi" },
	{ "TRITB_SIMPLE_SPRITE",
	  VAL( TRITB_SIMPLE_SPRITE ),
	  "Ask Torfi" },
	{ "TRITB_SIMPLE_SPRITE_FALLOFF",
	  VAL( TRITB_SIMPLE_SPRITE_FALLOFF ),
	  "Ask Torfi" },
	{ "TRITB_SIMPLE_SPRITE_CONSTANT",
	  VAL( TRITB_SIMPLE_SPRITE_CONSTANT ),
	  "Don't ask Torfi" },
	{ "TRITB_FIXED",
	  VAL( TRITB_FIXED ),
	  "This placed traditionally called world-space, the transform is considered to "
	  "be in the world cordinate system (object hierarchies are ignored)"
	  "This leves the texture space untouched (set to the identity matrix)." },
	{ "TRITB_BOOSTER_FALLOFF",
	  VAL( TRITB_BOOSTER_FALLOFF ),
	  "Makes the sprite behave as if it were an ellipsoid, includes falloff" },
	{ "TRITB_WORLD",
	  VAL( TRITB_WORLD ),
	  "Transforms the object relative to the world zero " },
	{ 0 }
};

const Be::VarChooser TriOperator[] = {
	{ "TRIOP_MULTIPLY",
	  VAL( TRIOP_MULTIPLY ),
	  "multiply" },
	{ "TRIOP_ADD",
	  VAL( TRIOP_ADD ),
	  "add" },
	{ "TRIOP_AVERAGE",
	  VAL( TRIOP_AVERAGE ),
	  "average" },
	{ 0 }
};

const Be::VarChooser TriBlendOp[] = {
	{ "TRIBLENDOP_DISABLE",
	  VAL( Tr2RenderContextEnum::BO_DISABLE ),
	  "No blending\n" },
	{ "TRIBLENDOP_ADD",
	  VAL( Tr2RenderContextEnum::BO_ADD ),
	  "The result is the destination added to the source.\n"
	  "Result = Source + Destination" },
	{ "TRIBLENDOP_SUBTRACT",
	  VAL( Tr2RenderContextEnum::BO_SUBTRACT ),
	  "The result is the destination subtracted from to the source.\n"
	  "Result = Source - Destination" },
	{ "TRIBLENDOP_REVSUBTRACT",
	  VAL( Tr2RenderContextEnum::BO_REVSUBTRACT ),
	  "The result is the source subtracted from the destination.\n"
	  "Result = Destination - Source" },
	{ "TRIBLENDOP_MIN",
	  VAL( Tr2RenderContextEnum::BO_MIN ),
	  "The result is the minimum of the source and destination.\n"
	  "Result = MIN(Source, Destination)" },
	{ "TRIBLENDOP_MAX",
	  VAL( Tr2RenderContextEnum::BO_MAX ),
	  "The result is the maximum of the source and destination.\n"
	  "Result = MAX(Source, Destination)" },
	{ 0 }
};

const Be::VarChooser TriBlend[] = {
	{ "TRIBLEND_ZERO",
	  VAL( Tr2RenderContextEnum::BM_ZERO ),
	  "Blend factor is (0, 0, 0, 0)." },
	{ "TRIBLEND_ONE",
	  VAL( Tr2RenderContextEnum::BM_ONE ),
	  "Blend factor is (1, 1, 1, 1)." },
	{ "TRIBLEND_SRCCOLOR",
	  VAL( Tr2RenderContextEnum::BM_SRCCOLOR ),
	  "Blend factor is (Rs, Gs, Bs, As)." },
	{ "TRIBLEND_INVSRCCOLOR",
	  VAL( Tr2RenderContextEnum::BM_INVSRCCOLOR ),
	  "Blend factor is (1-Rs, 1-Gs, 1-Bs, 1-As)." },
	{ "TRIBLEND_SRCALPHA",
	  VAL( Tr2RenderContextEnum::BM_SRCALPHA ),
	  "Blend factor is (As, As, As, As)." },
	{ "TRIBLEND_INVSRCALPHA",
	  VAL( Tr2RenderContextEnum::BM_INVSRCALPHA ),
	  "Blend factor is (1-As, 1-As, 1-As, 1-As)." },
	{ "TRIBLEND_DESTALPHA",
	  VAL( Tr2RenderContextEnum::BM_DESTALPHA ),
	  "Blend factor is (Ad, Ad, Ad, Ad)." },
	{ "TRIBLEND_INVDESTALPHA",
	  VAL( Tr2RenderContextEnum::BM_INVDESTALPHA ),
	  "Blend factor is (1-Ad, 1-Ad, 1-Ad, 1-Ad)." },
	{ "TRIBLEND_DESTCOLOR",
	  VAL( Tr2RenderContextEnum::BM_DESTCOLOR ),
	  "Blend factor is (Rd, Gd, Bd, Ad)." },
	{ "TRIBLEND_INVDESTCOLOR",
	  VAL( Tr2RenderContextEnum::BM_INVDESTCOLOR ),
	  "Blend factor is (1-Rd, 1-Gd, 1-Bd, 1-Ad)." },
	{ "TRIBLEND_SRCALPHASAT",
	  VAL( Tr2RenderContextEnum::BM_SRCALPHASAT ),
	  "Blend factor is (f, f, f, 1); f = min(As, 1-Ad)." },
	{ "TRIBLEND_BOTHINVSRCALPHA",
	  VAL( Tr2RenderContextEnum::BM_BOTHINVSRCALPHA ),
	  "Source blend factor is (1-As, 1-As, 1-As, 1-As), and destination blend factor is "
	  "(As, As, As, As); the destination blend selection is overridden. This blend mode is "
	  "supported only for the D3DRS_SRCBLEND render state." },
	{ 0 }
};


#if DEPRECATED_ENABLED

const Be::VarChooser TriTextureAddress[] = {
	{ "TRITADDRESS_WRAP",
	  VAL( Tr2RenderContextEnum::TA_WRAP ),
	  "Tile the texture at every integer junction. For example, "
	  "for u values between 0 and 3, the texture is repeated three "
	  "times; no mirroring is performed. " },
	{ "TRITADDRESS_MIRROR",
	  VAL( Tr2RenderContextEnum::TA_MIRROR ),
	  "Similar to D3DTADDRESS_WRAP, except that the texture is flipped at "
	  "every integer junction. For u values between 0 and 1, for example, the "
	  "texture is addressed normally; between 1 and 2, the texture is flipped "
	  "(mirrored); between 2 and 3, the texture is normal again, and so on. " },
	{ "TRITADDRESS_CLAMP",
	  VAL( Tr2RenderContextEnum::TA_CLAMP ),
	  "Texture coordinates outside the range [0.0, 1.0] are set to "
	  "the texture color at 0.0 or 1.0, respectively. " },
	{ "TRITADDRESS_BORDER",
	  VAL( Tr2RenderContextEnum::TA_BORDER ),
	  "Texture coordinates outside the range [0.0, 1.0] are set "
	  "to the border color." },
	{ "TRITADDRESS_MIRRORONCE",
	  VAL( Tr2RenderContextEnum::TA_MIRROR_ONCE ),
	  "Similar to D3DTADDRESS_MIRROR and D3DTADDRESS_CLAMP. Takes the "
	  "absolute value of the texture coordinate (thus, mirroring around 0), and "
	  "then clamps to the maximum value. The most common usage is for volume textures, "
	  "where support for the full D3DTADDRESS_MIRRORONCE texture-addressing mode is not "
	  "necessary, but the data is symmetric around the one axis. " },
	{ 0 }
};


#endif


const Be::VarChooser TriD3DRenderState[] = {
	KV_RC(
		RS_ZENABLE,
		"Depth-buffering state as one member of the ZBUFFERTYPE enumerated type.\n"
		"Set this state to ZB_TRUE to enable z-buffering, ZB_USEW to enable\n"
		"w-buffering, or ZB_FALSE to disable depth buffering. The default value\n"
		"for this render state is ZB_TRUE if a depth stencil was created along\n"
		"with the swap chain by setting the EnableAutoDepthStencil member of the\n"
		"PRESENT_PARAMETERS structure to TRUE, and ZB_FALSE otherwise." ),
	KV_RC(
		RS_FILLMODE,
		"One or more members of the FILLMODE enumerated type.\n"
		"The default value is FILL_SOLID." ),
	KV_RC( RS_ZWRITEENABLE, "" ),
	KV_RC( RS_SRCBLEND, "" ),
	KV_RC( RS_DESTBLEND, "" ),
	KV_RC( RS_CULLMODE, "" ),
	KV_RC( RS_ZFUNC, "" ),
	KV_RC( RS_ALPHABLENDENABLE, "" ),
	KV_RC( RS_STENCILENABLE, "" ),
	KV_RC( RS_STENCILFAIL, "" ),
	KV_RC( RS_STENCILZFAIL, "" ),
	KV_RC( RS_STENCILPASS, "" ),
	KV_RC( RS_STENCILFUNC, "" ),
	KV_RC( RS_STENCILREF, "" ),
	KV_RC( RS_STENCILMASK, "" ),
	KV_RC( RS_STENCILWRITEMASK, "" ),
	KV_RC( RS_COLORWRITEENABLE, "" ),
	KV_RC( RS_BLENDOP, "" ),
	KV_RC( RS_SLOPESCALEDEPTHBIAS, "" ),
	KV_RC( RS_TWOSIDEDSTENCILMODE, "" ),
	KV_RC( RS_CCW_STENCILFAIL, "" ),
	KV_RC( RS_CCW_STENCILZFAIL, "" ),
	KV_RC( RS_CCW_STENCILPASS, "" ),
	KV_RC( RS_CCW_STENCILFUNC, "" ),
	KV_RC( RS_SRGBWRITEENABLE, "" ),
	KV_RC( RS_DEPTHBIAS, "" ),
	KV_RC( RS_SEPARATEALPHABLENDENABLE, "" ),
	KV_RC( RS_SRCBLENDALPHA, "" ),
	KV_RC( RS_DESTBLENDALPHA, "" ),
	KV_RC( RS_BLENDOPALPHA, "" ),

	{ 0 }
};
