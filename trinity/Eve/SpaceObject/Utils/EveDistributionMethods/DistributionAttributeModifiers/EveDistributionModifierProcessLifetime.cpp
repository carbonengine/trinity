// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionModifierProcessLifetime.h"

EveDistributionModifierProcessLifetime::EveDistributionModifierProcessLifetime( IRoot* lockobj ) :
	m_lifetimeDuration( -1.f ),
	m_killEvent( DistributionEntityLifeTimeEvent::KILL_ENTITY )
{
}

DistributionEntityLifeTimeEvent EveDistributionModifierProcessLifetime::ProcessDistributionModifier( PlacementDataWithIdentifier& placement, float deltaTime, const EveChildUpdateParams& params )
{
	if( placement.lifeTime > m_lifetimeDuration && m_lifetimeDuration > 0.f )
	{
		return m_killEvent;
	}
	return DistributionEntityLifeTimeEvent::DO_NOTHING;
}
