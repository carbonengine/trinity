#include "StdAfx.h"
#include "Tr2DepthStencil.h"

BLUE_DEFINE( Tr2DepthStencil );

const Be::ClassInfo* Tr2DepthStencil::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2DepthStencil, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( Tr2DepthStencil )
		MAP_INTERFACE( ITr2TextureProvider )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			6,
			"Provide no arguments, and call Create later, or provide\n"
			":param width: buffer width\n"
			":param height: buffer height\n"
			":param format: buffer format (trinity.DEPTH_STENCIL_FORMAT)\n"
			":param msaaType: sample count\n"
			":param msaaQuality: MSAA quality\n"
			":param flags: combination of trinity.EX_FLAG"
		);

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"Create",
			Create,
			1,
			":jessica-deprecated:\n"
			":param width: buffer width\n"
			":param height: buffer height\n"
			":param format: buffer format (trinity.DEPTH_STENCIL_FORMAT)\n"
			":param msaaType: sample count\n"
			":param msaaQuality: MSAA quality"
			":param flags: trinity.EX_FLAG"
		)

		MAP_ATTRIBUTE( "name", m_name, "", Be::PERSISTONLY );
		MAP_PROPERTY( "name", GetName, SetName, "" )

		MAP_PROPERTY_READONLY( "width", GetWidth, "" );
		MAP_PROPERTY_READONLY( "height", GetHeight, "" );
		MAP_PROPERTY_READONLY( "multiSampleType", GetMsaaSamples, "" );
		MAP_PROPERTY_READONLY( "multiSampleQuality", GetMsaaQuality, "" );
		MAP_PROPERTY_READONLY( "format", GetFormat, "" );
		MAP_PROPERTY_READONLY( "mipCount", GetMipCount, "" );
		MAP_PROPERTY_READONLY( "multiSampleType", GetMsaaSamples, "" );
		MAP_PROPERTY_READONLY( "multiSampleQuality", GetMsaaQuality, "" );

		MAP_PROPERTY_READONLY( "isValid", IsValid, "is the graphics object successfully creaed" );
		MAP_PROPERTY_READONLY( "isReadable", IsReadable, "can the DS be used as a texture" );

		MAP_METHOD_AND_WRAP( 
			"sharedHandle",	
			GetSharedHandle, 
			"sharedHandle" 
			);

		MAP_METHOD_AND_WRAP
		(
			"HasALObject",
			HasALObject,
			"Returns True iff Tr2DepthStencil contains a reference to passed AL object ID.\n"
			"Used for debugging along with trinity.GetLiveALResources.\n"
			":param alType: AL object type (trinity.AL_OBJECT_TYPE)\n"
			":param alObject: AL object ID"
		)

	EXPOSURE_END()
}
