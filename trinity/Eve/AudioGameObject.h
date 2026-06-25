// Copyright © 2025 CCP ehf.

#pragma once

#ifndef AudioGameObject_h
#define AudioGameObject_h

#include "IWorldPosition.h"
#include "EveTransform.h"
#include "IEveSpaceObject2.h"

#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2
// This is only needed for py2 as the file now belongs in blue.
// Unfortunatly the blue py2 branch cannot be updated at present due to security vulnerability work.
// The file version in the older blue versions had diverged from this one is incompatible.
#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif

#include <ITriFunction.h>
#include <ITr2AudEmitter.h>

BLUE_DECLARE_INTERFACE( ITr2AudEmitter );
BLUE_DECLARE_IVECTOR( ITr2AudEmitter );

BLUE_DECLARE( Tr2ExternalParameter );
BLUE_DECLARE_VECTOR( Tr2ExternalParameter );

BLUE_DECLARE( AudioGameObject );

/**
 * @brief AudioGameObject is an object that provides the ability to freely place an audio emitter in a scene without being attached to assets.
 *
 */

BLUE_CLASS( AudioGameObject ) :
	public IWorldPosition,
	public IEveSpaceObject2,
	public IInitialize,
	public ITr2DebugRenderable,
	public INotify

{
public:
	EXPOSE_TO_BLUE();

	AudioGameObject( IRoot* lockobj = NULL );

	bool Initialize();

	void py__init__();

	virtual bool OnModified( Be::Var * val );

	// IEveSpaceObject2
	void UpdateSyncronous( const EveUpdateContext& updateContext );
	void UpdateAsyncronous( const EveUpdateContext& updateContext );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform );
	void GetRenderables( std::vector<ITr2Renderable*> & renderables, Tr2ImpostorManager * impostors );
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void GetPerObjectStructs( EveSpaceObjectVSData & vsData, EveSpaceObjectPSData & psData ) const;
	void UpdateModelCenterWorldPosition( Vector3 & position, Be::Time t );
	void GetModelCenterWorldPosition( Vector3 & position ) const;
	bool GetLocalBoundingBox( Vector3 & min, Vector3 & max );
	void GetLocalToWorldTransform( Matrix & transform ) const;

	// IWorldPosition
	virtual Vector3 GetWorldPosition();
	virtual Quaternion GetWorldRotation();

	// ITr2DebugRenderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions & options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer );

	/**
	 * @brief Gets the audio emitter for this object.
	 * @return Smart pointer to the emitter, or null if not initialized
	 */
	ITr2AudEmitterPtr GetAudioEmitter() const
	{
		return m_audioEmitter;
	}

	/**
	 * @brief Sets the name of this object's audio emitter.
	 * @param name Name for the emitter
	 */
	void SetEmitterName( const std::string& name );

	/**
	 * @brief Plays an audio event on this object's emitter.
	 * @param eventName Wwise event name to trigger
	 * @return Playing ID from Wwise, or 0 if failed
	 */
	unsigned int PlayAudioEvent( const std::wstring& eventName );

	Quaternion m_rotation;
	Vector3 m_translation;

private:
	void UpdateWorldTransform( Be::Time time );

protected:
	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;
	bool m_display;

	Matrix m_worldTransform;

	ITr2AudEmitterPtr m_audioEmitter;
	bool m_mute;

	std::string m_name;

	PTr2ExternalParameterVector m_externalParameters;
};

TYPEDEF_BLUECLASS( AudioGameObject );

#endif
