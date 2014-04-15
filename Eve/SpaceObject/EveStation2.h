#ifndef EVESTATION2_H
#define EVESTATION2_H

#include "EveSpaceObject2.h"

// forwards
BLUE_DECLARE( EveStation2 );
BLUE_DECLARE( EveDynamicPlaneSet );
BLUE_DECLARE_VECTOR( EveDynamicPlaneSet );
BLUE_DECLARE( EveSpriteSet );
BLUE_DECLARE_VECTOR( EveSpriteSet );

class EveStation2 :
	public EveSpaceObject2
{
public:
	EXPOSE_TO_BLUE();

	EveStation2( IRoot* lockobj = NULL );
	~EveStation2();

	/////////////////////////////////////////////////////////////////////////////////////
	// Overrides of EveSpaceObject2 implementations
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	virtual void EveStation2::UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const;

private:
	/////////////////////////////////////////////////////////////////////////////////////
	// dynamic planesets (used for holograms, etc.)
	PEveDynamicPlaneSetVector m_hologramSets;

	/////////////////////////////////////////////////////////////////////////////////////
	// environment spritesets (used for landing strips etc.)
	PEveSpriteSetVector m_environmentSpriteSets;
};

TYPEDEF_BLUECLASS( EveStation2 );

#endif
