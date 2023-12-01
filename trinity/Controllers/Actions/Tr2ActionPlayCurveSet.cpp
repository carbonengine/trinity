////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionPlayCurveSet.h"
#include "Controllers/Tr2Controller.h"
#include "ITr2CurveSetOwner.h"
#include "Curves/TriCurveSet.h"

#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Eve/EveEffectRoot2.h"


Tr2ActionPlayCurveSet::Tr2ActionPlayCurveSet( IRoot* )
	:m_startTime( 0 ),
	m_prevTime( 0 ),
	m_duration( 0 ),
	m_syncToRange( false )
{
}

void Tr2ActionPlayCurveSet::Start( Tr2Controller& controller )
{
	m_duration = 0;
	if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( controller.GetOwner() ) )
	{
		owner->PlayCurveSet( m_curveSetName, m_rangeName );

		if( m_syncToRange && !m_rangeName.empty() )
		{
			m_duration = owner->GetRangeDuration( m_curveSetName, m_rangeName );
			m_startTime = BeOS->GetCurrentFrameTime();
			m_prevTime = m_startTime;
			controller.RegisterUpdateable( *this );
		}
	}
}

void Tr2ActionPlayCurveSet::Stop( Tr2Controller& controller )
{
	controller.UnRegisterUpdateable( *this );
	if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( controller.GetOwner() ) )
	{
		owner->StopCurveSet( m_curveSetName );
	}
}

void Tr2ActionPlayCurveSet::RebaseSimTime( Be::Time diff )
{
	m_startTime += diff;
	m_prevTime += diff;
}

bool Tr2ActionPlayCurveSet::CanTransition() const
{
	if( !m_syncToRange || m_duration <= 0 )
	{
		return true;
	}
	auto now = BeOS->GetCurrentFrameTime();
	if( now == m_startTime )
	{
		return true;
	}
	int32_t prevIteration = int32_t( TimeAsFloat( m_prevTime - m_startTime ) / m_duration );
	int32_t iteration = int32_t( TimeAsFloat( now - m_startTime ) / m_duration );
	return iteration != prevIteration;
}

void Tr2ActionPlayCurveSet::Update( Be::Time realTime, Be::Time simTime )
{
	m_prevTime = BeOS->GetCurrentFrameTime();
}