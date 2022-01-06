////////////////////////////////////////////////////////////
//
//    Created:   July 2018
//    Copyright: CCP 2018
//

#pragma once

#include "ITr2Renderable.h"
#include "Tr2DebugRenderer.h"
#include "Tr2LightManager.h"


class Tr2QuadRenderer;
class TriFrustum;


BLUE_INTERFACE( IEveSpaceObjectAttachment ) : public IRoot
{
	virtual bool UpdateVisibility( const TriFrustum & frustum, const Matrix & parentTransform, const granny_matrix_3x4 * bones, size_t boneCount ) { return false; }

	virtual void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData * perObjectData, Tr2RenderReason reason = Tr2RenderReason::TR2RENDERREASON_NORMAL ) {}

	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) {}
	virtual void AddToQuadRenderer( Tr2QuadRenderer & quadRenderer, const Matrix & parentTransform, float activation, float boosterGain, const granny_matrix_3x4 * bones, size_t boneCount ) {}

	virtual void GetDebugOptions( Tr2DebugRendererOptions & options ) {}
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix & parentTransform, const granny_matrix_3x4 * bones, size_t boneCount ) {}

	virtual void SetShaderOption( const BlueSharedString & name, const BlueSharedString & value ) {}

	virtual void GetLights( Tr2LightManager & lightManager, const Matrix & parentTransform ) const {}
};
