// Copyright © 2025 CCP ehf.

#pragma once
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"

enum class DistributionEntityLifeTimeEvent
{
	DO_NOTHING = 0,
	KILL_ENTITY = 1,
	KILL_AND_SPAWN_NEW_FROM_DISTRIBUTION = 2,
	KILL_AND_SPAWN_NEW_FROM_INITIAL_POSITION = 3,
	KILL_AND_SPAWN_NEW_FROM_CURRENT_POSITION = 4,
};

struct PlacementDataWithIdentifier
{
	PlacementDataWithIdentifier() :
		initialTranslation( 0.f, 0.f, 0.f ),
		initialRotation( 0.f, 0.f, 0.f, 1.f ),
		initialScale( 1.f, 1.f, 1.f ),
		additionalTranslation( 0.f, 0.f, 0.f ),
		translationFrameDelta( 0.f, 0.f, 0.f ),
		additionalRotation( 0.f, 0.f, 0.f, 1.f ),
		additionalScale( 1.f, 1.f, 1.f ),
		boneIndex( -1 ),
		lifeTime( 0.0 ),
		uniqueID( 0 ),
		initialPlacementID( -1 )
	{
	}

	Vector3 initialTranslation;
	Quaternion initialRotation;
	Vector3 initialScale;
	Vector3 additionalTranslation;
	Vector3 translationFrameDelta; // for motion vectors
	Quaternion additionalRotation;
	Vector3 additionalScale;
	int32_t boneIndex;
	float lifeTime;
	uint32_t uniqueID;
	int32_t initialPlacementID;
};

struct InitialPlacement
{
	// for tracking and timing out triggers
	PlacementDataWithIdentifier placement;
	float timeOutDuration;
};

BLUE_DECLARE_STRUCTURE_LIST( PlacementDataWithIdentifier );

BLUE_INTERFACE( IEveDistributionModifier ) :
	public IRoot
{
public:
	virtual bool AffectsTransform()
	{
		return false;
	}
	virtual DistributionEntityLifeTimeEvent ProcessDistributionModifier( PlacementDataWithIdentifier & placement, float deltaTime, const EveChildUpdateParams& params ) = 0;
};
