// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "IEveDistributionSpawner.h"

BLUE_CLASS( EveDistributionSpawnerTriggerSnake ) :
	public IEveDistributionSpawner
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionSpawnerTriggerSnake( IRoot* lockobj = nullptr );

	void Reset( const std::vector<InitialPlacement>& placements ) override;
	void Restart() override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner ) override;

private:
	int32_t m_totalDestinations;
	int32_t m_numDestinationsReached;
	uint32_t m_activeTargetUniqueID;
	Vector3 m_targetPoint;
	Vector3 m_lastTarget;
	float m_minTimeBetweenTriggers;
	float m_maxTimeBetweenTriggers;
	float m_currentTravelTime;
	float m_travelProgress;
	float m_travelDurationToNextPoint;
	float m_distanceToTravelTimeMultiplier;
};

TYPEDEF_BLUECLASS( EveDistributionSpawnerTriggerSnake );
