#pragma once
#ifndef Tr2GrannyTransformTrack_h
#define Tr2GrannyTransformTrack_h

#include "include/ITriFunction.h"
#include "include/ITriCurveLength.h"
#include "Curves/Tr2GrannyTrack.h"
BLUE_DECLARE( TriGrannyRes );

BLUE_CLASS( Tr2GrannyTransformTrack ):
	public Tr2GrannyTrack
{
public:
	EXPOSE_TO_BLUE();
	Tr2GrannyTransformTrack( IRoot* lockobj = NULL );
	void UpdateValueImpl( double time );
	void ResetTracks( void );
	void ApplyTracks( granny_track_group* group, float duration, float timeStep );
	bool TracksReady( void );

protected:

	template<typename T>
	void GetKeyFrameAtT( granny_curve2* curve, double time, T& value )
	{
		granny_curve_data_da_keyframes32f* keyframedCurve = reinterpret_cast<granny_curve_data_da_keyframes32f*>( curve->CurveData.Object );

		CCP_ASSERT( keyframedCurve->Dimension == sizeof( T ) / 4 );

		int numFrames = keyframedCurve->ControlCount / keyframedCurve->Dimension;
		int frame = int(float(numFrames) * time / m_duration);
		T* keys = reinterpret_cast<T*>( keyframedCurve->Controls );
		value = keys[frame];
	}

	Vector3 m_translation;
	Quaternion m_rotation;
	Vector3 m_scale;
	granny_curve2* m_positionCurve;
	granny_curve2* m_orientationCurve;
	granny_curve2* m_scaleCurve;
	bool m_compressCurves;

};

TYPEDEF_BLUECLASS( Tr2GrannyTransformTrack );

#endif //Tr2GrannyTransformTrack_h
