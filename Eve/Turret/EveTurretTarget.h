////////////////////////////////////////////////////////////
//
//    Created:   November 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef EveTurretTarget_H
#define EveTurretTarget_H

// forwards
BLUE_DECLARE_INTERFACE( ITriTargetable );

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
	void SetBehaviour( bool laserMiss, bool projectileMiss, float impactSize );
	bool GetShotMissed() const;
	void SetShotMissed( bool missed );
	double GetLastShotTime() const;
	bool PopShotMissed();
	size_t MissQueueSize() const;

	// target object queries
	float GetRadius() const;
	bool ShowDestObject() const;

private:
	// data
	ITriTargetablePtr m_object;
	int m_locator;

	// impacts
	float m_impactDelay;
	float m_impactLength;
	float m_impactSize;
	int m_impactID;

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