#ifndef IEveSpaceObjectChild_H
#define IEveSpaceObjectChild_H

#include "../../EveLODHelper.h"

class TriFrustum;
struct ITr2Renderable;
class EveSpaceObject2;
class EveUpdateContext;

BLUE_INTERFACE( IEveSpaceObjectChild ) : public IRoot
{
	virtual void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform ) = 0;
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const = 0;
	
	virtual void UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent ) = 0;
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent ) = 0;
	
	virtual void PlayCurveSet( const std::string& name ) = 0;
	virtual void StopCurveSet( const std::string& name ) = 0;
	virtual float GetCurveSetDuration( const std::string& name ) const = 0;

	virtual void Transform( const Vector3* scale, const Quaternion* rotation, const Vector3* translation ) = 0;
};

BLUE_DECLARE_IVECTOR( IEveSpaceObjectChild );

#endif