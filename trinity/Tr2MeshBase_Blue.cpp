// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "Tr2MeshBase.h"

BLUE_DEFINE_ABSTRACT( Tr2MeshBase );


const Be::ClassInfo* Tr2MeshBase::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MeshBase, "" )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Toggle visibility", Be::READWRITE )
		MAP_ATTRIBUTE( "meshIndex", m_meshIndex, "The index of the mesh within the granny file to use", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "opaqueAreas", m_opaqueAreas, "Areas that are rendered sorted by effect", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "decalAreas", m_decalAreas, "Areas that are rendered in the order that they exist, before transparency", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "depthAreas", m_depthAreas, "Areas that are rendered in the order to render depth information", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "transparentAreas", m_transparentAreas, "Areas are rendered in the order that they exist in the mesh, sorted by the mesh center", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "additiveAreas", m_additiveAreas, "Areas that are rendered sorted by effect, after transparencies", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "pickableAreas", m_pickableAreas, "Areas that are rendered for picking only", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "mirrorAreas", m_mirrorAreas, "Areas that define mirrors", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "decalNormalAreas", m_decalNormalAreas, "Areas that provide normals for prepass rendering but do not affect depth", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "depthNormalAreas", m_depthNormalAreas, "Areas that provide depth and normals for prepass rendering", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "opaquePrepassAreas", m_opaquePrepassAreas, "Prepass areas that are rendered sorted by effect", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "decalPrepassAreas", m_decalPrepassAreas, "Prepass areas that are rendered in the order that they exist, before transparency", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "geometryEraserAreas", m_geometryEraserAreas, "Areas that erase geometry", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "distortionAreas", m_distortionAreas, "", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "maxVertexScale", m_boundsAdjustment.maxLocalScale, "Max scaling applied by shaders in all areas to vertices of the mesh\n:jessica-group: Bounds", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "maxVertexDisplacement", m_boundsAdjustment.maxLocalDisplacement, "Max displacement applied by shaders in all areas to vertices of the mesh\n:jessica-group: Bounds", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "rotatesVertices", m_boundsAdjustment.rotatesVertices, "If a shader in any area rotates mesh vertices\n:jessica-group: Bounds", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "GetGeometryResPath", GetGeometryResPath, "Returns the respath to the currently used geometry" )
		MAP_METHOD_AND_WRAP( "GetAllAreas", GetAllAreas, "Returns a list of all areas (from all area type lists)" )

		MAP_METHOD_AND_WRAP( "GetMaterialBoundsAdjustment", GetMaterialBoundsAdjustment, "Returns material bounds adjustment parameters" )
		MAP_METHOD_AND_WRAP(
			"SetMaterialBoundsAdjustment",
			SetMaterialBoundsAdjustment,
			"Assigns material bounds adjustment parameters. Normally set by the publisher\n"
			":param adjustment: adjustment values extracted from shaders" )

	EXPOSURE_END()
}
