// Copyright © 2023 CCP ehf.

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
		droneArrived( false ),
		oldTarget( 0, 0, 0 )
	{
	}

	bool effectPlaying;
	bool droneArrived;
	Vector3 oldTarget;
};

BLUE_DECLARE_INTERFACE( IEveFiringEffectElement );
BLUE_DECLARE_IVECTOR( IEveFiringEffectElement );

BLUE_CLASS( PlayFX ) :
	public IBehavior,
	public IListNotify,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();
	PlayFX( IRoot* lockobj = nullptr );
	~PlayFX();

	/////////////////////////////////////////////////////////////////////////////////////
	// PlayFX
	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( void* scratchMemory );
	virtual std::vector<Vector3> CalculateBehavior( std::vector<DroneAgent> & agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius );
	std::string GetBehaviorName();
	int GetProcessPriority();

	void GetRenderables( std::vector<ITr2Renderable*> & renderables );

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	void UpdateAsyncronous( const EveUpdateContext& updateContext, const Matrix& parentTransform );
	void UpdateSyncronous( const EveUpdateContext& updateContext );
	void UpdateState( bool state )
	{
		m_stop = state;
	}
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const;

	void OnListModified(
		long event, // BLUELISTEVENT values
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const struct IList* theList ) override;

private:
	void CheckCount( size_t agentSize );

	bool m_enabled;
	size_t m_count;
	float m_behaviorWeight;
	float m_distanceFromCenter;
	int32_t m_sec;
	int32_t m_priority;
	bool m_stop;

	IEveFiringEffectElementPtr m_firingEffect;
	PIEveFiringEffectElementVector m_firingEffects;

	std::vector<Vector3> m_todo;
};

TYPEDEF_BLUECLASS( PlayFX );

#endif