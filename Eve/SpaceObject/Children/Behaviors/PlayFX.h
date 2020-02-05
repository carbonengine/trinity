#pragma once
#ifndef PlayFX_H
#define PlayFX_H

#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"
#include "ITr2Renderable.h"

struct PlayFXData
{
	PlayFXData() :
		effectPlaying( false ),
		seconds( 10 ),
		oldTarget( 0, 0, 0 )
	{
	}

	bool effectPlaying;
	int seconds;
	Vector3 oldTarget;
};

BLUE_DECLARE_INTERFACE( IEveFiringEffectElement );
BLUE_DECLARE_IVECTOR( IEveFiringEffectElement );

BLUE_CLASS( PlayFX ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	PlayFX( IRoot* lockobj = nullptr );
	~PlayFX();

	/////////////////////////////////////////////////////////////////////////////////////
	// PlayFX
	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( const DroneAgent& drone, void* scratchMemory );
	virtual std::vector<Vector3> CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
		BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );
	std::string GetBehaviorName();
	int GetProcessPriority();

	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	void Update( EveUpdateContext& updateContext, const TriFrustum & frustum, const Matrix & parentTransform );

	void UpdateState( bool state ) { m_stop = state; }

private:
	void ClearEffects();
	void CheckCount( size_t agentSize );

	size_t m_count;
	float m_behaviorWeight;
	float m_delay;
	int m_minSec;
	int m_maxSec;
	bool m_stop;

	IEveFiringEffectElementPtr m_firingEffect;
	PIEveFiringEffectElementVector m_firingEffects;

	std::vector<Vector3> m_todo;
};

TYPEDEF_BLUECLASS( PlayFX );

#endif