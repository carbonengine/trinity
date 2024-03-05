////////////////////////////////////////////////////////////
//
//    Created:   January 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "EveUiObject.h"

#include "TriFrustum.h"
#include "Tr2MeshBase.h"

// --------------------------------------------------------------------------------
EveUiObject::EveUiObject( IRoot* lockobj ) :
	EveSpaceObject2( lockobj ),
	m_usePerspectiveScale( true )
{
}

// --------------------------------------------------------------------------------
EveUiObject::~EveUiObject()
{
}

// --------------------------------------------------------------------------------
void EveUiObject::UpdateWorldTransform( Be::Time time )
{
	EveSpaceObject2::UpdateWorldTransform( time );

	if( !m_usePerspectiveScale )
	{
		// add a scaling to the worldmatrix based on camera distance to make this guy not perspective
		XMVECTOR d = m_worldPosition - Tr2Renderer::GetViewPosition();
		XMVECTOR dist = XMVector3Length( d );

		XMMATRIX scaleMatrix = XMMatrixScalingFromVector( dist );
		m_worldTransform = XMMatrixMultiply( scaleMatrix, m_worldTransform );
	}
}

// --------------------------------------------------------------------------------
void EveUiObject::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
	EveSpaceObject2::UpdateVisibility( frustum, parentTransform );

	// no matter what gets calculated, ui models have NO lod
	m_isVisible = m_isMeshVisible = m_isInFrustum = true;
	m_lodLevel = TR2_LOD_HIGH;
	m_lodLevelWithChildren = TR2_LOD_HIGH;
	m_impostorMode = false;
}

// --------------------------------------------------------------------------------
std::string EveUiObject::GetNameForPickingAreaID( uint32_t areaID ) const
{
	Tr2MeshBase* mesh = GetMesh();
	if( mesh == nullptr )
	{
		return "invalid_mesh";
	}

	// find area with correct areaID, but only in the picking areas!
	const Tr2MeshAreaVector* pickingAreas = mesh->GetAreas( TRIBATCHTYPE_PICKING );
	if( pickingAreas )
	{
		for( Tr2MeshAreaVector::const_iterator it = pickingAreas->begin(); it != pickingAreas->end(); ++it )
		{
			Tr2MeshAreaPtr area = *it;
			if( area->GetIndex() == areaID )
			{
				return area->GetName();
			}
		}
	}
	return "invalid_areaid";
}

// --------------------------------------------------------------------------------
void EveUiObject::SetVisibilityForArea( const char* areaName, bool enable )
{
	Tr2MeshBase* mesh = GetMesh();
	if( mesh == nullptr )
	{
		return;
	}

	// find all areas with the provided name and toggle visibility
	for( uint32_t i = 0; i < TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
	{
		const Tr2MeshAreaVector* areas = mesh->GetAreas( ( TriBatchType)i );
		if( areas )
		{
			for( Tr2MeshAreaVector::const_iterator it = areas->begin(); it != areas->end(); ++it )
			{
				Tr2MeshAreaPtr area = *it;
				if( area->GetName().compare( areaName ) == 0 )
				{
					area->SetDisplay( enable );
				}
			}
		}
	}
}
