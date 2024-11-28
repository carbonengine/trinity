////////////////////////////////////////////////////////////
//
//    Created:   December 2018
//    Copyright: CCP 2018
//

#pragma once

#include "StdAfx.h"
#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "ITr2Renderable.h"
#include "Tr2DebugRenderer.h"
#include "Eve/Volume/IEveVolume.h"
#include "PostProcess/ITr2PostProcessOwner.h"
#include "PostProcess/Tr2PostProcessAttributes.h"

BLUE_DECLARE_INTERFACE( ITr2PostProcessOwner );
BLUE_DECLARE_INTERFACE( IEveVolume );
BLUE_DECLARE_IVECTOR( IEveVolume );


BLUE_CLASS( EveChildPostProcessVolume ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public IInitialize,
	public ITr2DebugRenderable,
	public ITr2PostProcessOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveChildPostProcessVolume( IRoot* lockobj = NULL );
	~EveChildPostProcessVolume();

	void RebuildBoundingSphere();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const;
	void SetName( const char* name );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*> & renderables ){};
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void ChangeLOD( Tr2Lod lod ) {};
	void GetLights( Tr2LightManager& lightManager ) const {};
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) override;
	bool IsAlwaysOn() const;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) {} ;

	virtual void RegisterComponents();
	virtual void UnRegisterComponents();

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2PostProcessOwner
	Tr2PostProcessAttributes* GetPostProcessAttributes() override;

private:
	void UpdateTransformFromParent( const EveChildUpdateParams& params );

	BlueSharedString m_name;
	PIEveVolumeVector m_volumes;
	PIEveVolumeVector m_exclusionVolumes; 

	CcpMath::Sphere m_boundingSphere;

	// post process attributes
	Tr2PostProcessAttributesPtr m_postProcessAttributes;
};

TYPEDEF_BLUECLASS( EveChildPostProcessVolume );
