// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2GrannyVectorTrack_h
#define Tr2GrannyVectorTrack_h

#include "Curves/Tr2GrannyTrack.h"

#include <ITriFunction.h>
#include <ITriCurveLength.h>

BLUE_DECLARE( TriGrannyRes );

BLUE_CLASS( Tr2GrannyVectorTrack ) :
	public Tr2GrannyTrack
{
public:
	EXPOSE_TO_BLUE();
	Tr2GrannyVectorTrack( IRoot* lockobj = NULL );
	void UpdateValueImpl( double time ) override;
	void ResetTracks( void ) override;
	void ApplyTracks( granny_track_group * group, float duration, float timeStep ) override;
	bool TracksReady( void ) override;

protected:
	float m_value;
	granny_curve2* m_valueCurve;
};

TYPEDEF_BLUECLASS( Tr2GrannyVectorTrack );

#endif //Tr2GrannyVectorTrack_h
