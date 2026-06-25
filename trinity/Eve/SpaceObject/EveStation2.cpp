// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "EveStation2.h"

#include "Tr2Mesh.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveStation2::EveStation2( IRoot* lockobj ) :
	EveSpaceObject2( lockobj )
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
//   Override base ::PrepareShaderData() function
// --------------------------------------------------------------------------------
void EveStation2::PrepareShaderData( const EveUpdateContext& updateContext )
{
	EveSpaceObject2::PrepareShaderData( updateContext );

	m_spaceObjectShipData.y *= m_activationStrength;
}

// --------------------------------------------------------------------------------
// Description:
//   Override base ::UpdateSyncronous() function, so we can update the turrets and
//   their positions (if they are attached to animated bones!)
// --------------------------------------------------------------------------------
void EveStation2::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	EveSpaceObject2::GetBatches( batches, batchType, perObjectData, reason );
}
