#include "StdAfx.h"
#include "Tr2RenderTarget.h"

BLUE_DEFINE( Tr2RenderTarget );

const Be::ClassInfo* Tr2RenderTarget::ExposeToBlue()
{
	/////////////////////////////////////////
	// Blue class info
    EXPOSURE_BEGIN( Tr2RenderTarget, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( Tr2RenderTarget )
		MAP_INTERFACE( ITr2TextureProvider )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			8,
			"Provide no arguments, and call Create/CreateMsaa later, or provide\n"
			"width, height, mipCount, trinity.PIXEL_FORMAT, msaaType=0, msaaQuality=0, flags=0, type=0.\n"
			":param width: render target width\n"
			":param height: render target height\n"
			":param mipCount: number of mip levels (0 = full pyramid)\n"
			":param format: pixel format (trinity.PIXEL_FORMAT)\n"
			":param msaaType: number of samples\n"
			":param msaaQuality: MSAA quality\n"
			":param flags: trinity.EX_FLAG\n"
			":param type: texture type (2D or CUBE)"
		)


		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"Create",
			Create,
			4,
			"Creates a new render target\n"
			":jessica-deprecated:\n"
			":param width: render target width\n"
			":param height: render target height\n"
			":param mipCount: number of mip levels (0 = full pyramid)\n"
			":param format: pixel format (trinity.PIXEL_FORMAT)\n"
			":param msaaType: number of samples\n"
			":param msaaQuality: MSAA quality\n"
			":param flags: trinity.EX_FLAG\n"
			":param type: texture type (2D or CUBE)"
		)

		MAP_METHOD_AND_WRAP
		(
			"GenerateMipMaps",
			GenerateMipMaps,
			"Generate mipmaps, if any"
		)
				
		MAP_METHOD_AND_WRAP
		(
			"Resolve",
			Resolve,
			"Resolve a renderTarget (typically MSAA ) into another RT (typically non-MSAA).\n"
			"May also work for just copying non-MSAA to non-MSAA, I'm not sure :P\n"
			":param destination: Tr2RenderTarget"
		)

		MAP_ATTRIBUTE( "name", m_name, "", Be::PERSISTONLY );
		MAP_PROPERTY( "name", GetName, SetName, "" )

		MAP_PROPERTY_READONLY( "width", GetWidth, "" );
		MAP_PROPERTY_READONLY( "height", GetHeight, "" );		
		MAP_PROPERTY_READONLY( "mipCount", GetMipCount, "" );
		MAP_PROPERTY_READONLY( "multiSampleType", GetMsaaType, "" );
		MAP_PROPERTY_READONLY( "multiSampleQuality", GetMsaaQuality, "" );

		MAP_PROPERTY_READONLY( "format", GetFormat,	"" );
		MAP_PROPERTY_READONLY( "type", GetType, "" );

		MAP_PROPERTY_READONLY( "isValid", IsValid, "is the graphics object successfully creaed" );
		MAP_PROPERTY_READONLY( "isReadable", IsReadable, "can the RT be used as a texture" );

		MAP_METHOD_AND_WRAP( 
			"sharedHandle",	
			GetSharedHandle, 
			"sharedHandle" 
			);

		MAP_METHOD_AND_WRAP
		(
			"HasALObject",
			HasALObject,
			"Returns True iff Tr2RenderTarget contains a reference to passed AL object ID.\n"
			"Used for debugging along with trinity.GetLiveALResources.\n"
			":param alType: AL object type (trinity.AL_OBJECT_TYPE)\n"
			":param alObject: AL object ID"
		)

	EXPOSURE_END()
}
