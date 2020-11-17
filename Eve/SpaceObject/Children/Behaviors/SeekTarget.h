#pragma once
#ifndef SeekTarget_H
#define SeekTarget_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"
#include "PlayFX.h"
#include "Utilities/BoundingBox.h"
#include "Eve/SpaceObject/Utils/EveLocatorSets.h"

BLUE_DECLARE( EveLocatorSets );

struct SeekTargetData
{
	SeekTargetData() :
		bucketId( -1 ),
		locatorIndex( -1 ),
		timePassed( 0.f ),
		position( 0, 0, 0 ),
		direction( 0, 0, 0 ),
		arrived( true )
	{
	}

	int bucketId;
	int locatorIndex;
	float timePassed;
	Vector3 position;
	Vector3 direction;
	bool arrived;
};

struct LocatorData
{
	Vector3 position;
	Quaternion direction;
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
	virtual void InitializeScratch( void* scratchMemory );
	virtual std::vector<Vector3> CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
		BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius );
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );
	virtual int GetProcessPriority();
	
	void SetTarget( EveSpaceObject2* target );
	void SetExit( bool value );
	void SetBehaviorWeight( float value );
	void SetupShipRepair();
	void ResetBehavior();
	void SplitBoundingBox();
	void SetTotalRepairTime( float seconds );

private:
	void SortLocators();

	bool m_exit;
	bool m_droneArrived;
	bool m_sortedLocators;
	bool m_repair;
	bool m_startTimer;
	bool m_doneRepairing;
	int m_counter;
	int m_priority;
	float m_totalRepairTime;
	float m_repairTimePassed;
	float m_behaviorWeight;
	float m_arrivedRadius;
	float m_slowDownRadius;
	float m_seconds;
	Vector3 m_arrivalPoint; // debug
	EveSpaceObject2* m_target;

	IBehavior* m_fxBehavior;

	std::vector<Vector3> m_todo;

	BlueScriptCallback m_onFirstDroneArrivedCallback;

	std::vector<AxisAlignedBoundingBox> m_boundingBoxes;
	std::vector<std::vector<LocatorData>> m_locatorBuckets;
	std::vector<std::vector<int>> m_locatorBucketIndices;
	BlueSharedString m_locatorSetName;

	/////////////////////////////////////////////////////////////////////////////////////
	// locator sets
	const LocatorStructureList* GetLocatorsForSet( const BlueSharedString& setName ) const;
	EveLocatorSetsPtr m_locatorSet;
	void AddLocatorSet();
};

TYPEDEF_BLUECLASS( SeekTarget );

#endif