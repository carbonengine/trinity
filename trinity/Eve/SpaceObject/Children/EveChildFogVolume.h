// Copyright © 2024 CCP ehf.

#pragma once

#include "EveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "ITr2Renderable.h"
#include "Tr2DebugRenderer.h"
#include "Eve/Volume/IEveVolume.h"
#include "Tr2VolumetricsRenderer.h"

BLUE_DECLARE_INTERFACE( IEveVolume );
BLUE_DECLARE_IVECTOR( IEveVolume );


BLUE_CLASS( EveChildFogVolume ) :
	public EveSpaceObjectChild,
	public EveChildTransform,
	public IInitialize,
	public ITr2DebugRenderable,
	public ITr2FroxelFogSettings,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveChildFogVolume( IRoot* lockobj = nullptr );

	void RebuildBoundingSphere();

	/////////////////////////////////////////////////////////////////////////////////////
	// EveSpaceObjectChild
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query ) const override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void GetLocalToWorldTransform( Matrix & transform ) const override;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) override;
	bool IsAlwaysOn() const override;

	void RegisterComponents() override;
	void UnRegisterComponents() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2FroxelFogSettings
	FroxelFogSettings* GetFroxelFogSettings() override;

private:
	void UpdateTransformFromParent( const EveChildUpdateParams& params );

	PIEveVolumeVector m_volumes;

	float m_intensity;

	FroxelFogSettings m_settings;

	CcpMath::Sphere m_boundingSphere;
};

TYPEDEF_BLUECLASS( EveChildFogVolume );
