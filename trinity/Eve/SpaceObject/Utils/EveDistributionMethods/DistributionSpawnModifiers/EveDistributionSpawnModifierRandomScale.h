// Copyright © 2025 CCP ehf.

#pragma once
#include "IEveDistributionSpawnModifier.h"

BLUE_CLASS( EveDistributionSpawnModifierRandomScale ) :
	public IEveDistributionSpawnModifier,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionSpawnModifierRandomScale( IRoot* lockobj = nullptr );

	void ProcessSpawnModifier( PlacementDataWithIdentifier & placement, size_t numPlacements );

	bool Initialize() override;

private:
	bool m_overrideScale;
	bool m_consistentRandom;
	bool m_uniformScale;
	uint32_t m_timeSeed;
	Vector3 m_minScale;
	Vector3 m_maxScale;
};

TYPEDEF_BLUECLASS( EveDistributionSpawnModifierRandomScale );
