#pragma once
#ifndef EveTransform_h
#define EveTransform_h


#include "Tr2Transform.h"
#include "Tr2PerObjectData.h"
#include "IEveSpaceObject2.h"
#include "Tr2Renderer.h"
#include "TriRenderBatch.h"
#include "TriObserverLocal.h"
#include "Tr2DebugRenderer.h"
#include "ITr2CurveSetOwner.h"

#include "IEveTransform.h"
#include "IWorldPosition.h"

BLUE_DECLARE( Tr2ParticleSystem );
BLUE_DECLARE_VECTOR( Tr2ParticleSystem );
BLUE_DECLARE( Tr2ParticleSystem );
BLUE_DECLARE_VECTOR( Tr2ParticleSystem );
BLUE_DECLARE_INTERFACE( ITr2Emitter );
BLUE_DECLARE_IVECTOR( ITr2Emitter );
BLUE_DECLARE_INTERFACE( ITr2GenericEmitter );
BLUE_DECLARE_IVECTOR( ITr2GenericEmitter );

BLUE_DECLARE( EveTransform );
BLUE_DECLARE_VECTOR( EveTransform );

BLUE_CLASS( EveTransform ):
	public Tr2Transform,
	public IEveTransform,
	public IEveSpaceObject2,
	public ITr2Pickable,
	public IWorldPosition,
	public IInitialize,
	public ITr2CurveSetOwner,
	public ITr2DebugRenderable
{
public:
    EXPOSE_TO_BLUE();
	using Tr2Transform::Lock;
	using Tr2Transform::Unlock;

    EveTransform( IRoot* lockobj = NULL );

    virtual void UpdateViewDependentData( const TriFrustum& frustum, const Matrix& parentTransform );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2	
	void Update( EveUpdateContext& updateContext ) override;
	void UpdateSyncronous( EveUpdateContext& updateContext ) override;
	void UpdateAsyncronous( EveUpdateContext& updateContext ) override;
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform ) override;
	void GetRenderables( std::vector<ITr2Renderable*>& renderables ) override;
	void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors ) override;
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const override;
	void UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t ) override;
	void GetModelCenterWorldPosition( Vector3 &position ) const override;
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max ) override { return false; }
	void GetLocalToWorldTransform( Matrix &transform ) const override { transform = IdentityMatrix(); }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable - mostly implemented by Tr2Transform except for these
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	/////////////////////////////////////////////////////////////////////////////////////
	// IWorldPosition
	/////////////////////////////////////////////////////////////////////////////////////
	virtual Vector3 GetWorldPosition();
	virtual Quaternion GetWorldRotation();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID( uint16_t ) { return this->GetRawRoot(); }
	virtual void GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
    virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
    virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer );

	// EveTransforms can be under EveTransforms
	PIEveTransformVector m_children;

	// PlacementObservers
	PTriObserverLocalVector m_observers;

	// Generic particle system/emitter lists (Python-exposed)
	PTr2ParticleSystemVector m_particleSystems;
	PITr2GenericEmitterVector m_particleEmitters;
	
	Tr2Lod GetLODLevel() const;

	void PlayCurveSets();
	void PlayCurveSet( const std::string& name, const std::string& rangeName );
	void StopCurveSet( const std::string& name );
	float GetCurveSetDuration( const std::string& name ) const;
	float GetRangeDuration( const std::string& name, const std::string& rangeName ) const;

	void SetDisplay( bool value ) { m_display = value; };

protected:
	bool m_isVisible;
	bool m_useLodLevel;
	bool m_hideOnLowQuality;
	Tr2Lod m_lodLevel;
	
	float m_visibilityThreshold;

	Vector3 m_lastRelativePosition;
	float m_lastDeltaTime;
	float m_lastCurveUpdateDelta;

	Vector3 m_overrideBoundsMin;
	Vector3 m_overrideBoundsMax;

	Tr2MeshBasePtr m_meshLod;  // This thing is deprecated and should be removed once there are no references to it in branch/content
	
private:
	using Tr2Transform::Update; // Silence warning about this hidden function
};

TYPEDEF_BLUECLASS( EveTransform );

class EveBasicPerObjectData : public Tr2PerObjectData
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
	{
		FillAndSetConstants(	*buffers[Tr2RenderContextEnum::VERTEX_SHADER],
											&m_world, sizeof( m_world ) + sizeof( m_worldInverseTranspose ),
											Tr2RenderContextEnum::VERTEX_SHADER,
											Tr2Renderer::GetPerObjectVSStartRegister(),
											renderContext );
	}

	Matrix m_world;
	Matrix m_worldInverseTranspose;
};


#endif //EveTransform_h
