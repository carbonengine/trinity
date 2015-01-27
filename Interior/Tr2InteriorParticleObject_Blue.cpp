////////////////////////////////////////////////////////////
//
//    Created:   September 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"

#include "Tr2InteriorParticleObject.h"

BLUE_DEFINE( Tr2InteriorParticleObject );

const Be::ClassInfo* Tr2InteriorParticleObject::ExposeToBlue()
{
	EXPOSURE_BEGIN(Tr2InteriorParticleObject, "" )
		MAP_INTERFACE( Tr2InteriorParticleObject )
		MAP_INTERFACE( ITr2Interior )
		MAP_INTERFACE( ITr2InteriorDynamic )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "Object name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "particleSystems", m_particleSystems, "Particle systems rendered by this object", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "emitters", m_emitters, "Particle emitters for this object", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "meshes", m_meshes, "Meshes used to render particle systems", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transform", m_transform, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "renderDebugInfo", m_renderDebugInfo, "Render particle system debug info", Be::READWRITE )
		MAP_ATTRIBUTE( "maxParticleRadius", m_maxParticleRadius, "Max particle radius (to calculate correct bounding box)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shBoundsMin", m_shBoundsMin, "Min bounds for SH lighting bounding box", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shBoundsMax", m_shBoundsMax, "Max bounds for SH lighting bounding box", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depthOffset", m_depthOffset, "Depth offset for transparency sorting", Be::READWRITE | Be::PERSIST )

		// Bounding boxes
		MAP_METHOD_AND_WRAP( "GetBoundingBoxInLocalSpace", GetBoundingBoxInLocalSpace, "Gets the bounding box in local space" )
		MAP_METHOD_AND_WRAP( "GetBoundingBoxInWorldSpace", GetBoundingBoxInWorldSpace, "Gets the bounding box in world space" )

		MAP_METHOD_AND_WRAP( "BindLowLevelShaders", BindLowLevelShaders, "Rebinds low level shaders on all meshes" )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curve sets to animate particle systems/emitters attributes", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
