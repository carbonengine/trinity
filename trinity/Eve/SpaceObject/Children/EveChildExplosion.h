// Copyright © 2016 CCP ehf.

#pragma once
#ifndef EveChildExplosion_H
#define EveChildExplosion_H

#include "EveChildContainer.h"

BLUE_DECLARE( Tr2SphereShapeAttributeGenerator );
BLUE_DECLARE_VECTOR( Tr2SphereShapeAttributeGenerator );

// --------------------------------------------------------------------------------------
// Description:
//   EveChildExplosion is a specialized EveSpaceObject2 child class that creates an
//   explosion animation. The explosion contains multiple "local" explosions and a single
//   "global" explosion.
// --------------------------------------------------------------------------------------
BLUE_CLASS( EveChildExplosion ) :
	public EveChildContainer
{
public:
	EXPOSE_TO_BLUE();

	EveChildExplosion( IRoot* lockobj = nullptr );
	~EveChildExplosion();

	void Play();
	void Stop();
	void SetLocalExplosionTransforms( const std::vector<Matrix>& transforms );
	void SetGlobalExplosionOffset( const Vector3& offset );

	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer );


	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

private:
	struct Transform
	{
		Quaternion rotation;
		Vector3 position;
	};

	void CalculateExplosionTimes( uint32_t localExplosionCount );
	void FindSharedObjects();
	void SpawnLocalExplosion( const Matrix& transform );
	static ICopier::OverrideResult CopyElement( IRoot * source, IRoot * *dest, ICopier * copier, void* context );
	static void UpdateEmitter( IRoot * source, IRoot * *dest, ICopier * copier, void* context );

	// Delay from explosion start to the first "local" explosion in seconds
	float m_localExplosionDelay;
	// Maximum interval between local explosions in seconds
	float m_localExplosionInterval;
	// Coefficent to apply to m_localExplosionInterval for consecutive explosions
	float m_localExplosionIntervalFactor;
	// Store the localExplosion times in seconds so we can safely calculate the global explosion time
	std::vector<float> m_localExplosionTimes;
	// Delay from explosion start to the "global" explosion in seconds
	float m_globalExplosionDelay;
	float m_countdownToGlobalExplosionStart;
	// Time from the start of the explosion to the point when original model needs to be switched to the wreck
	float m_wreckSwitchTime;
	float m_wreckSwitchOffsetFromGlobalStart;
	// Total duration of a single local explosion
	float m_localDuration;
	// Duration of the global explosion
	float m_globalDuration;
	float m_totalDuration;



	// Child containing local explosion effect
	IEveSpaceObjectChildPtr m_localExplosion;
	PIEveSpaceObjectChildVector m_localExplosions;
	// Child containing shared parts of the local explosion effect (particle systems etc.)
	IEveSpaceObjectChildPtr m_localExplosionShared;
	// Child containing global explosion effect
	IEveSpaceObjectChildPtr m_globalExplosion;
	PIEveSpaceObjectChildVector m_globalExplosions;
	PIEveSpaceObjectChildVector m_globalExplosionInstances;
	EveChildContainerPtr m_globalExplosionContainer;

	// Transforms for local explosions
	TrackableStdVector<Matrix> m_localExplosionTransforms;
	// Cache of all blue objects in m_localExplosionShared
	TrackableStdHashSet<IRoot*> m_sharedObjects;

	// Time since the start of playback
	float m_playTime;
	// Time to the next local explosion
	float m_nextLocalExplosionTime;
	// Time to the start of the global explosion
	float m_globalExplosionTime;
	// Index into m_localExplosionTransforms for the next local explosion
	size_t m_nextLocalExplosion;

	// Global explosion scaling
	Vector3 m_globalExplosionScaling;
	Vector3 m_globalExplosionOffset;

	// Local explosion scaling
	Vector3 m_localExplosionScaling;

	// Is the effect playing
	bool m_isPlaying;
};

TYPEDEF_BLUECLASS( EveChildExplosion );

#endif