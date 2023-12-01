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
	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform ) = 0;
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables ) = 0;
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const = 0;
	virtual Tr2Lod GetLODLevel() const = 0;
};

BLUE_DECLARE_IVECTOR( IEveTransform );

#endif