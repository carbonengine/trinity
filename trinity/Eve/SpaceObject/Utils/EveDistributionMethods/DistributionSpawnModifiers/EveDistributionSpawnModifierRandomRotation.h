// Copyright © 2025 CCP ehf.

#pragma once
#include "IEveDistributionSpawnModifier.h"

BLUE_CLASS( EveDistributionSpawnModifierRandomRotation ) :
	public IEveDistributionSpawnModifier,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionSpawnModifierRandomRotation( IRoot* lockobj = nullptr );

	void ProcessSpawnModifier( PlacementDataWithIdentifier & placement, size_t numPlacements );

	bool Initialize() override;

private:
	bool m_overrideRotation;
	bool m_consistentRandom;
	uint32_t m_timeSeed;
	Vector3 m_minRotation;
	Vector3 m_maxRotation;
};

TYPEDEF_BLUECLASS( EveDistributionSpawnModifierRandomRotation );
