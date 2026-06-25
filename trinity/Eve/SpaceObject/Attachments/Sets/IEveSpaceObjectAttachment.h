// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2Renderable.h"
#include "Tr2DebugRenderer.h"
#include "Tr2LightManager.h"
#include "Lights/Tr2Light.h"
#include "Eve/EveUpdateContext.h"

class Tr2QuadRenderer;
class TriFrustum;


BLUE_INTERFACE( IEveSpaceObjectAttachment ) :
	public IRoot{
		virtual bool UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, const Float4x3* bones, size_t boneCount ){ return false;
}

virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = Tr2RenderReason::TR2RENDERREASON_NORMAL )
{
}

virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
}
virtual void AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& parentTransform, float activation, float boosterGain, const Float4x3* bones, size_t boneCount )
{
}

virtual void GetDebugOptions( Tr2DebugRendererOptions& options )
{
}
virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const Float4x3* bones, size_t boneCount )
{
}

virtual void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
}

virtual void UpdateLights( const Matrix& parentTransform, const Float4x3* bones, size_t boneCount, float parentStrength, float boosterGain )
{
}
}
;
