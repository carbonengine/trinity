// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriTextureParameter.h"
#include "Resources/TriTextureRes.h"

BLUE_DEFINE( TriTextureParameter );

extern Be::VarChooser SamplerStateChooser_AddressMode[];
extern Be::VarChooser SamplerStateChooser_FilterMode[];
extern Be::VarChooser SamplerStateChooser_MipFilterMode[];

const Be::ClassInfo* TriTextureParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriTextureParameter, "" )
		MAP_INTERFACE( ITriEffectParameter )
		MAP_INTERFACE( ITriEffectResourceParameter )
		MAP_INTERFACE( ITriEffectTextureParameter )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ICopierCustomAssignment )

		////////////////////////////////////////////////////////////////////////////
		//	name
		MAP_ATTRIBUTE(
			"name",
			m_name,
			"na",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		////////////////////////////////////////////////////////////////////////////
		//	resource path (texture / buffer)
		// TODO: Add valid chooser
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"resourcePath",
			m_resourcePath,
			"Resource path to .x file",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST,
			NULL )
		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE(
			"resource",
			m_resource,
			"na",
			Be::READ )
		MAP_ATTRIBUTE( "uavMipLevel", m_uavMipLevel, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "positionScale", m_uvDensityScale[0], "Model position sampling density scale modifier for texture LOD\n:jessica-group: LOD", Be::READ )
		MAP_ATTRIBUTE( "uvDensityScale0", m_uvDensityScale[1], "UV0 density scale modifier for texture LOD\n:jessica-group: LOD", Be::READ )
		MAP_ATTRIBUTE( "uvDensityScale1", m_uvDensityScale[2], "UV1 density scale modifier for texture LOD\n:jessica-group: LOD", Be::READ )
		MAP_ATTRIBUTE( "uvDensityScale2", m_uvDensityScale[3], "UV2 density scale modifier for texture LOD\n:jessica-group: LOD", Be::READ )
		MAP_ATTRIBUTE( "uvDensityScale3", m_uvDensityScale[4], "UV3 density scale modifier for texture LOD\n:jessica-group: LOD", Be::READ )

		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE( "usedByCurrentTechnique", m_isUsedByEffect, "na", Be::READ )
		MAP_ATTRIBUTE( "usedByCurrentEffect", m_isUsedByEffect, "na", Be::READ )

		MAP_METHOD_AND_WRAP(
			"SetResource",
			SetResource,
			"Takes a TriTextureRes and sets it directly, without using a resourcePath.\n"
			":param resource: new texture resource" )
		MAP_METHOD_AND_WRAP( "GetResourcePath", GetResourcePath, "Returns the respath to the currently used texture. Might be LOD dependent." )

	EXPOSURE_END()
}
