#pragma once
#ifndef EveUpdateContext_h
#define EveUpdateContext_h
#include "Include/IEveBallpark.h"
#include "Utilities/Vector3d.h"
#include "../TbbStub.h"

BLUE_DECLARE( Tr2DataTextureManager );

static const double UNINITIALIZED_ORIGIN = std::numeric_limits<double>::infinity();

BLUE_DECLARE( Tr2GpuParticleSystem );

class EveUpdateContext
{
public:
	EveUpdateContext() {}
	EveUpdateContext( Be::Time time ) : 
		m_lastTime( 0 ),
		m_currentTime( 0 ),
		m_origin( UNINITIALIZED_ORIGIN, UNINITIALIZED_ORIGIN, UNINITIALIZED_ORIGIN ),
		m_originShift( 0, 0, 0 ),
		m_originShiftRemainder( 0, 0, 0 )
	{
		SetTime( time );
	}

	// Base attribute time
	const Be::Time GetTime() const
	{
		return m_currentTime;
	}
	void SetTime( Be::Time time )
	{
		m_lastTime = m_currentTime;
		m_currentTime = time;
	}

	// get deltaT to last
	float GetDeltaT() const
	{
		// get usefull time info
		float deltaT = 0.f;
		if( m_lastTime != 0 )
			deltaT = TimeAsFloat( m_currentTime - m_lastTime );
		return deltaT;
	}

	// Any extra objects you would like to pass along
	Tr2DataTextureManager* GetDataTextureManager() const
	{
		return m_dataTextureManager;
	}
	void SetDataTextureManager( Tr2DataTextureManager* manager )
	{
		m_dataTextureManager = manager;
	}

	Tr2GpuParticleSystem* GetGpuParticleSystem() const
	{
		return m_gpuParticleSystem;
	}
	void SetGpuParticleSystem( Tr2GpuParticleSystem* ps )
	{
		m_gpuParticleSystem = ps;
	}

	IEveBallpark* GetBallpark() const
	{
		return m_ballpark;
	}

	// World origin change
	void UpdateOrigin( IEveBallpark* ballpark )
	{
		m_ballpark = ballpark;
		Vector3d originNow;
		IEveReferencePointPtr refObject( ballpark );
		if( refObject )
		{
			refObject->GetReferencePoint( &originNow, m_currentTime );
			if( m_origin.x != UNINITIALIZED_ORIGIN )
			{
				Vector3d originDelta = originNow - m_origin + m_originShiftRemainder;
				Vector3 originDeltaF = originDelta.AsVector3();
				m_originShiftRemainder = originDelta - Vector3d( originDeltaF );
				m_originShift = -originDeltaF;
			}
			m_origin = originNow;
		}
	}
	Vector3 GetOriginShift() const
	{
		return m_originShift;
	}
	Vector3d GetOrigin() const
	{
		if( m_origin.x != UNINITIALIZED_ORIGIN )
		{
			return m_origin;
		}
		return Vector3d( 0, 0, 0 );
	}

	void SetTaskGroup( Tr2ParallelTaskGroup* taskGroup )
	{
		m_taskGroup = taskGroup;
	}

	Tr2ParallelTaskGroup* GetTaskGroup() const
	{
		return m_taskGroup;
	}

private:
	Be::Time m_currentTime;
	Be::Time m_lastTime;

	// extra stuff
	Tr2DataTextureManagerPtr m_dataTextureManager;
	Tr2GpuParticleSystemPtr m_gpuParticleSystem;
	IEveBallparkPtr m_ballpark;

	// For tracking world origin
	Vector3d m_origin;
	Vector3 m_originShift;
	Vector3d m_originShiftRemainder;
	Tr2ParallelTaskGroup* m_taskGroup = nullptr;
};

#endif //EveUpdateContext_h
