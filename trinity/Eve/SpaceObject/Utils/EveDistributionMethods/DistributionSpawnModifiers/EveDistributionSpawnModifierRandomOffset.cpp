// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnModifierRandomOffset.h"
#include "TriMath.h"
#include "random"

EveDistributionSpawnModifierRandomOffset::EveDistributionSpawnModifierRandomOffset( IRoot* lockobj ) :
	m_consistentRandom( false ),
	m_uniformOffset( false ),
	m_minOffset( 0.f, 0.f, 0.f ),
	m_maxOffset( 0.f, 0.f, 0.f )
{
	m_timeSeed = uint32_t( BeOS->GetCurrentFrameTime() );
}

bool EveDistributionSpawnModifierRandomOffset::Initialize()
{
	m_timeSeed = uint32_t( BeOS->GetCurrentFrameTime() );
	return true;
}

void EveDistributionSpawnModifierRandomOffset::ProcessSpawnModifier( PlacementDataWithIdentifier& placement, size_t numPlacements )
{
	uint32_t randSeed = placement.uniqueID;
	auto generator = std::minstd_rand();
	std::uniform_real_distribution<float> sizeDistribution( 0.f, 1.f );

	if( !m_consistentRandom )
	{
		randSeed = m_timeSeed << ( randSeed % 11 );
	}

	generator.seed( randSeed );

	Vector3 randomOffset( 0.f, 0.f, 0.f );

	if( m_uniformOffset )
	{
		randomOffset = Lerp( m_minOffset, m_maxOffset, sizeDistribution( generator ) );
	}
	else
	{
		randomOffset.x = Lerp( m_minOffset.x, m_maxOffset.x, sizeDistribution( generator ) );
		randomOffset.y = Lerp( m_minOffset.y, m_maxOffset.y, sizeDistribution( generator ) );
		randomOffset.z = Lerp( m_minOffset.z, m_maxOffset.z, sizeDistribution( generator ) );
	}

	TriVectorRotateQuaternion( &randomOffset, &randomOffset, &placement.initialRotation );
	placement.initialTranslation += randomOffset;
}
