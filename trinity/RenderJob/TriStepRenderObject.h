// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriStepRenderObject_h_
#define TriStepRenderObject_h_



#include "TriRenderStep.h"
#include "TriRenderBatch.h"
#include "ITr2Renderable.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2EffectStateManager.h"
#include "Eve/EveSpaceScene.h"

// Render any ITr2Renderable -- just gather its batches and submit.
BLUE_CLASS( TriStepRenderObject ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepRenderObject( IRoot* lockobj = 0 );
	~TriStepRenderObject();

	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	void py__init__( ITr2Renderable * obj );

private:
	ITr2RenderablePtr m_renderable;
	Tr2MaterialPtr m_effectOverride;
	EveSpaceScene::BatchMap m_batches;

	bool m_typeEnabled[4];

	TriStepRenderObject( const TriStepRenderObject& );
	TriStepRenderObject& operator=( const TriStepRenderObject& );
};

TYPEDEF_BLUECLASS( TriStepRenderObject );

#endif