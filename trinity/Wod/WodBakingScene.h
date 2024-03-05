#pragma once
#ifndef WodBakingScene_H
#define WodBakingScene_H

#include "include/ITr2Scene.h"
#include "Interior/Tr2InteriorVisualization.h"

BLUE_DECLARE( WodBakingScene );
BLUE_DECLARE( Tr2SkinnedObject );

class WodBakingScene:
	public ITr2Scene
{
public:
	WodBakingScene( IRoot* lockobj = NULL );
	~WodBakingScene();

	EXPOSE_TO_BLUE();

	virtual void Render( Tr2RenderContext& renderContext );	
	virtual void RenderDebugInfo( Tr2RenderContext& renderContext );
	virtual void Update( Be::Time realTime, Be::Time simTime );

private:
	ITriRenderBatchAccumulator* m_opaqueRenderBatches;
	//TriRenderBatchAccumulator* m_transparentRenderBatches;
	Tr2SkinnedObjectPtr m_skinnedObject;

	// Visualization
	VisualizeMethod m_visualizeMethod;

	Tr2ConstantBufferAL	m_VSBuffer, m_PSBuffer;
};

TYPEDEF_BLUECLASS( WodBakingScene );

#endif