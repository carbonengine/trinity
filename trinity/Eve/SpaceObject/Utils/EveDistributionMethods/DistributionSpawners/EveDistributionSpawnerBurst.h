// Copyright © 2025 CCP ehf.

#pragma once
#include "IEveDistributionSpawner.h"

BLUE_CLASS( EveDistributionSpawnerBurst ) :
	public IEveDistributionSpawner
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionSpawnerBurst( IRoot* lockobj = nullptr );

	void Reset( const std::vector<InitialPlacement>& placements ) override;
	void Restart() override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner ) override;

private:
	float m_delayBeforeInitialBurst;
	float m_completeness;
	float m_localTimer;
	uint32_t m_additionalTriggersPerBurst;
};

TYPEDEF_BLUECLASS( EveDistributionSpawnerBurst );
