// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnModifierRandomRotation.h"
#include "TriMath.h"
#include "TriQuaternion.h"
#include "random"

static const Vector3 Y_AXIS( 0.0f, 1.0f, 0.0f );

EveDistributionSpawnModifierRandomRotation::EveDistributionSpawnModifierRandomRotation( IRoot* lockobj ) :
	m_consistentRandom( false ),
	m_overrideRotation( false ),
	m_minRotation( 0.f, 0.f, 0.f ),
	m_maxRotation( 0.f, 0.f, 0.f )
{
	m_timeSeed = uint32_t( BeOS->GetCurrentFrameTime() );
}

bool EveDistributionSpawnModifierRandomRotation::Initialize()
{
	m_timeSeed = uint32_t( BeOS->GetCurrentFrameTime() );
	return true;
}

void EveDistributionSpawnModifierRandomRotation::ProcessSpawnModifier( PlacementDataWithIdentifier& placement, size_t numPlacements )
{
	uint32_t randSeed = placement.uniqueID;
	auto generator = std::minstd_rand();
	std::uniform_real_distribution<float> normalizedDistribution( 0.f, 1.f );

	if( !m_consistentRandom )
	{
		randSeed = m_timeSeed << ( randSeed % 11 );
	}

	generator.seed( randSeed );

	Vector3 randomEulerRotation( 0.f, 0.f, 0.f );

	randomEulerRotation.x = Lerp( m_minRotation.x, m_maxRotation.x, normalizedDistribution( generator ) );
	randomEulerRotation.y = Lerp( m_minRotation.y, m_maxRotation.y, normalizedDistribution( generator ) );
	randomEulerRotation.z = Lerp( m_minRotation.z, m_maxRotation.z, normalizedDistribution( generator ) );

	Quaternion rotation = RotationQuaternion( randomEulerRotation.x, randomEulerRotation.y, randomEulerRotation.z );

	if( m_overrideRotation )
	{
		placement.initialRotation = rotation;
	}
	else
	{
		placement.initialRotation = rotation * placement.initialRotation;
	}
}
