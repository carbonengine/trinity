////////////////////////////////////////////////////////////
//
//    Created:   February 2012
//    Copyright: CCP 2012
//
#pragma once
#ifndef EveMissileWarhead_H
#define EveMissileWarhead_H

#include "../../Tr2PersistentPerObjectData.h"
#include "Eve/EveTransform.h"
#include "include/ITriTargetable.h"


BLUE_DECLARE( EveSpriteSet );

// --------------------------------------------------------------------------------
// Description:
//   This class holds an Eve missile warhead. Multiple warheads can be hold by an
//   EveMissile. It derives from an EveTransform.
// SeeAlso:
//   EveMissile, EveTransform
// --------------------------------------------------------------------------------
class EveMissileWarhead :
	public EveTransform
{
public:
	EXPOSE_TO_BLUE();

	EveMissileWarhead( IRoot* lockobj = NULL );
	~EveMissileWarhead();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void Update( EveUpdateContext& updateContext );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable - overriding EveTransform implementations
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );

	void RenderDebugInfoFromParent( ITr2DebugRenderer2& renderer, Matrix transform );


	// access to relative data
	const Matrix& GetCurrentOffsetTransform() const;

	// local bounding sphere
	bool GetLocalBoundingSphere( Vector4& sphere ) const;

	// calc relative positions of this warhead
	void UpdateWarhead( float deltaT, float estimatedTotalAliveTime, const Vector3* currentBallVelocity, const Vector3* currentInheritedVelocity, const Matrix* invBallRotation, const Matrix& missileTransform, const Vector3& egoTranslation );
	// update warhead's final position/orientation
	void UpdateEndTransform( const Matrix& endTransform, bool switchLocators );

	// set warhead's inital launchdelay
	void PrepareLaunch();

	// set warhead's inital position/orientation (aka Launch)
	void Launch( const Matrix& startTransform );

	// locator access
	void SetTargetLocator( int damageLocator ) { m_targetLocator = damageLocator; }
	int GetTargetLocator() const { return m_targetLocator; }

	// enable particle emitting on this warhead
	void EnableParticleEmitting( bool enable );

	// warhead states
	enum State
	{
		STATE_DELAYED = 0,
		STATE_LAUNCH,
		STATE_EJECTING,
		STATE_START_TRACKING,
		STATE_TRACKING_SPREAD,
		STATE_TRACKING_FINAL,
		STATE_EXPLODED,
		STATE_DEAD
	};
	// warhead state change return values
	enum StateChangeEvent
	{
		EVT_SWITCH_TARGET = 0,
		EVT_EXPLODE,
		EVT_NONE
	};
	// state handling
	StateChangeEvent UpdateState( float deltaT, float estimatedAliveTime, ITriTargetable* target );
	StateChangeEvent CheckImpact( float deltaT, float estimatedTotalAliveTime, ITriTargetable* target );
	State GetState() const { return m_state; }

	// this warhead's unique ID
	int GetWarheadID() const { return m_id; }

	uint32_t GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const;
	void UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data );

protected:
	// per-object data
	Tr2PersistentPerObjectData<EveMissileWarhead> m_perObjectDataVs;
	Tr2PersistentPerObjectData<EveMissileWarhead> m_perObjectDataPs;

private:
	EveSpriteSetPtr m_spriteSet;

	float m_warheadLength;
	float m_warheadRadius;

	// timing
	float m_flyingTime;

	// impact effect time
	float m_impactDuration;
	float m_impactSize;

	// state of this warhead
	State m_state;

	// dynamic start data
	Vector3 m_currentStartOffset;
	Quaternion m_startOrientation;
	// dynamic start data valid?
	bool m_startDataValid;
	bool m_doSpread;

	// 
	Vector3 m_movement;
	Vector3 m_posLastFrame;

	// dynamic end data
	Vector3 m_endOffset;
	Vector3 m_oldEndOffset;
	Vector3 m_currentEndOffset;

	// dynamic current data
	Vector3 m_currentOffset;
	Quaternion m_currentOrientation;
	float m_currentEjectVelocity;
	float m_currentDurationEjectPhase;
	// resulting transform
	Matrix m_currentOffsetTransform;

	// final destination data
	float m_finalDestinationTimer;
	float m_finalTargetTime;

	// animatable data to make the path and eject of this warhead more unique/interesting
	Vector3 m_pathOffset;
	float m_pathOffsetNoiseScale;
	float m_pathOffsetNoiseSpeed;
	float m_durationEjectPhase;
	float m_startEjectVelocity;
	float m_acceleration;

	// what we are aiming for!
	int m_targetLocator;

	// each warhead has a unique ID, set on the python side
	int m_id;

	// random speed modifier
	float m_speedModifier;

	// explosion data: where to put the BOOM
	float m_explosionDistance;
	float m_maxExplosionDistance;
	Vector3 m_explosionPosition;

	//bombs
	bool m_bombFlightpath;
	bool m_lastPositionValid;
};

TYPEDEF_BLUECLASS( EveMissileWarhead );

// --------------------------------------------------------------------------------
// Description:
//   This class holds the per-object data of an Eve missile warhead.
// --------------------------------------------------------------------------------
class EveMissileWarheadPerObjectData : public Tr2PerObjectDataWithPersistentBuffers<EveMissileWarhead>
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;
};

#endif // EveMissileWarhead_H