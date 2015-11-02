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
	void StartFireAtLocator( int l );
	void StopFireAtLocator();
	const Vector3* GetTargetPosition() const;
	int FindClosestLocator( const Vector3* source, Vector3* position ) const;

	// updates
	void Update( float deltaT );

	// hit/miss
	void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out );

	// target object queries
	float GetRadius() const;

private:
	// data
	ITriTargetablePtr m_object;
	int m_locator;

	// actual target position and smoothing
	Vector3 m_position;
	Vector3 m_positionOld;
	float m_positionOldInfluence;
};

TYPEDEF_BLUECLASS( EveTurretTarget );

#endif