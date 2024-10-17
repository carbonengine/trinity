////////////////////////////////////////////////////////////
//
//    Created:   October 2024
//    Copyright: CCP 2024
//

#pragma once

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "Eve/EveEntity.h"
#include "PostProcess/Tr2PostProcessEnums.h"
#include "Eve/Volume/IEveVolume.h"


BLUE_INTERFACE( IEveLightingOverride ) :
	public IRoot
{
	struct Overrides
	{
		Color sunColor = Color( 0, 0, 0, 0 );
		float sunIntensity = 0;
		float backgroundIntensity = 0;
		float reflectionIntensity = 0;

		Overrides operator*( float rhs ) const;
		Overrides operator+( const Overrides& rhs ) const;
	};

	struct OverrideInfo
	{
		PostProcessEnums::Priority priority = PostProcessEnums::MEDIUM_PRIORITY;
		float intensity = 1;
		Overrides value;
	};
	virtual OverrideInfo GetOverrides() const = 0;
};

REGISTER_COMPONENT_TYPE( "EveLightingOverride", IEveLightingOverride );



BLUE_CLASS( EveChildLightingOverride ) :
	public IEveSpaceObjectChild,
	public IEveLightingOverride,
	public IInitialize,
	public ITr2DebugRenderable,
	public EveChildTransform,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveChildLightingOverride( IRoot* lockobj = nullptr );

	OverrideInfo GetOverrides() const override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const override;
	void SetName( const char* name ) override;
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) override{};
	void GetRenderables( std::vector<ITr2Renderable*> & renderables ) override{};
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query ) const override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void GetLocalToWorldTransform( Matrix & transform ) const override;
	void ChangeLOD( Tr2Lod lod ) override{};
	void GetLights( Tr2LightManager & lightManager ) const override{};
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) override;
	bool IsAlwaysOn() const override;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override{};

	void RegisterComponents() override;
	void UnRegisterComponents() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;


private:
	void RebuildBoundingSphere();

	BlueSharedString m_name;

	OverrideInfo m_overrides;
	PIEveVolumeVector m_volumes;
	float m_intensity;
	CcpMath::Sphere m_boundingSphere;
};

TYPEDEF_BLUECLASS( EveChildLightingOverride );
