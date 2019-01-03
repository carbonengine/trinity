////////////////////////////////////////////////////////////
//
//    Created:   December 2018
//    Copyright: CCP 2018
//

#pragma once
#ifndef EveChildEnvironment_H
#define EveChildEnvironment_H

#include "StdAfx.h"
#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "ITr2Renderable.h"
#include "Tr2DebugRenderer.h"


BLUE_INTERFACE( IEveVolume ) : 
	public IRoot
{
	virtual float GetIntensity( Vector3 cameraPosition ) = 0;
	virtual void RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& parentTransform ) = 0;
	virtual Vector4 GetBoundingSphere() const = 0;
	virtual void RegisterForChanges( std::function<void()> NotifyParent );
};
BLUE_DECLARE_IVECTOR( IEveVolume );


BLUE_CLASS( EveSphereVolume ) : 
	public IEveVolume,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveSphereVolume( IRoot* lockobj = NULL );
	~EveSphereVolume();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveVolume
	void RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& parentTransform ) override;
	float GetIntensity( Vector3 cameraPosition ) override;
	Vector4 GetBoundingSphere() const override;
	void RegisterForChanges( std::function<void()> NotifyParent ) override;

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

private:
	BlueSharedString m_name;

	Vector3 m_position;
	Vector3 m_centerOffset;
	float m_radius;
	float m_innerRadius;
	
	std::function<void()> m_notifyParentFunc;
	bool m_notifyParent;

};

BLUE_CLASS( EveBoxVolume ) : 
	public IEveVolume,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveBoxVolume( IRoot* lockobj = NULL );
	~EveBoxVolume();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveVolume
	void RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& parentTransform ) override;
	float GetIntensity( Vector3 cameraPosition ) override;
	Vector4 GetBoundingSphere() const override;
	void RegisterForChanges( std::function<void()> NotifyParent ) override;

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

private:

	BlueSharedString m_name;

	Vector3 m_position;
	Vector3 m_centerOffset;
	Vector3 m_scaling;
	Vector3 m_innerScaling;
	Quaternion m_rotation;
	
	Matrix m_boxTransform;
	Matrix m_centerTransform;
	Matrix m_inverseBoxTransform;
	Matrix m_inverseCenterTransform;

	std::function<void()> m_notifyParentFunc;
	bool m_notifyParent;
};


BLUE_CLASS( EveChildEnvironment ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2Renderable,
	public IInitialize,
	public ITr2DebugRenderable,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	EveChildEnvironment( IRoot* lockobj = NULL );
	~EveChildEnvironment();

	void RebuildBoundingSphere();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const;
	void SetName( const char* name );
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void ChangeLOD( Tr2Lod lod ) {};
	void GetLights( Tr2LightManager& lightManager ) const {};
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) override;
	bool IsAlwaysOn() const;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) {} ;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	bool HasTransparentBatches() override;
	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData ) override;
	void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData );
	float GetSortValue() override;
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	void RenderDebugInfo( Tr2DebugRenderer& renderer ) override;

private:
	void UpdateTransformFromParent( const EveChildUpdateParams& params );
	void SetAsDirty();

	
	BlueSharedString m_name;
	PIEveVolumeVector m_volumes;
	PIEveVolumeVector m_exclusionVolumes;

	Vector4 m_boundingSphere;
	bool m_isDirty;

	float m_environmentIntensity;
};

TYPEDEF_BLUECLASS( EveSphereVolume );
TYPEDEF_BLUECLASS( EveBoxVolume );
TYPEDEF_BLUECLASS( EveChildEnvironment );

#endif