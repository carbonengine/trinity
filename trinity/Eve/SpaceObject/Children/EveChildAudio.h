// Copyright © 2026 CCP ehf.

#ifndef EVE_CHILD_AUDIO_H
#define EVE_CHILD_AUDIO_H

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"

#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2
// This is only needed for py2 as the file now belongs in blue.
// Unfortunatly the blue py2 branch cannot be updated at present due to security vulnerability work.
// The file version in the older blue versions had diverged from this one is incompatible.
#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif

#include <ITr2AudEmitter.h>

// Forward declarations and smart pointer typedefs
BLUE_DECLARE_INTERFACE( ITr2AudEmitter );
BLUE_DECLARE_IVECTOR( ITr2AudEmitter );
BLUE_DECLARE( EveChildAudio );

/**
 * @class EveChildAudio
 * @brief Audio emitter that can be added as a child to EveChildContainer.
 */
BLUE_CLASS( EveChildAudio ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public IInitialize,
	public INotify,
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	/**
         * @brief Constructs an EveChildAudio instance.
         * @param lockobj Optional lock object for thread safety (default: NULL).
         */
	EveChildAudio( IRoot* lockobj = NULL );

	/**
         * @brief Initializes the audio child after loading from a .red file or construction.
         *
         * Called by the Blue framework after the object is fully constructed or deserialized.
         * Sets up the audio emitter and prepares it for playback.
         *
         * @return true if initialization succeeded, false otherwise.
         */
	bool Initialize();

	/**
         * @brief Python initialization hook.
         */
	void py__init__();

	/**
         * @brief Handles modification notifications from the Blue framework.
         *
         * Called when a property with Be::NOTIFY changes.
         *
         * @param val Pointer to the modified variable.
         * @return true if the modification was successfully.
         */
	bool OnModified( Be::Var * val );

	/**
         * @brief Sets the name of the audio emitter.
         * @param name The emitter name to set.
         */
	void SetEmitterName( const std::string& name );

	// Implementing the IEveSpaceObjectChild interface
	const char* GetName() const;
	void SetName( const char* name );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*> & renderables );
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void GetLocalToWorldTransform( Matrix & transform ) const;
	void ChangeLOD( Tr2Lod lod );
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );

	// ITr2DebugRenderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;

private:
	std::string m_name; ///< The name identifier for this audio child.
	Matrix m_worldTransform; ///< Cached world transformation matrix.
	bool m_mute; ///< Whether audio playback is muted.
	ITr2AudEmitterPtr m_audioEmitter; ///< The audio emitter instance.
};

/**
 * @brief Macro that creates container typedefs for EveChildAudio.
 */
TYPEDEF_BLUECLASS( EveChildAudio );

#endif // EVE_CHILD_AUDIO_H