// Copyright © 2025 CCP ehf.

#pragma once
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Eve/SpaceObject/Utils/EveDistributionMethods/IEveDistributionMethod.h"
#include "Tr2DebugRenderer.h"

BLUE_INTERFACE( IEveSmartLightGroup ) :
	public IRoot
{
public:
	virtual void SetColor( const Color& color ) {};
	virtual void AddQuadsToQuadRenderer( const PlacementDataWithIdentifierStructureList& placements, size_t size, const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const {};
	virtual void GetRenderables( std::vector<ITr2Renderable*> & renderables ){};
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution ) {};
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution ) {};
	virtual void SetControllerVariable( const char* name, float value ) {};
	virtual void SetInheritProperties( const Color* colorSet ) {};
	virtual void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) {};
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ){};
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const PlacementDataWithIdentifierStructureList& placements, size_t size ) {};
};
