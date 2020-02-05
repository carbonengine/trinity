#pragma once
#ifndef SeekTarget_H
#define SeekTarget_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"
#include "PlayFX.h"

struct LocatorData
{
	LocatorData() :
		index( -1 ),
		timePassed( 0.f ),
		position( 0, 0, 0 ),
		direction( 0, 0, 0 )
	{
	}

	int index;
	float timePassed;
	Vector3 position;
	Vector3 direction;
};

BLUE_CLASS( SeekTarget ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	SeekTarget( IRoot* lockobj = nullptr );
	~SeekTarget();

	/////////////////////////////////////////////////////////////////////////////////////
	// SeekTarget
	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( const DroneAgent& drone, void* scratchMemory );
	virtual std::vector<Vector3> CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
		BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );
	
	void SetTarget( ITriTargetable* target );
	void SetExit( bool value );

private:
	bool m_exit;
	float m_behaviorWeight;
	float m_arrivedRadius;
	float m_slowDownRadius;
	float m_distanceFromShip;
	float m_seconds;
	Vector3 m_arrivalPoint; // debug
	ITriTargetable* m_target;
	std::vector<int> m_takenLocatorIndices;

	IBehavior* m_tunnelBehavior;
	IBehavior* m_fxBehavior;

	std::vector<Vector3> m_todo;
};

TYPEDEF_BLUECLASS( SeekTarget );

#endif