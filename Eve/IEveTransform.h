#ifndef IEveTransform_H
#define IEveTransform_H

#include "EveLODHelper.h"

struct ViewDistanceInfo;
struct ITr2Renderable;
class TriFrustum;
class Tr2RenderContext;
class EveUpdateContext;

BLUE_INTERFACE( IEveTransform ) : public IRoot
{
	virtual void Update( EveUpdateContext& updateContext ) = 0;
	virtual void RenderDebugInfo( Tr2RenderContext& renderContext ) = 0;
	virtual void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform ) = 0;
    virtual void UpdateViewDependentData( const Matrix& parentTransform ) = 0;
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const = 0;
	virtual Tr2Lod GetLODLevel() const = 0;
	virtual void UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const = 0;
	virtual void PlayCurveSet( const std::string& name ) = 0;
	virtual void StopCurveSet( const std::string& name ) = 0;
	virtual float GetCurveSetDuration( const std::string& name ) const = 0; 
};

BLUE_DECLARE_IVECTOR( IEveTransform );

#endif