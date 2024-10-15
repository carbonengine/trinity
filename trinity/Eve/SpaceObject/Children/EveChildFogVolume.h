////////////////////////////////////////////////////////////
//
//    Created:   December 2024
//    Copyright: CCP 2024
//

#pragma once

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "ITr2Renderable.h"
#include "Tr2DebugRenderer.h"
#include "Eve/Volume/IEveVolume.h"
#include "Tr2VolumetricsRenderer.h"

BLUE_DECLARE_INTERFACE( IEveVolume );
BLUE_DECLARE_IVECTOR( IEveVolume );


BLUE_CLASS( EveChildFogVolume ) :
	public IEveSpaceObjectChild,
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
	// IEveSpaceObjectChild
	const char* GetName() const override;
	void SetName( const char* name ) override;
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) override;
	void GetRenderables( std::vector<ITr2Renderable*> & renderables ) override {};
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query ) const override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void GetLocalToWorldTransform( Matrix & transform ) const override;
	void ChangeLOD( Tr2Lod lod ) override {};
	void GetLights( Tr2LightManager & lightManager ) const override {};
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) override;
	bool IsAlwaysOn() const override;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override {};

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
	FroxelFogWeightedSettings GetFroxelFogSettings() override;

private:
	void UpdateTransformFromParent( const EveChildUpdateParams& params );

	BlueSharedString m_name;
	PIEveVolumeVector m_volumes;

	float m_intensity;

	FroxelFogWeightedSettings m_settings;

	CcpMath::Sphere m_boundingSphere;
};

TYPEDEF_BLUECLASS( EveChildFogVolume );
