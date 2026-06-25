// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriCurveSet_h
#define TriCurveSet_h


#include "include/ITr2Updateable.h"
#include "include/ITr2ValueBinding.h"

BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( TriValueBinding );
BLUE_DECLARE_INTERFACE( ITriFunction );
BLUE_DECLARE_IVECTOR( ITriFunction );
BLUE_DECLARE_IVECTOR( ITr2ValueBinding );
BLUE_DECLARE_INTERFACE( ICurveSetDriver );


BLUE_CLASS( Tr2CurveSetRange ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2CurveSetRange( IRoot* lockobj = nullptr );

	std::string m_name;

	float m_startTime;
	float m_endTime;
	bool m_looped;
};

TYPEDEF_BLUECLASS( Tr2CurveSetRange );

BLUE_DECLARE_VECTOR( Tr2CurveSetRange );


BLUE_CLASS( TriCurveSet ) :
	public IInitialize,
	public ITr2Updateable,
	public ISimTimeRebaseNotify
{
public:
	EXPOSE_TO_BLUE();
	TriCurveSet( IRoot* lockobj = NULL );
	~TriCurveSet();

	void Update( double time );
	void Update( Be::Time realTime, Be::Time simTime );

	void Play();
	void PlayTimeRange( const char* name );
	void Stop();
	void StopOnNextFrame();
	void StopAfter( double seconds );
	void StopAfterWithCallback( double seconds, const BlueScriptCallback& cb );

	void PlayFrom( double time );

	bool IsPlaying() const;

	void Apply();
	void ApplyTime( double time );

	// access the name
	const std::string& GetName() const
	{
		return m_name;
	}
	void SetName( const std::string& name );

	// access the curves
	size_t GetCurvesCount() const
	{
		return m_curves.size();
	}
	ITriFunctionPtr GetCurve( unsigned int _id )
	{
		return m_curves[_id];
	}
	void AddCurve( ITriFunctionPtr curve );

	// access the bindings
	size_t GetBindingsCount() const
	{
		return m_bindings.size();
	}
	ITr2ValueBindingPtr GetBinding( unsigned int _id )
	{
		return m_bindings[_id];
	}
	void AddBinding( ITr2ValueBindingPtr binding );

	// Gets the duration of the longest non-cycling curve in the curve set
	float GetMaxCurveDuration() const;
	float GetRangeDuration( const char* rangeName ) const;
	float GetTimeScale() const;
	double GetScaledTime() const;

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// ISimTimeRebaseNotify
	void OnSimClockRebase( Be::Time oldTime, Be::Time newTime );

	void SetTimeRange( double timeMin, double timeMax, Be::OptionalWithDefaultValue<bool, true> looped = true );
	void ResetTimeRange();
	bool HasTimeRange() const;
	std::pair<double, double> GetTimeRange() const;

private:
	void UpdateWithCurrentTime();

	std::string m_name;
	PITr2ValueBindingVector m_bindings;
	PITriFunctionVector m_curves;
	PTr2CurveSetRangeVector m_ranges;
	ICurveSetDriverPtr m_driver;

	bool m_isPlaying;
	bool m_stopOnNextFrame;
	bool m_playOnLoad;
	bool m_useSimTimeRebase;
	bool m_isUsingSimTimeRebase;
	bool m_useRealTime;
	bool m_hasTimeRange;
	bool m_loopedTimeRange;

	double m_startTime;
	double m_lastTime;
	double m_endTime;

	double m_scaledTime;

	float m_scale;

	double m_timeRangeMin;
	double m_timeRangeMax;

	BlueScriptCallback m_callback;
};

TYPEDEF_BLUECLASS( TriCurveSet );

#endif //TriCurveSet_h