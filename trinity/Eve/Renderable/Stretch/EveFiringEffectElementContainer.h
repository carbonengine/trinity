////////////////////////////////////////////////////////////
//
//    Created:   January 2017
//    Copyright: CCP 2017
//

#pragma once

#include "Eve/IEveSpaceObject2.h"
#include "Lights/ITr2LightOwner.h"

BLUE_DECLARE_INTERFACE( IEveFiringEffectElement );


// --------------------------------------------------------------------------------------
// Description:
//   A simple top-level container to host an IEveFiringEffectElement element for easy 
//   editing.
// --------------------------------------------------------------------------------------
BLUE_CLASS( EveFiringEffectElementContainer ) :
	public IEveSpaceObject2,
	public ITr2LightOwner
{
public:
	EXPOSE_TO_BLUE();

	EveFiringEffectElementContainer( IRoot* lockObj = nullptr );

	virtual void UpdateSyncronous( EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext );
	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	
	// lights
	virtual void GetLights( Tr2LightManager& lightManager ) const;
	virtual void AddLight( Tr2Light* light ){};
	virtual void ClearLights(){};

	// This version of the function should perform an update on the model / ball position
	virtual void UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t );

	// This version of the function should not update the object
	virtual void GetModelCenterWorldPosition( Vector3 &position ) const;

	// If possible, return an AABB in local coordinates
	virtual bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );
	// Get the local to world transform
	virtual void GetLocalToWorldTransform( Matrix &transform ) const;

	void StartFiring();
	void StopFiring();

	void SetActive( bool active );
	bool GetActive() const;
private:

	IEveFiringEffectElementPtr m_element;

	Matrix m_source;
	Vector3 m_destination;

	float m_destinationScale;
	bool m_display;
	bool m_useSourceTransform;
	bool m_displaySource;
	bool m_displayDestination;
	bool m_isActive;
};

TYPEDEF_BLUECLASS( EveFiringEffectElementContainer );