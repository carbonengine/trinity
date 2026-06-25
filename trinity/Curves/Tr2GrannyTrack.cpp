// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2GrannyTrack.h"
#include "Resources/TriGrannyRes.h"

// Curve set
Tr2GrannyTrack::Tr2GrannyTrack( IRoot* lockobj ) :
	m_duration( 0.0f ),
	m_cycle( false )
{
}

Tr2GrannyTrack::~Tr2GrannyTrack()
{
	if( m_grannyRes )
	{
		m_grannyRes->RemoveNotifyTarget( this );
		m_grannyRes.Unlock();
	}
}

void Tr2GrannyTrack::UpdateValue( double time )
{
	if( !TracksReady() )
	{
		return;
	}

	if( m_cycle )
	{
		time = fmod( time, (double)m_duration );
	}
	// if you go below zero or beyond the duration of the granny curve,
	// granny gets Alzheimer and the result is all over the place
	// plus she will throw some assert popups at you
	if( time <= m_duration && time >= 0.0 )
	{
		UpdateValueImpl( time );
	}
}

bool Tr2GrannyTrack::Initialize()
{
	SetGrannyResource();
	return true;
}

bool Tr2GrannyTrack::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_grannyResPath ) )
	{
		SetGrannyResource();
	}

	if( IsMatch( value, m_name ) ||
		IsMatch( value, m_group ) )
	{
		ResetTracks();
		m_duration = 0.0f;
		SetCurves();
	}

	return true;
}


void Tr2GrannyTrack::ReleaseCachedData( BlueAsyncRes* p )
{
	if( p == m_grannyRes )
	{
		m_duration = 0.0f;
		ResetTracks();
	}
}


void Tr2GrannyTrack::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_grannyRes && p->IsGood() )
	{
		SetCurves();
	}
}

void Tr2GrannyTrack::SetGrannyResource()
{
	if( m_grannyRes )
	{
		m_grannyRes->RemoveNotifyTarget( this );
		m_grannyRes.Unlock();
	}

	if( !m_grannyResPath.empty() )
	{
		BeResMan->GetResource( m_grannyResPath.c_str(), "raw", BlueInterfaceIID<TriGrannyRes>(), (void**)&m_grannyRes );
	}
	else
	{
		ReleaseCachedData( m_grannyRes );
	}

	if( m_grannyRes )
	{
		m_grannyRes->AddNotifyTarget( this );
	}
}

void Tr2GrannyTrack::SetCurves( void )
{
	if( m_name.empty() || m_group.empty() || !m_grannyRes )
	{
		return;
	}

	granny_file* file = m_grannyRes->GetGrannyFile();

	if( !file )
	{
		CCP_LOGERR( "Tr2GrannyTrack::SetCurves: '%s' not found or not a valid Granny file", m_grannyResPath.c_str() );
		return;
	}

	granny_file_info* info = GrannyGetFileInfo( file );
	if( !info )
	{
		CCP_LOGERR( "Tr2GrannyTrack::SetCurves: unable to obtain a granny_file_info from the input file\n" );
		return;
	}

	for( int animIdx = 0; animIdx < info->AnimationCount; ++animIdx )
	{
		granny_animation* animation = info->Animations[animIdx];
		if( !animation )
			continue;
		// find the group we belong to
		for( int tGIdx = 0; tGIdx < animation->TrackGroupCount; ++tGIdx )
		{
			granny_track_group* trackGroup = animation->TrackGroups[tGIdx];
			if( !trackGroup )
				continue;

			if( m_group == trackGroup->Name )
			{
				ApplyTracks( trackGroup, animation->Duration, animation->TimeStep );
				return;
			}
		}
	}
}

granny_curve2* CompressCurve(
	granny_real32 Tolerance,
	granny_real32 dT,
	bool AsQuats,
	granny_data_type_definition** CurveFormats,
	int NumFormats,
	granny_data_type_definition* ShaderType,
	granny_real32* IdentityVector,
	int KnotCount,
	int Dimension,
	std::vector<granny_real32>& Data )
{
	granny_int32x SolverFlags =
		( GrannyBSplineSolverExtraDOFKnotZero |
		  GrannyBSplineSolverForceEndpointAlignment |
		  ( AsQuats ? GrannyBSplineSolverEvaluateAsQuaternions : 0 ) );

	granny_compress_curve_parameters Params;
	Params.DesiredDegree = 2;
	Params.AllowDegreeReduction = true;
	Params.AllowReductionOnMissedTolerance = true;
	Params.ErrorTolerance = Tolerance; // !!!
	Params.C0Threshold = 0.0f;
	Params.C1Threshold = 0.0f;
	Params.PossibleCompressionTypes = CurveFormats;
	Params.PossibleCompressionTypesCount = NumFormats;
	Params.ConstantCompressionType = ShaderType;
	Params.IdentityCompressionType = GrannyCurveDataDaIdentityType;
	Params.IdentityVector = IdentityVector;

	granny_bspline_solver* Solver = GrannyAllocateBSplineSolver( 2, KnotCount, Dimension );

	bool AcheivedTol;
	// convert the key framed curve to a compressed bspline
	granny_curve2* newCurve =
		GrannyCompressCurve( Solver, SolverFlags, &Params, &Data[0], Dimension, KnotCount, dT, &AcheivedTol );

	GrannyDeallocateBSplineSolver( Solver );

	if( newCurve )
	{
		if( !AcheivedTol )
		{
			CCP_LOGWARN( "CompressCurve: Failed to achieve tolerance\n" );
		}
		return newCurve;
	}
	else
	{
		// note error in compression
		CCP_LOGERR( "CompressCurve: Failed to compress curve\n" );
		return NULL;
	}
}
