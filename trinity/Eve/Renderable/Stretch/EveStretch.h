// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EveStretch_h
#define EveStretch_h


#include <IStretchAudio.h>
#include <ITr2Audio.h>

#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/EveTransform.h"
#include "Curves/TriCurveSet.h"
#include "Eve/IEveFiringEffectElement.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE( EveStretch );
BLUE_DECLARE( TriFloat );

class EveStretch : public IEveFiringEffectElement,
				   public INotify,
				   public IEveTransform,
				   public IEveSpaceObject2,
				   public ITr2DebugRenderable,
				   public ITr2LightOwner,
				   public EveEntity
{
public:
	EXPOSE_TO_BLUE();
	EveStretch( IRoot* lockobj = NULL );

	void SetSourcePosition( Vector3 val );
	void SetDestinationPosition( Vector3 val );

	void SetSourceTransform( const Matrix& val );
	void SetDestinationTransform( const Matrix& val );

	// stretch effect can go "both ways"
	void SetIsNegZForward( bool val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	virtual void UpdateModelCenterWorldPosition( Vector3& position, Be::Time t ) {};
	virtual void GetModelCenterWorldPosition( Vector3& position ) const {};
	virtual bool GetLocalBoundingBox( Vector3& min, Vector3& max )
	{
		return false;
	}
	virtual void GetLocalToWorldTransform( Matrix& transform ) const
	{
		transform = IdentityMatrix();
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2LightOwner
	virtual void GetLights( Tr2LightManager& lightManager ) const override;

	//////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* value ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveTranfrom
	virtual void Update( const EveUpdateContext& updateContext );
	virtual void UpdateViewDependentData( const TriFrustum& frustum, const Matrix& parentTransform ) {};
	virtual Tr2Lod GetLODLevel() const
	{
		return m_lodLevel;
	}

	unsigned int GetCurveSetCount() const
	{
		return (unsigned int)m_curveSets.size();
	}
	TriCurveSetPtr GetCurveSet( unsigned int n )
	{
		return m_curveSets[n];
	}

	// toggle display of source and dest objects of the stretcher
	void SetDisplayDestObject( bool display )
	{
		m_displayDestObject = display;
	}
	void SetDisplaySourceObject( bool display )
	{
		m_displaySourceObject = display;
	}

	void UpdateCurves( const EveUpdateContext& updateContext );
	void Start();

	virtual void SetDisplay( bool display );

	virtual void SetDestObjectScale( float scale )
	{
		m_destObjectScale = scale;
	};
	virtual void StartMoving();
	virtual float GetCurveDuration();
	virtual void StartFiring( float delay );
	virtual void StopFiring();
	virtual void UpdateEffectAsync( const EveUpdateContext& updateContext ) override;
	virtual void UpdateEffectSync( const EveUpdateContext& updateContext ) override;

	virtual void SetFiringTransform( const Matrix& source, const Vector3& dest );
	virtual void SetFiringTransform( const Vector3& source, const Vector3& dest );
	virtual void DisplayEndPoints( bool displaySource, bool displayDest );

	void SetSourceObjectScale( float scale )
	{
		m_sourceObjectScale = scale;
	};

	// debug
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer );

private:
	Tr2Lod m_lodLevel;
	Be::Time m_lastCurveUpdateTime;

	std::string m_name;

	bool m_display;
	bool m_update;
	bool m_displaySourceObject;
	bool m_displayDestObject;
	bool m_useCurveLod;

	float m_sourceObjectScale;
	float m_destObjectScale;

	// Set to true when source transform is set to point towards the destination.
	// This is used for turret firing effects - the turrets already take care of aiming
	// and the stretch only needs to apply the scaling. This is more efficient than
	// calculating the aiming twice, and allows to use one stretch for multi-barrel
	// guns.
	bool m_useTransformsForStretch;

	// what is the direction of the stretch? +z or -z?
	bool m_isNegZForward;

	Matrix m_sourceTransform;
	Matrix m_destinationTransform;

	Vector3 m_sourcePosition;
	Vector3 m_destinationPosition;

	ITriVectorFunctionPtr m_source;
	ITriVectorFunctionPtr m_dest;

	EveTransformPtr m_sourceObject;
	EveTransformPtr m_destObject;
	EveTransformPtr m_stretchObject;
	EveTransformPtr m_moveObject;

	PTr2LightVector m_sourceLights;
	PTr2LightVector m_destLights;

	ITriScalarFunctionPtr m_progressCurve;
	PTriCurveSetVector m_curveSets;
	TriCurveSetPtr m_moveCompletion;
	bool m_moveCompleted;
	bool m_moving;
	Be::Time m_startTime;

	// This can't be stored directly here as we must allow
	// value bindings to the length.
	TriFloatPtr m_length;

	ITr2AudioPtr m_audio;
	IStretchAudioPtr m_stretchAudio;
};

TYPEDEF_BLUECLASS( EveStretch );
BLUE_DECLARE_VECTOR( EveStretch );
#endif //EveStretch_h