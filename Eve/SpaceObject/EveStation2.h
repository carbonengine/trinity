#ifndef EVESTATION2_H
#define EVESTATION2_H

#include "EveSpaceObject2.h"

// forwards
BLUE_DECLARE( EveStation2 );
BLUE_DECLARE( EveDynamicPlaneSet );
BLUE_DECLARE_VECTOR( EveDynamicPlaneSet );
BLUE_DECLARE( EveSpriteSet );
BLUE_DECLARE_VECTOR( EveSpriteSet );

BLUE_CLASS( EveStation2 ):
	public EveSpaceObject2
{
public:
	EXPOSE_TO_BLUE();

	EveStation2( IRoot* lockobj = NULL );
	~EveStation2();

	/////////////////////////////////////////////////////////////////////////////////////
	// Overrides of EveSpaceObject2 implementations
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	virtual void UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// Overrides of animation controller
	virtual bool ExecuteAnimationStateCommand( EveAnimationCmd cmd, const std::string& data, const std::map<std::string, float>& parameters );

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
