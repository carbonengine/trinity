// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2GrannyTrack_h
#define Tr2GrannyTrack_h

#include <ITriFunction.h>
#include <ITriCurveLength.h>

BLUE_DECLARE( TriGrannyRes );

// This static compressCurve func comes from the Granny Preprocessor
#define ArrayLength( x ) ( sizeof( x ) / sizeof( ( x )[0] ) )

static granny_real32 PositionTolerance = 0.1f;
static granny_real32 OrientationTolerance = GrannyOrientationToleranceFromDegrees( 0.1f ); // important! :)
static granny_real32 ScaleShearTolerance = 0.1f;

static granny_data_type_definition* PositionCurveFormats[] = {
	GrannyCurveDataDaK32fC32fType,
	GrannyCurveDataD3I1K8uC8uType,
	GrannyCurveDataD3I1K16uC16uType,
	GrannyCurveDataD3K8uC8uType,
	GrannyCurveDataD3K16uC16uType,
	GrannyCurveDataD3I1K32fC32fType,
};

static granny_data_type_definition* OrientationCurveFormats[] = {
	GrannyCurveDataDaK32fC32fType,
	GrannyCurveDataD4nK8uC7uType,
	GrannyCurveDataD4nK16uC15uType,
};

static granny_data_type_definition* ScaleShearCurveFormats[] = {
	GrannyCurveDataDaK32fC32fType,
	GrannyCurveDataD9I1K8uC8uType,
	GrannyCurveDataD9I3K8uC8uType,
	GrannyCurveDataDaK8uC8uType,
	GrannyCurveDataDaK16uC16uType,
	GrannyCurveDataD9I1K16uC16uType,
	GrannyCurveDataD9I3K16uC16uType,
};
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
	std::vector<granny_real32>& Data );

BLUE_CLASS( Tr2GrannyTrack ) :
	public ITriFunction,
	public IInitialize,
	public ITriCurveLength,
	public INotify,
	public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();
	Tr2GrannyTrack( IRoot* lockobj = NULL );
	~Tr2GrannyTrack();
	using IInitialize::Lock;
	using IInitialize::Unlock;

	//////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes * p );
	void RebuildCachedData( BlueAsyncRes * p );

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var * value );

	void SetGrannyResource();

	virtual void UpdateValue( double time );

	virtual void UpdateValueImpl( double time ) = 0;
	virtual void ResetTracks( void ) = 0;
	virtual void ApplyTracks( granny_track_group * group, float duration, float timeStep ) = 0;
	virtual bool TracksReady( void ) = 0;

	float Length()
	{
		return m_duration;
	}
	void SetCurves( void );


protected:
	bool m_cycle;
	granny_real32 m_duration;
	std::string m_name;
	std::string m_group;
	std::string m_grannyResPath;
	TriGrannyResPtr m_grannyRes;
};
TYPEDEF_BLUECLASS( Tr2GrannyTrack );

#endif //Tr2GrannyTrack_h
