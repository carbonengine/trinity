#pragma once
#ifndef EveUpdateContext_h
#define EveUpdateContext_h
#include "Include/IEveBallpark.h"
#include "Tr2GPUParticlePoolManager.h"
#include "Vector3d.h"

BLUE_DECLARE( Tr2DataTextureManager );

static const double UNINITIALIZED_ORIGIN = std::numeric_limits<double>::infinity();

class EveUpdateContext
{
public:
	EveUpdateContext() {}
	EveUpdateContext( Be::Time time ) : 
		m_lastTime( 0 ),
		m_currentTime( 0 ),
		m_origin( UNINITIALIZED_ORIGIN, UNINITIALIZED_ORIGIN, UNINITIALIZED_ORIGIN ),
		m_originShift( 0, 0, 0 )
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
	Tr2DataTextureManagerPtr GetDataTextureManager() const
	{
		return m_dataTextureManager;
	}
	void SetDataTextureManager( Tr2DataTextureManagerPtr manager )
	{
		m_dataTextureManager = manager;
	}
	Tr2GPUParticlePoolManager* GetParticlePoolManager()
	{
		return m_gpuParticleManager;
	}
	void SetParticlePoolManager( Tr2GPUParticlePoolManager* manager )
	{
		m_gpuParticleManager = manager;
	}
	
	// World origin change
	void UpdateOrigin( IEveBallpark* ballpark )
	{
		Vector3d originNow;
		IEveReferencePointPtr refObject( ballpark );
		if( refObject )
		{
			refObject->GetReferencePoint( &originNow, m_currentTime );
			if( m_origin.x != UNINITIALIZED_ORIGIN )
			{
				m_origin = m_origin - originNow;
				m_originShift = m_origin.AsVector3();
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

private:
	Be::Time m_currentTime;
	Be::Time m_lastTime;

	// extra stuff
	Tr2GPUParticlePoolManagerPtr m_gpuParticleManager;
	Tr2DataTextureManagerPtr m_dataTextureManager;

	// For tracking world origin
	Vector3d m_origin;
	Vector3 m_originShift;
};

#endif //EveUpdateContext_h
