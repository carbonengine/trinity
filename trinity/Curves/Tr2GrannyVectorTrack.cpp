#include "StdAfx.h"
#include "Tr2GrannyVectorTrack.h"
#include "Resources/TriGrannyRes.h"
#include "Utilities/GrannyCurveHelpers.h"

// Curve set
Tr2GrannyVectorTrack::Tr2GrannyVectorTrack( IRoot* lockobj ): 
	Tr2GrannyTrack( lockobj ),	
	m_valueCurve( NULL )
{
	m_value = 0.0f;
}

bool Tr2GrannyVectorTrack::TracksReady( void )
{
	return m_valueCurve != NULL;
}

void Tr2GrannyVectorTrack::UpdateValueImpl( double time )
{
	float defaultValue = 0.f;
	if( GrannyCurveIsKeyframed( m_valueCurve ) )
	{
		GetKeyFrameAtT( m_valueCurve, time, m_value, m_duration );
	}
	else
	{
		GrannyEvaluateCurveAtT( 1, false, false, m_valueCurve, false, m_duration, (float)time,(float*)&m_value, &defaultValue );
	}
}

void Tr2GrannyVectorTrack::ResetTracks( void )
{
	m_valueCurve = NULL;
}

void Tr2GrannyVectorTrack::ApplyTracks( granny_track_group* group, float duration, float timeStep )
{
	// find the track we want to sample
	for( int tTIdx = 0; tTIdx < group->VectorTrackCount; ++tTIdx )
	{
		granny_vector_track& track = group->VectorTracks[tTIdx];					

		if( m_name == track.Name )
		{
			m_duration = duration;
			m_valueCurve = &track.ValueCurve;
			UpdateValue( 0.0 );// set the default values to start values of the curve
			return;
		}								
	}
}
