#include "StdAfx.h"
#include "Tr2GrannyTransformTrack.h"
#include "Resources/TriGrannyRes.h"
#include "include/TriMath.h"
#include "Utilities/GrannyCurveHelpers.h"

// Curve set
Tr2GrannyTransformTrack::Tr2GrannyTransformTrack( IRoot* lockobj ): 
	Tr2GrannyTrack( lockobj ),	
	m_positionCurve( NULL ),
	m_orientationCurve( NULL ),
	m_scaleCurve( NULL ),
	m_compressCurves( false ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_scale( 0.0f, 0.0f, 0.0f )
{
}

bool Tr2GrannyTransformTrack::TracksReady( void )
{
	return ( m_positionCurve != NULL && m_orientationCurve != NULL && m_scaleCurve != NULL);
}

void Tr2GrannyTransformTrack::UpdateValueImpl( double time )
{
	float scaleShear[9];
	if( GrannyCurveIsKeyframed( m_positionCurve ) )
	{
		GetKeyFrameAtT( m_positionCurve, time, m_translation, m_duration );
	}
	else
	{
		GrannyEvaluateCurveAtT( 3, false, false, m_positionCurve, false, m_duration, (float)time,(float*)&m_translation, GrannyCurveIdentityPosition );
	}

	if( GrannyCurveIsKeyframed( m_orientationCurve ) )
	{
		GetKeyFrameAtT( m_orientationCurve, time, m_rotation, m_duration );
	}
	else
	{
		GrannyEvaluateCurveAtT( 4, false, false, m_orientationCurve, false, m_duration, (float)time,(float*)&m_rotation, GrannyCurveIdentityOrientation );
	}

	GrannyEvaluateCurveAtT( 9, false, false, m_scaleCurve, false, m_duration, (float)time, scaleShear, GrannyCurveIdentityScaleShear );
	m_scale.x = XMVectorGetX(XMVector3Length( *(Vector3*)&scaleShear[0] ));
	m_scale.y = XMVectorGetX(XMVector3Length( *(Vector3*)&scaleShear[3] ));
	m_scale.z = XMVectorGetX(XMVector3Length( *(Vector3*)&scaleShear[6] ));
}

void Tr2GrannyTransformTrack::ResetTracks( void )
{
	m_positionCurve = NULL;
	m_orientationCurve = NULL;
	m_scaleCurve = NULL;
}

void Tr2GrannyTransformTrack::ApplyTracks( granny_track_group* group, float duration, float timeStep )
{
	for( int tTIdx = 0; tTIdx < group->TransformTrackCount; ++tTIdx )
	{
		granny_transform_track& track = group->TransformTracks[tTIdx];					

		if( m_name == track.Name )
		{
			m_duration = duration;
			// Assumption: either the track is keyframed, or it's already had the
			// compression we want applied to it.  This also prevents us from
			// considering tracks that were reduced in the export process to constant
			// or identity.
			if( m_compressCurves && GrannyCurveIsKeyframed( &track.PositionCurve ) )
			{
				int KnotCount = GrannyCurveGetKnotCount( &track.PositionCurve );
				int Dimension = GrannyCurveGetDimension( &track.PositionCurve );

				std::vector<granny_real32> Data( KnotCount * Dimension );
				GrannyCurveExtractKnotValues( &track.PositionCurve, 0, KnotCount, 0, &Data[0], GrannyCurveIdentityPosition );

				m_positionCurve = CompressCurve(
					PositionTolerance, // error tolerance					
					timeStep, // the time step between frames
					false,				 // solve as quaternions
					PositionCurveFormats, // possible compression formats
					ArrayLength(PositionCurveFormats),// number of compression formats
					GrannyCurveDataD3Constant32fType, // constant compression type
					GrannyCurveIdentityPosition,
					KnotCount,
					Dimension,
					Data
					);   //
			}			 
			else
			{
				m_positionCurve = &track.PositionCurve;
			}

			if( m_compressCurves && GrannyCurveIsKeyframed( &track.OrientationCurve ) )
			{
				int KnotCount = GrannyCurveGetKnotCount( &track.OrientationCurve );
				int Dimension = GrannyCurveGetDimension( &track.OrientationCurve );

				std::vector<granny_real32> Data( KnotCount * Dimension );
				GrannyCurveExtractKnotValues( &track.OrientationCurve, 0, KnotCount, 0, &Data[0], GrannyCurveIdentityOrientation );
				m_orientationCurve = CompressCurve(
					OrientationTolerance,
					timeStep, 
					true,
					OrientationCurveFormats, 
					ArrayLength(OrientationCurveFormats),
					GrannyCurveDataD4Constant32fType,
					GrannyCurveIdentityOrientation,
					KnotCount,
					Dimension,
					Data
					);
			}
			else
			{
				m_orientationCurve = &track.OrientationCurve;
			}

			
			if( GrannyCurveIsKeyframed( &track.ScaleShearCurve ) )
			{
				int KnotCount = GrannyCurveGetKnotCount( &track.ScaleShearCurve );
				int Dimension = GrannyCurveGetDimension( &track.ScaleShearCurve );

				std::vector<granny_real32> Data( KnotCount * Dimension );
				GrannyCurveExtractKnotValues( &track.ScaleShearCurve, 0, KnotCount, 0, &Data[0], GrannyCurveIdentityScaleShear );
				m_scaleCurve = CompressCurve(
					ScaleShearTolerance,					
					timeStep, 
					false,
					ScaleShearCurveFormats, 
					ArrayLength(ScaleShearCurveFormats),
					GrannyCurveDataDaConstant32fType,
					GrannyCurveIdentityScaleShear,
					KnotCount,
					Dimension,
					Data
					);
			}
			else
			{
				m_scaleCurve = &track.ScaleShearCurve;
			}
			UpdateValue( 0.0 );// set the default values to start values of the curve
			return;
		}								
	}
}
