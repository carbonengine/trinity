#pragma once

#ifndef EveEffectRoot_h
#define EveEffectRoot_h

#include "include/ITriFunction.h"

#include "EveTransform.h"
#include "EveLODHelper.h"
#include "IEveSpaceObject2.h"

BLUE_DECLARE( Tr2PointLight );
BLUE_DECLARE_VECTOR( Tr2PointLight );

BLUE_DECLARE( EveEffectRoot );

class EveEffectRoot:
	public IEveSpaceObject2,
	public IInitialize
{
public:
    EXPOSE_TO_BLUE();
	using IEveSpaceObject2::Lock;
	using IEveSpaceObject2::Unlock;

	EveEffectRoot( IRoot* lockobj = NULL );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	void UpdateSyncronous( EveUpdateContext& updateContext );
	void UpdateAsyncronous( EveUpdateContext& updateContext );
	void RenderDebugInfo( Tr2RenderContext& renderContext );
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	virtual void GetLights( Tr2LightManager& lightManager ) const;

	// This version of the function should perform an update on the model / ball position
	void GetModelCenterWorldPosition( Vector3 &position, Be::Time t );

	// This version of the function should not update the object
	void GetCurrentModelCenterWorldPosition( Vector3 &position );

	// If possible, return an AABB in local coordinates
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );
	// Get the local to world transform
	void GetLocalToWorldTransform( Matrix &transform ) const;

	// Functions for starting and stopping the effect.
	void Start();
	void Stop();
protected:
	void UpdateWorldTransform( Be::Time time );

	std::string m_name;

	Vector3 m_scaling;
	Quaternion m_rotation;
	Vector3 m_translation;

	Be::Time m_startTime;

	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;

	Vector4 m_boundingSphere;
	
	// last known results from updating m_ballPosition and m_ballRotation
	Matrix m_worldTransform;
	Matrix m_lastUpdateMatrix;
	Matrix m_localTransform;

	// PlacementObservers
	PTriObserverLocalVector m_observers;

	IBlueObjectProxyPtr m_highDetail;
	IBlueObjectProxyPtr m_mediumDetail;
	IBlueObjectProxyPtr m_lowDetail;
	
	EveTransformPtr m_effectObject;
	PTr2PointLightVector m_lights;

	bool m_display;
	bool m_dynamicLODSelection;
	bool m_playEffect;

	Tr2Lod m_lodLevel;
	
	float GetBoundingSphereRadius() { return m_boundingSphere.w; }
	float m_estimatedSize;
	float m_effectDuration;

	void AssignLOD();
	void UpdateLOD( Be::Time time );
	IBlueObjectProxyPtr CreateLOD( const EveTransform* tf );
	
	IBlueEventListenerPtr m_loadedEventListener;
};

TYPEDEF_BLUECLASS( EveEffectRoot );

#endif // EveEffectRoot_h