// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "IEveDistributionSpawner.h"

BLUE_CLASS( EveDistributionSpawnerInterval ) :
	public IEveDistributionSpawner
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionSpawnerInterval( IRoot* lockobj = nullptr );

	void Reset( const std::vector<InitialPlacement>& placements ) override;
	void Restart() override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner ) override;

private:
	float m_delayBeforeInitialSpawn;
	bool m_useRandomStartOffset;
	uint32_t m_numberOfTriggers;
	float m_delayBetweenRepeats;
	float m_localTimer;
	uint32_t m_numTriggered;
	float m_currentRandomizedIntervalDelta;
	float m_maxRandomizedIntervalDelta;
};

TYPEDEF_BLUECLASS( EveDistributionSpawnerInterval );
