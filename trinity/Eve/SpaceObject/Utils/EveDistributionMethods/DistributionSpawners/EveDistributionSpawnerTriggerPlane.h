// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "IEveDistributionSpawner.h"

BLUE_CLASS( EveDistributionSpawnerTriggerPlane ) :
	public IEveDistributionSpawner
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionSpawnerTriggerPlane( IRoot* lockobj = nullptr );

	void Reset( const std::vector<InitialPlacement>& placements ) override;
	void Restart() override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner ) override;

private:
	std::vector<std::pair<float, uint32_t>> m_distSortedIndexes;
	Quaternion m_planeRotation;
	float m_delayBeforeActivation;
	float m_playDuration;
	float m_currentPlayTime;
	float m_triggerChance;
	uint32_t m_currentTrigger;
	bool m_startSequenceAtFirstTrigger;
	bool m_reversePlaneAnimation;
};

TYPEDEF_BLUECLASS( EveDistributionSpawnerTriggerPlane );
