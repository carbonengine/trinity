// Copyright © 2025 CCP ehf.

#pragma once
#include "IEveDistributionSpawnModifier.h"

BLUE_CLASS( EveDistributionSpawnModifierRandomOffset ) :
	public IEveDistributionSpawnModifier,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionSpawnModifierRandomOffset( IRoot* lockobj = nullptr );

	void ProcessSpawnModifier( PlacementDataWithIdentifier & placement, size_t numPlacements );

	bool Initialize() override;

private:
	bool m_consistentRandom;
	bool m_uniformOffset;
	uint32_t m_timeSeed;
	Vector3 m_minOffset;
	Vector3 m_maxOffset;
};

TYPEDEF_BLUECLASS( EveDistributionSpawnModifierRandomOffset );
