// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriCurveSet.h"
#include "TriValueBinding.h"
#include <ICurveSetDriver.h>
#include "include/ITriDuration.h"

#include <ITriFunction.h>
#include <ITriCurveLength.h>

Tr2CurveSetRange::Tr2CurveSetRange( IRoot* ) :
	m_startTime( 0 ),
	m_endTime( 1 ),
	m_looped( false )
{
}


TriCurveSet::TriCurveSet( IRoot* lockobj ) :
	PARENTLOCK( m_curves ),
	PARENTLOCK( m_bindings ),
	PARENTLOCK( m_ranges ),
	m_driver(),
	m_isPlaying( false ),
	m_stopOnNextFrame( false ),
	m_playOnLoad( true ),
	m_useSimTimeRebase( false ),
	m_isUsingSimTimeRebase( false ),
	m_useRealTime( false ),
	m_hasTimeRange( false ),
	m_loopedTimeRange( true ),
	m_startTime( 0.0 ),
	m_lastTime( 0.0 ),
	m_endTime( 0.0 ),
	m_scaledTime( 0.0 ),
	m_scale( 1.0f ),
	m_timeRangeMin( 0 ),
	m_timeRangeMax( 0 )
{
}

TriCurveSet::~TriCurveSet()
{
	if( m_isUsingSimTimeRebase )
	{
		BeOS->UnregisterForSimTimeRebase( this );
	}
}

void TriCurveSet::Update( Be::Time realTime, Be::Time simTime )
{
	double time;
	if( m_useRealTime )
	{
		time = TimeAsDouble( realTime );
	}
	else
	{
		time = TimeAsDouble( simTime );
	}

	Update( time );
}


void TriCurveSet::OnSimClockRebase( Be::Time oldTime, Be::Time newTime )
{
	double diff = TimeAsDouble( newTime - oldTime );
	m_startTime += diff;
	if( m_endTime > 0.0 )
	{
		m_endTime += diff;
	}
}


void TriCurveSet::Update( double time )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_driver != nullptr )
	{
		time = m_driver->GetCurveSetTime( time );
	}

	if( m_endTime < 0.0 )
	{
		// StopAfter sets m_endTime to the negative value. Subtracting it here
		// really means adding the absolute value.
		m_endTime = time - m_endTime;
	}

	if( m_isPlaying )
	{
		if( m_startTime < 0.0 )
		{
			if( m_driver != nullptr )
			{
				m_startTime = 0;
			}
			else
			{
				// Negative start time means it hasn't been set - set it to the current time.
				m_startTime = time;
			}
		}

		double now = time - m_startTime;
		double delta = now - m_lastTime;
		m_lastTime = now;

		now = m_scaledTime + (double)m_scale * delta;
		m_scaledTime = now;

		if( m_hasTimeRange )
		{
			if( m_scaledTime < m_timeRangeMin )
			{
				m_scaledTime = m_timeRangeMin;
			}

			if( m_loopedTimeRange )
			{
				m_scaledTime = std::fmod( m_scaledTime - m_timeRangeMin, m_timeRangeMax - m_timeRangeMin ) + m_timeRangeMin;
			}
			else
			{
				m_scaledTime = std::min( m_scaledTime, m_timeRangeMax );
			}
		}

		if( ( m_endTime > 0.0 ) && ( ( m_startTime + m_scaledTime ) >= m_endTime ) )
		{
			m_scaledTime = m_endTime - m_startTime - 0.001f;
			m_stopOnNextFrame = true;
		}

		Apply();
	}

	if( m_stopOnNextFrame )
	{
		if( m_callback )
		{
			m_callback.CallVoid();
			m_callback.Destroy();
		}
		m_isPlaying = false;
		m_stopOnNextFrame = false;
	}
}

void TriCurveSet::Apply()
{
	for( ITriFunctionVector::const_iterator it = m_curves.begin(); it != m_curves.end(); ++it )
	{
		( *it )->UpdateValue( m_scaledTime );
	}

	for( ITr2ValueBindingVector::const_iterator it = m_bindings.begin(); it != m_bindings.end(); ++it )
	{
		( *it )->CopyValue();
	}
}

void TriCurveSet::ApplyTime( double time )
{
	m_scaledTime = time;
	Apply();
}

bool TriCurveSet::Initialize()
{
	if( m_playOnLoad )
	{
		Play();
	}

	if( m_useSimTimeRebase )
	{
		BeOS->RegisterForSimTimeRebase( this );
		m_isUsingSimTimeRebase = true;
	}
	return true;
}

void TriCurveSet::Play()
{
	PlayFrom( 0.0 );
}

void TriCurveSet::PlayTimeRange( const char* name )
{
	bool rangeFound = false;
	for( auto it = begin( m_ranges ); it != end( m_ranges ); ++it )
	{
		auto range = *it;
		if( range->m_name == name )
		{
			rangeFound = true;
			SetTimeRange( range->m_startTime, range->m_endTime, range->m_looped );
			Play();
			return;
		}
	}

	if( rangeFound )
	{
		Stop();
	}
}

void TriCurveSet::PlayFrom( double time )
{
	m_startTime = -1.0;
	m_endTime = 0.0;
	m_isPlaying = true;
	m_lastTime = 0.0;
	m_scaledTime = time;

	// some curves need to know about a play start (event curves)
	for( ITriFunctionVector::const_iterator it = m_curves.begin(); it != m_curves.end(); ++it )
	{
		( *it )->Reset();
	}

	m_callback.Destroy();
}

void TriCurveSet::Stop()
{
	m_isPlaying = false;
}

void TriCurveSet::StopOnNextFrame()
{
	m_stopOnNextFrame = true;
}

void TriCurveSet::StopAfter( double seconds )
{
	m_endTime = -seconds;
}

void TriCurveSet::StopAfterWithCallback( double seconds, const BlueScriptCallback& cb )
{
	StopAfter( seconds );
	m_callback = cb;
}

float TriCurveSet::GetTimeScale() const
{
	return m_scale;
}

double TriCurveSet::GetScaledTime() const
{
	return m_scaledTime;
}

void TriCurveSet::SetName( const std::string& name )
{
	m_name = name;
}

void TriCurveSet::AddCurve( ITriFunctionPtr curve )
{
	m_curves.Append( curve );
}

void TriCurveSet::AddBinding( ITr2ValueBindingPtr binding )
{
	m_bindings.Append( binding );
}

float TriCurveSet::GetMaxCurveDuration() const
{
	float maxDuration = 0.0f;

	for( ITriFunctionVector::const_iterator it = m_curves.begin(); it != m_curves.end(); ++it )
	{
		float l = 0.0f;
		if( ITriDurationPtr f = BlueCastPtr( *it ) )
		{
			l = f->Length();
		}
		else if( ITriCurveLengthPtr f = BlueCastPtr( *it ) )
		{
			l = f->Length();
		}

		if( l > maxDuration )
		{
			maxDuration = l;
		}
	}

	return maxDuration;
}

float TriCurveSet::GetRangeDuration( const char* rangeName ) const
{
	for( auto it = begin( m_ranges ); it != end( m_ranges ); ++it )
	{
		auto range = *it;
		if( range->m_name == rangeName )
		{
			return range->m_endTime - range->m_startTime;
		}
	}
	return 0;
}

bool TriCurveSet::IsPlaying() const
{
	return m_isPlaying;
}

void TriCurveSet::UpdateWithCurrentTime()
{
	Update( double( BeOS->GetCurrentFrameTime() ) );
}

void TriCurveSet::SetTimeRange( double timeMin, double timeMax, Be::OptionalWithDefaultValue<bool, true> looped )
{
	m_hasTimeRange = true;
	m_timeRangeMin = std::min( timeMin, timeMax );
	m_timeRangeMax = std::max( timeMin, timeMax );
	m_loopedTimeRange = looped;
}

void TriCurveSet::ResetTimeRange()
{
	m_hasTimeRange = false;
	m_timeRangeMin = m_timeRangeMax = 0;
}

bool TriCurveSet::HasTimeRange() const
{
	return m_hasTimeRange;
}

std::pair<double, double> TriCurveSet::GetTimeRange() const
{
	return std::make_pair( m_timeRangeMin, m_timeRangeMax );
}
