// Copyright © 2023 CCP ehf.

#ifndef EVESTATION2_H
#define EVESTATION2_H

#include "EveSpaceObject2.h"

// forwards
BLUE_DECLARE( EveStation2 );
BLUE_DECLARE( EveSpriteSet );
BLUE_DECLARE_VECTOR( EveSpriteSet );

BLUE_CLASS( EveStation2 ) :
	public EveSpaceObject2
{
public:
	EXPOSE_TO_BLUE();

	EveStation2( IRoot* lockobj = NULL );
	~EveStation2();

	/////////////////////////////////////////////////////////////////////////////////////
	// Overrides of EveSpaceObject2 implementations
	virtual void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	void PrepareShaderData( const EveUpdateContext& updateContext ) override;
};

TYPEDEF_BLUECLASS( EveStation2 );

#endif
