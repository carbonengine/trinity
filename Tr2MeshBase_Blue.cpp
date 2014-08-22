////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2MeshBase.h"

BLUE_DEFINE_ABSTRACT( Tr2MeshBase );


const Be::ClassInfo* Tr2MeshBase::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2MeshBase, "" )

		MAPHIDEABLE()

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "meshIndex", m_meshIndex, "The index of the mesh within the granny file to use", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "opaqueAreas", m_opaqueAreas, "Areas that are rendered sorted by effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "decalAreas", m_decalAreas, "Areas that are rendered in the order that they exist, before transparency", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depthAreas", m_depthAreas, "Areas that are rendered in the order to render depth information", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transparentAreas", m_transparentAreas, "Areas are rendered in the order that they exist in the mesh, sorted by the mesh center", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "additiveAreas", m_additiveAreas, "Areas that are rendered sorted by effect, after transparencies", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "pickableAreas", m_pickableAreas, "Areas that are rendered for picking only", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "mirrorAreas", m_mirrorAreas, "Areas that define mirrors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "decalNormalAreas", m_decalNormalAreas, "Areas that provide normals for prepass rendering but do not affect depth", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depthNormalAreas", m_depthNormalAreas, "Areas that provide depth and normals for prepass rendering", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "opaquePrepassAreas", m_opaquePrepassAreas, "Prepass areas that are rendered sorted by effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "decalPrepassAreas", m_decalPrepassAreas, "Prepass areas that are rendered in the order that they exist, before transparency", Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( "geometryEraserAreas", m_geometryEraserAreas, "Areas that erase geometry", Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( "distortionAreas", m_distortionAreas, "", Be::READWRITE | Be::PERSIST )
		
    EXPOSURE_END()
}
