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
//   Gets called by the state machine of this object to execute some command.
// Return Value:
//   Returns true if this implementation has handled the command.
// --------------------------------------------------------------------------------
bool EveStation2::ExecuteAnimationStateCommand( EveAnimationCmd cmd, const std::string& data, const std::map<std::string, float>& parameters )
{
	return EveSpaceObject2::ExecuteAnimationStateCommand( cmd, data, parameters );
}





