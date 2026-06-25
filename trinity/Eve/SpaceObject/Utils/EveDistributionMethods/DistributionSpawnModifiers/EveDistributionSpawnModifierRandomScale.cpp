// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnModifierRandomScale.h"
#include "TriMath.h"
#include "random"

EveDistributionSpawnModifierRandomScale::EveDistributionSpawnModifierRandomScale( IRoot* lockobj ) :
	m_consistentRandom( false ),
	m_uniformScale( false ),
	m_overrideScale( false ),
	m_minScale( 1.f, 1.f, 1.f ),
	m_maxScale( 1.f, 1.f, 1.f )
{
	m_timeSeed = uint32_t( BeOS->GetCurrentFrameTime() );
}

bool EveDistributionSpawnModifierRandomScale::Initialize()
{
	m_timeSeed = uint32_t( BeOS->GetCurrentFrameTime() );
	return true;
}

void EveDistributionSpawnModifierRandomScale::ProcessSpawnModifier( PlacementDataWithIdentifier& placement, size_t numPlacements )
{
	uint32_t randSeed = placement.uniqueID;
	auto generator = std::minstd_rand();
	std::uniform_real_distribution<float> normalizedDistribution( 0.f, 1.f );

	if( !m_consistentRandom )
	{
		randSeed = m_timeSeed << ( randSeed % 11 );
	}

	generator.seed( randSeed );

	Vector3 randomScale( 1.f, 1.f, 1.f );

	if( m_uniformScale )
	{
		randomScale = Lerp( m_minScale, m_maxScale, normalizedDistribution( generator ) );
	}
	else
	{
		randomScale.x = Lerp( m_minScale.x, m_maxScale.x, normalizedDistribution( generator ) );
		randomScale.y = Lerp( m_minScale.y, m_maxScale.y, normalizedDistribution( generator ) );
		randomScale.z = Lerp( m_minScale.z, m_maxScale.z, normalizedDistribution( generator ) );
	}

	if( m_overrideScale )
	{
		placement.initialScale = randomScale;
	}
	else
	{
		placement.initialScale *= randomScale;
	}
}
