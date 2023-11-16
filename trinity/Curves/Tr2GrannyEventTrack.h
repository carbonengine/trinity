////////////////////////////////////////////////////////////
//
//    Created:   May 2014
//    Copyright: CCP 2014
//

#pragma once
#ifndef Tr2GrannyEventTrack_H
#define Tr2GrannyEventTrack_H

#include "Tr2GrannyTrack.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2GrannyEventTrack is a curve used to playback Granny text tracks. For each text
//   key the class executes provided IBlueEventListener.
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2GrannyEventTrack ): public Tr2GrannyTrack
{
public:
	Tr2GrannyEventTrack( IRoot* lockobj = NULL );

	EXPOSE_TO_BLUE();

	void UpdateValueImpl( double time );
	void ResetTracks( void );
	void ApplyTracks( granny_track_group* group, float duration, float timeStep );
	bool TracksReady( void );
protected:
	granny_text_track* m_track;
	double m_previousTime;
	granny_int32x m_previousIndex;
	IBlueEventListenerPtr m_eventListener;
};

TYPEDEF_BLUECLASS( Tr2GrannyEventTrack );

#endif