// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2RuntimeTextureParameter.h"
#include "ITr2TextureProvider.h"

BLUE_DEFINE( Tr2RuntimeTextureParameter );

const Be::ClassInfo* Tr2RuntimeTextureParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2RuntimeTextureParameter, "" )

		MAP_INTERFACE( Tr2RuntimeTextureParameter )
		MAP_INTERFACE( ITriEffectResourceParameter )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "texture", m_texture, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "uavMipLevel", m_uavMipLevel, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			Create,
			3,
			":param name: parameter name\n"
			":param texture: texture associated with the parameter\n"
			":param mip: UAV mip level" )

	EXPOSURE_END()
}
