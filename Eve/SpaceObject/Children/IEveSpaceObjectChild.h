#ifndef IEveSpaceObjectChild_H
#define IEveSpaceObjectChild_H

#include "Eve/IEveSpaceObject2.h"

class TriFrustum;
struct ITr2Renderable;
class EveSpaceObject2;
class EveUpdateContext;
class Tr2LightManager;

BLUE_INTERFACE( IEveSpaceObjectChild ) : public IRoot
{
	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod ) = 0;
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables ) = 0;
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const = 0;
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer ) {}
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const {}
	
	virtual void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent ) = 0;
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent ) = 0;

	virtual void GetLocalToWorldTransform( Matrix& transform ) const = 0;

	virtual void PlayCurveSet( const std::string& name ) = 0;
	virtual void StopCurveSet( const std::string& name ) = 0;
	virtual void UpdateCurveSet( const std::string& name, Be::Time time ) {};
	virtual float GetCurveSetDuration( const std::string& name ) const = 0;
	
	virtual bool IsAlwaysOn() const { return false; };	

	virtual void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) = 0;

	virtual void ChangeLOD( Tr2Lod lod ) = 0;

	virtual void GetLights( Tr2LightManager& lightManager ) const = 0;
};

BLUE_DECLARE_IVECTOR( IEveSpaceObjectChild );

#endif