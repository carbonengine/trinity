// Copyright © 2017 CCP ehf.

#pragma once

#include "Eve/IEveSpaceObject2.h"
#include "Eve/EveEntity.h"
#include "Eve/IEveFiringEffectElement.h"

BLUE_DECLARE_INTERFACE( IEveFiringEffectElement );


// --------------------------------------------------------------------------------------
// Description:
//   A simple top-level container to host an IEveFiringEffectElement element for easy
//   editing.
// --------------------------------------------------------------------------------------
BLUE_CLASS( EveFiringEffectElementContainer ) :
	public IEveSpaceObject2,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveFiringEffectElementContainer( IRoot* lockObj = nullptr );

	virtual void UpdateSyncronous( const EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext );
	virtual void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*> & renderables, Tr2ImpostorManager * impostors );
	virtual bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	// This version of the function should perform an update on the model / ball position
	virtual void UpdateModelCenterWorldPosition( Vector3 & position, Be::Time t );

	// This version of the function should not update the object
	virtual void GetModelCenterWorldPosition( Vector3 & position ) const;

	// If possible, return an AABB in local coordinates
	virtual bool GetLocalBoundingBox( Vector3 & min, Vector3 & max );
	// Get the local to world transform
	virtual void GetLocalToWorldTransform( Matrix & transform ) const;

	void StartFiring();
	void StopFiring();

	void SetActive( bool active );
	bool GetActive() const;

	IEveFiringEffectElement* GetElement();
	void SetElement( IEveFiringEffectElement * element );

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