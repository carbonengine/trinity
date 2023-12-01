#pragma once
#ifndef GrannyCurveHelpers_h
#define GrannyCurveHelpers_h

#include "granny.h"

template<typename T>
	void GetKeyFrameAtT( granny_curve2* curve, double time, T& value, double duration )
	{
		granny_curve_data_da_keyframes32f* keyframedCurve = reinterpret_cast<granny_curve_data_da_keyframes32f*>( curve->CurveData.Object );

		CCP_ASSERT( keyframedCurve->Dimension == sizeof( T ) / 4 );

		int numFrames = keyframedCurve->ControlCount / keyframedCurve->Dimension;
		int frame = int(float(numFrames) * time / duration);
		T* keys = reinterpret_cast<T*>( keyframedCurve->Controls );
		value = keys[frame];
	}

#endif