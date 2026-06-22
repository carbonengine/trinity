// Copyright © 2025 CCP ehf.

#pragma once
#include "IEveDistributionSpawnModifier.h"

BLUE_CLASS( EveDistributionSpawnModifierLifeTimeOffset ) :
	public IEveDistributionSpawnModifier,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionSpawnModifierLifeTimeOffset( IRoot* lockobj = nullptr );

	void ProcessSpawnModifier( PlacementDataWithIdentifier & placement, size_t numPlacements );
	bool Initialize() override;

private:
	float m_cascadingLifetimeOffset;
	float m_currentCascadingOffset;
	float m_minOffset;
	float m_maxOffset;
	bool m_consistentRandom;
	bool m_normalizeOffsets;
	uint32_t m_timeSeed;
};

TYPEDEF_BLUECLASS( EveDistributionSpawnModifierLifeTimeOffset );
