////////////////////////////////////////////////////////////
//
//    Created:   May 2014
//    Copyright: CCP 2014
//

#include "StdAfx.h"
#include "Tr2GrannyEventTrack.h"

Tr2GrannyEventTrack::Tr2GrannyEventTrack( IRoot* lockobj )
	:m_track( nullptr ),
	m_previousTime( 0 ),
	m_previousIndex( 0 )
{
}

void Tr2GrannyEventTrack::UpdateValueImpl( double time )
{
	if( time < m_previousTime )
	{
		m_previousTime = 0;
		m_previousIndex = 0;
	}
	while( m_previousIndex < m_track->EntryCount )
    {
        auto& entry = m_track->Entries[m_previousIndex];
        if( ( entry.TimeStamp >= m_previousTime ) && ( entry.TimeStamp <= time ) )
        {
			if( m_eventListener )
			{
				m_eventListener->HandleEvent( CA2W( entry.Text ) );
			}
        }
		else if( entry.TimeStamp > time )
		{
			break;
		}
		++m_previousIndex;
    }
	m_previousTime = time;
}

void Tr2GrannyEventTrack::ResetTracks( void )
{
	m_track = nullptr;
	m_previousTime = 0;
	m_previousIndex = 0;
}

void Tr2GrannyEventTrack::ApplyTracks( granny_track_group* group, float duration, float timeStep )
{
	// find the track we want to sample
	for( int i = 0; i < group->TextTrackCount; ++i )
	{
		granny_text_track& track = group->TextTracks[i];					

		if( m_name == track.Name )
		{
			m_duration = duration;
			m_track = &track;
			m_previousTime = 0;
			m_previousIndex = 0;
			return;
		}								
	}
}

bool Tr2GrannyEventTrack::TracksReady( void )
{
	return m_track != nullptr;
}
