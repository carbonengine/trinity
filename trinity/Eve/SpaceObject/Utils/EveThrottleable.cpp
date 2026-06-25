// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveThrottleable.h"


EveThrottleable::EveThrottleable( IRoot* lockobj ) :
	m_minUpdateFrequency( 2 ),
	m_maxUpdateFrequency( 20 ),
	m_currentUpdateFrequency( 10.f ),
	m_nextUpdateTS( 0 ),
	m_updateThrottle( true )
{
}

bool EveThrottleable::ShouldSkipUpdate( float normalizedUpdateFrequency )
{
	auto currentTime = BeOS->GetActualTime();

	if( !m_updateThrottle )
	{
		return false;
	}

	// skip updates based on the current distance related update frequency
	if( currentTime < m_nextUpdateTS )
	{
		return true;
	}

	float updateFrequency = normalizedUpdateFrequency * float( m_maxUpdateFrequency - m_minUpdateFrequency ) + float( m_minUpdateFrequency );
	m_currentUpdateFrequency = max( updateFrequency, 0.1f ); // floor: update every 10 sec at the least
	m_nextUpdateTS = currentTime + TimeFromDouble( 1.0 / m_currentUpdateFrequency );
	return false;
}
