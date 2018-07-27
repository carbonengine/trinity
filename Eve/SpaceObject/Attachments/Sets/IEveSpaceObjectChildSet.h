////////////////////////////////////////////////////////////
//
//    Created:   July 2018
//    Copyright: CCP 2018
//

#pragma once

#include "ITr2Renderable.h"
#include "Tr2DebugRenderer.h"

class Tr2QuadRenderer;
class TriFrustum;


BLUE_INTERFACE( IEveSpaceObjectChildSet ): public IRoot
{
	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount ) {}

	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData ) {}

	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer ) {}
	virtual void AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& parentTransform, float activation, float boosterGain, const granny_matrix_3x4* bones, size_t boneCount ) {}

	virtual void GetDebugOptions( Tr2DebugRendererOptions& options ) {}
	virtual void RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount ) {}
};