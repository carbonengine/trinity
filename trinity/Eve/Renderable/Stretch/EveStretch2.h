// Copyright © 2017 CCP ehf.

#pragma once

#include "Eve/IEveFiringEffectElement.h"
#include "Lights/ITr2LightOwner.h"
#include "ITr2Renderable.h"
#include "Tr2DeviceResource.h"
#include "Tr2DebugRenderer.h"
#include "Tr2ProceduralResources.h"


BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE( Tr2PointLight );
BLUE_DECLARE( Tr2GpuSharedEmitter );
BLUE_DECLARE( TriObserverLocal );


// --------------------------------------------------------------------------------------
// Description:
//   EveStretch2 is a simplified version of EveStretch. Renders an effect between two
//   points as a set of quads.
// --------------------------------------------------------------------------------------
BLUE_CLASS( EveStretch2 ) :
	public IEveFiringEffectElement,
	public ITr2Renderable,
	public Tr2DeviceResource,
	public IInitialize,
	public INotify,
	public ITr2DebugRenderable,
	public ITr2LightOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveStretch2( IRoot* lockObj = nullptr );

	virtual bool Initialize();
	virtual bool OnModified( Be::Var * value );

	virtual void SetDestObjectScale( float scale );
	virtual void StartMoving();
	virtual float GetCurveDuration();
	virtual void StartFiring( float delay );
	virtual void StopFiring();

	virtual void SetDisplay( bool display );
	virtual void SetFiringTransform( const Matrix& source, const Vector3& dest );
	virtual void SetFiringTransform( const Vector3& source, const Vector3& dest );
	virtual void DisplayEndPoints( bool displaySource, bool displayDest );
	virtual void SetIntensity( float intensity );

	virtual void Update( const EveUpdateContext& updateContext ) override;
	virtual void UpdateEffectAsync( const EveUpdateContext& updateContext ) override;
	virtual void UpdateEffectSync( const EveUpdateContext& updateContext ) override;

	virtual void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*> & renderables );

	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator );
	virtual void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	virtual bool HasTransparentBatches();
	virtual float GetSortValue();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2LightOwner
	virtual void GetLights( Tr2LightManager & lightManager ) const override;

	/////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	virtual void RegisterComponents() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;

protected:
	virtual void ReleaseResources( TriStorage s );
	virtual bool OnPrepareResources();

private:
	void GetEndPointTransforms( Matrix & source, Matrix & destination ) const;
	Matrix GetDestinationTransform() const;

	Tr2EffectPtr m_effect;
	TriCurveSetPtr m_start;
	TriCurveSetPtr m_loop;
	TriCurveSetPtr m_end;

	Tr2PointLightPtr m_sourceLight;
	Tr2PointLightPtr m_destinationLight;
	Tr2GpuSharedEmitterPtr m_sourceEmitter;
	Tr2GpuSharedEmitterPtr m_destinationEmitter;

	TriObserverLocalPtr m_sourceObserver;
	TriObserverLocalPtr m_destinationObserver;

	Be::Time m_startTime;

	std::string m_name;

	Vector3 m_source;
	float m_currentDestinationScale;
	Vector3 m_destination;
	float m_destinationScale;
	Vector4 m_effectData[2];

	uint32_t m_quadCount;
	Tr2ProceduralBuffer m_vb;
	unsigned int m_vertexDeclHandle;
	float m_intensity;

	float m_boundingRadius;

	Matrix m_sourceTransform;
	Matrix m_destinationTransform;

	bool m_visible;
	mutable bool m_isInFrustum;
};

TYPEDEF_BLUECLASS( EveStretch2 );
