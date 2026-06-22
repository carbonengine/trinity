// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnModifierLifeTimeOffset.h"
#include "TriMath.h"
#include "random"

EveDistributionSpawnModifierLifeTimeOffset::EveDistributionSpawnModifierLifeTimeOffset( IRoot* lockobj ) :
	m_consistentRandom( false ),
	m_normalizeOffsets( false ),
	m_minOffset( 0.f ),
	m_maxOffset( 0.f ),
	m_cascadingLifetimeOffset( 0.f ),
	m_currentCascadingOffset( 0.f )
{
	m_timeSeed = uint32_t( BeOS->GetCurrentFrameTime() );
}

bool EveDistributionSpawnModifierLifeTimeOffset::Initialize()
{
	m_timeSeed = uint32_t( BeOS->GetCurrentFrameTime() );
	return true;
}

void EveDistributionSpawnModifierLifeTimeOffset::ProcessSpawnModifier( PlacementDataWithIdentifier& placement, size_t numPlacements )
{
	if( m_normalizeOffsets )
	{
		float range = m_maxOffset - m_minOffset;
		float perInstanceOffset = range / float( numPlacements );
		m_currentCascadingOffset += perInstanceOffset;
		float instanceOffset = fmod( m_currentCascadingOffset, range );
		placement.lifeTime = m_minOffset + instanceOffset;
	}
	else
	{
		uint32_t randSeed = placement.uniqueID;
		auto generator = std::minstd_rand();
		std::uniform_real_distribution<float> normalizedDistribution( 0.f, 1.f );

		if( !m_consistentRandom )
		{
			randSeed = m_timeSeed << ( randSeed % 11 );
		}

		generator.seed( randSeed );

		float rndOffset = Lerp( m_minOffset, m_maxOffset, normalizedDistribution( generator ) ) + m_cascadingLifetimeOffset * float( placement.initialPlacementID );
		m_currentCascadingOffset += m_cascadingLifetimeOffset;
		placement.lifeTime += rndOffset;
	}
}
