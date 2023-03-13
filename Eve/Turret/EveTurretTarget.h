////////////////////////////////////////////////////////////
//
//    Created:   November 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef EveTurretTarget_H
#define EveTurretTarget_H

#include "Include/ITriTargetable.h"

namespace ImpactBehaviour {
	
	enum Type {
		DAMAGE_LOCATOR,
		SHIELD_ELLIPSOID,
		CENTER
	};

}

// forwards
BLUE_DECLARE_INTERFACE( IWorldPosition );

BLUE_CLASS( EveTurretTarget ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveTurretTarget( IRoot* lockobj = NULL );
	~EveTurretTarget();

	// access target object
	ITriTargetablePtr GetTargetable() const;
	bool SetTargetable( IRoot* object );

	// access locator
	int GetLocator() const;
	void StartFireAtLocator( int l, float delay, float length, const Vector3* source );
	void StopFireAtLocator();
	const Vector3* GetTrackingPosition() const;
	const Vector3* GetTargetPosition() const;
	int FindClosestLocator( const Vector3* source, Vector3* position ) const;
	int FindRandomValidLocator( const Vector3& source, Vector3& position ) const;

	// updates
	void Update( float deltaT, const Vector3* source );

	// hit/miss
	void SetBehaviour( bool laserMiss, bool projectileMiss, float impactSize, ImpactBehaviour::Type impactBehaviour );
	bool GetShotMissed() const;
	void SetShotMissed( bool missed );
	double GetLastShotTime() const;
	bool PopShotMissed();
	size_t MissQueueSize() const;

	// target object queries
	float GetRadius() const;
	ITriTargetable::ImpactConfiguration GetImpactConfiguration() const;
	bool ShowDestObject() const;

private:
	void GetImpactPosition( Vector3& out, const Vector3* source );

	// data
	ITriTargetablePtr m_object;
	IWorldPositionPtr m_objectPos;
	int m_locator;

	// impacts
	float m_impactDelay;
	float m_impactLength;
	float m_impactSize;
	int m_impactID;
	ImpactBehaviour::Type m_impactBehaviour;

	// actual target position and smoothing
	Vector3 m_trackingPosition;
	Vector3 m_targetPosition;
	Vector3 m_positionOld;
	float m_positionOldInfluence;

	// hit/miss related data
	Vector3 m_positionMiss;
	TrackableStdDeque<bool> m_missQueue;
	bool m_lastShotIsMiss;
	double m_lastShotTime;
	bool m_laserMissBehaviour, m_projectileMissBehaviour;
	bool m_readyToFireEffect;
	float m_randomMissDistanceOffset;
	Vector3 m_randomMissPositionOffset;
};

TYPEDEF_BLUECLASS( EveTurretTarget );

#endif