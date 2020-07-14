#pragma once
#ifndef BackAndForth_H
#define BackAndForth_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"
#include "Eve/SpaceObject/Utils/EveLocatorSets.h"

struct BackAndForthData
{
	BackAndForthData() :
		locatorTarget( 0, 0, 0 ),
		locatorDirection( 0, 0, 0 ),
		seek( true ),
		deliver( false ),
		arrived( true ),
		timePassed( 0.f )
	{}

	Vector3 locatorTarget;
	Vector3 locatorDirection;
	bool seek;
	bool deliver;
	bool arrived;
	float timePassed;
};

BLUE_CLASS( BackAndForth ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	BackAndForth( IRoot* lockobj = nullptr );
	~BackAndForth();

	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( void* scratchMemory );

	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation);
	int GetProcessPriority();

private:
	float m_arrivedRadius;
	float m_slowDownRadius;
	int m_rand;
	float m_backAndForthWeight;
	float m_seconds;

	IBehavior* m_fxBehavior;

	//Locators
	const LocatorStructureList* GetLocatorsForSet( const BlueSharedString& setName ) const;
	PEveLocatorSetsVector m_locatorSets;
	void AddLocatorSet();
};

TYPEDEF_BLUECLASS( BackAndForth );

#endif