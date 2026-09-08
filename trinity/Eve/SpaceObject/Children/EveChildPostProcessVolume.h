// Copyright © 2018 CCP ehf.

#pragma once

#include "StdAfx.h"
#include "EveSpaceObjectChild.h"
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
	public EveSpaceObjectChild,
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
	// EveSpaceObjectChild
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void GetLocalToWorldTransform( Matrix & transform ) const;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) override;
	bool IsAlwaysOn() const;

	virtual void RegisterComponents();
	virtual void UnRegisterComponents();

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2PostProcessOwner
	Tr2PostProcessAttributes* GetPostProcessAttributes() override;

private:
	void UpdateTransformFromParent( const EveChildUpdateParams& params );

	PIEveVolumeVector m_volumes;
	PIEveVolumeVector m_exclusionVolumes;

	CcpMath::Sphere m_boundingSphere;

	// post process attributes
	Tr2PostProcessAttributesPtr m_postProcessAttributes;
};

TYPEDEF_BLUECLASS( EveChildPostProcessVolume );
