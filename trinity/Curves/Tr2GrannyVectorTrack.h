#pragma once
#ifndef Tr2GrannyVectorTrack_h
#define Tr2GrannyVectorTrack_h

#include "include/ITriFunction.h"
#include "include/ITriCurveLength.h"
#include "Curves/Tr2GrannyTrack.h"
BLUE_DECLARE( TriGrannyRes );

BLUE_CLASS( Tr2GrannyVectorTrack ):
public Tr2GrannyTrack
{
public:
	EXPOSE_TO_BLUE();
	Tr2GrannyVectorTrack( IRoot* lockobj = NULL );
	void UpdateValueImpl( double time );
	void ResetTracks( void );
	void ApplyTracks( granny_track_group* group, float duration, float timeStep );
	bool TracksReady( void );

protected:

	float m_value;
	granny_curve2* m_valueCurve;
};

TYPEDEF_BLUECLASS( Tr2GrannyVectorTrack );

#endif //Tr2GrannyVectorTrack_h
