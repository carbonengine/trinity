// Copyright © 2023 CCP ehf.

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
		arrived( true ),
		hasSpawned( false )
	{
	}

	int bucketId;
	int locatorIndex;
	float timePassed;
	Vector3 position;
	Vector3 direction;
	bool arrived;
	bool hasSpawned;
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
	virtual std::vector<Vector3> CalculateBehavior( std::vector<DroneAgent> & agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius );
	void GetDebugOptions( Tr2DebugRendererOptions & options );
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, std::vector<DroneAgent> & agents, Matrix & parentWorldLocation );
	virtual int GetProcessPriority();

	void SetTarget( EveSpaceObject2 * target );
	void SetExit( bool value );
	void SetBehaviorWeight( float value );
	void SetupShipRepair();
	void ResetBehavior();
	void SplitBoundingBox();
	void SetTotalRepairTime( float seconds );

private:
	void SortLocators();

	bool m_enabled;
	bool m_exit;
	bool m_droneArrived;
	bool m_sortedLocators;
	bool m_repair;
	bool m_startTimer;
	bool m_doneRepairing;
	int m_counter;
	int32_t m_priority;
	float m_totalRepairTime;
	float m_repairTimePassed;
	float m_behaviorWeight;
	float m_arrivedRadius;
	float m_slowDownRadius;
	float m_distFromOrigin;
	float m_seconds;
	std::vector<Vector3> m_arrivalPoints; // debug
	EveSpaceObject2* m_target;

	BlueScriptCallback m_onFirstDroneArrivedCallback;

	std::vector<AxisAlignedBoundingBox> m_boundingBoxes;
	std::vector<std::vector<LocatorData>> m_locatorBuckets;
	std::vector<std::vector<int>> m_locatorBucketIndices;
	BlueSharedString m_locatorSetName;

	bool m_firstSpawnAtRandomPlaces;
	Vector3 FindSpawnPoint();

	EveSpaceObject2* m_parent;

	/////////////////////////////////////////////////////////////////////////////////////
	// locator sets
	const LocatorStructureList* GetLocatorsForSet( const BlueSharedString& setName ) const;
	EveLocatorSetsPtr m_locatorSet;
	void AddLocatorSet();
};

TYPEDEF_BLUECLASS( SeekTarget );

#endif




/*
void EveChildEffectPropagator::ProcessRefLocators( IEveSpaceObject2* parent )
{
	if( m_locatorSetName.empty() )
	{
		m_locatorSetName = BlueSharedString( "damage" );
	}

	const LocatorStructureList* locators;

	if( EveSpaceObject2Ptr spaceObject = BlueCastPtr( parent ) )
	{
		locators = spaceObject->GetLocatorsForSet( m_locatorSetName );
		Vector4 bounds;
		spaceObject->GetBoundingSphere( bounds );
		m_triggerSphereScalarMulti = bounds.w;
	}
	else
	{
		return;
	}

	if( locators )
	{
		for( auto locator = locators->begin(); locator != locators->end(); ++locator )
		{
			if( TriRand() > m_completeness )
			{
				continue;
			}

			Transform t;
			t.position = locator->position;
			t.rotation = locator->direction;
			float rand = m_randScaleMin + TriRand() * (m_randScaleMax - m_randScaleMin);
			t.scale = m_effectScaling * rand;
			m_processedTransforms.emplace_back( t );
		}
	}

}*/
