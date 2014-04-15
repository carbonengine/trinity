#include "StdAfx.h"

#include "EveStation2.h"

#include "Tr2Mesh.h"
#include "Attachments/EveDynamicPlaneSet.h"
#include "Attachments/EveSpriteSet.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveStation2::EveStation2( IRoot* lockobj ) : 
	EveSpaceObject2( lockobj ),
	PARENTLOCK( m_hologramSets ),
	PARENTLOCK( m_environmentSpriteSets )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Destr
// --------------------------------------------------------------------------------
EveStation2::~EveStation2()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Override base ::UpdateSyncronous() function, so we can update the turrets and 
//   their positions (if they are attached to animated bones!)
// --------------------------------------------------------------------------------
void EveStation2::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	EveSpaceObject2::GetBatches( batches, batchType, perObjectData );

	if( !m_mesh )
	{
		return;
	}

	if( m_mesh->IsHidden() )
	{
		return;
	}

	// only additives here
	if( batchType == TRIBATCHTYPE_ADDITIVE )
	{
		for( auto it = m_environmentSpriteSets.begin(); it != m_environmentSpriteSets.end(); ++it )
		{
			(*it)->GetBatches( batches, perObjectData );
		}
		for (auto it = m_hologramSets.begin(); it != m_hologramSets.end(); ++it )
		{
			(*it)->GetBatches( batches, perObjectData );
		}
	}

}

// --------------------------------------------------------------------------------
// Description:
//   Update view distance info using this object's bounds. Should be called AFTER
//   GetRenderables is called or the object might be ignored.
// --------------------------------------------------------------------------------
void EveStation2::UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const
{
	EveSpaceObject2::UpdateViewDistanceInfo( frustum, viewDistance );

	if( !m_display || !m_isVisible )
	{
		return;
	}

	for( auto it = m_hologramSets.begin(); it != m_hologramSets.end(); it++ )
	{
		(*it)->UpdateViewDistanceInfo( frustum, viewDistance, m_worldTransform );
	}
	for( auto it = m_environmentSpriteSets.begin(); it != m_environmentSpriteSets.end(); it++ )
	{
//		(*it)->UpdateViewDistanceInfo( frustum, viewDistance, m_worldTransform );
	}
}





