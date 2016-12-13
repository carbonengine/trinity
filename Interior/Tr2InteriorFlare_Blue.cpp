#include "StdAfx.h"

#include "Tr2InteriorFlare.h"
#include "TriViewport.h"

BLUE_DEFINE( Tr2InteriorFlare );

const Be::ClassInfo* Tr2InteriorFlare::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2InteriorFlare, "" )
        MAP_INTERFACE( Tr2InteriorFlare )
		MAP_INTERFACE( ITr2Interior )
		MAP_INTERFACE( ITr2InteriorDynamic )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2Renderable )

		MAP_ATTRIBUTE( "name", m_name, "The name of this interior light source", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transform", m_transform, "Transformation matrix from flare local space to world space", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_PROPERTY( "position", GetPosition, SetPosition, "Position of light emitter" )
		MAP_ATTRIBUTE( "color", m_color, "The light's color", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curve sets to animate light attributes", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "occluderSize", m_occluderSize, "Radius of occluder billboard in world units", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "flareIntensity", m_flareIntensity, "Flare intensity (for debugging)", Be::READ )
		MAP_ATTRIBUTE( "occluderMaterial", m_occluderMaterial, "Flare occluder shader material", Be::READWRITE )
		MAP_ATTRIBUTE( "transparentFlareMaterial", m_transparentFlareMaterial, "Flare shader material for rendering flares during material pass", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "flareMaterial", m_flareMaterial, "Flare shader material for rendering flares during flare pass", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "flareData", m_flareData, "Flare image data vector for flare pass", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transparentFlareData", m_transparentFlareData, "Flare image data vector for material pass", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "updateVisibility", m_updateVisibility, "Does the flare need to update its visibility", Be::READWRITE | Be::NOTIFY )
		MAP_METHOD_AND_WRAP( 
			"OverrideViewport", 
			OverrideViewport, 
			"Overrides render target size (for tiled rendering)"
			"\n:param xviewport: overriden viewport (TriViewport or None)" )
		MAP_ATTRIBUTE( "debugIsHidden", m_debugIsHidden, "Hide flare", Be::READWRITE )

		// Bounding boxes
		MAP_METHOD_AND_WRAP( "GetBoundingBoxInLocalSpace", GetBoundingBoxInLocalSpace, "Gets the bounding box in local space" )
		MAP_METHOD_AND_WRAP( "GetBoundingBoxInWorldSpace", GetBoundingBoxInWorldSpace, "Gets the bounding box in world space" )

	EXPOSURE_END()
}
