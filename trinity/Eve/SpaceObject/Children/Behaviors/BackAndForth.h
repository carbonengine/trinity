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
		locatorIndex( -1 ),
		seek( true ),
		deliver( false ),
		arrived( true ),
		timePassed( 0.f )
	{}

	Vector3 locatorTarget;
	Vector3 locatorDirection;
	int locatorIndex;
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

	enum LocatorType
	{
		LOCAL_LOCATORS = 0,
		PARENT_LOCATORS = 1,
		TARGET_LOCATORS = 2,
	};

	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( void* scratchMemory );

	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation);
	int GetProcessPriority();
	std::string GetBehaviorName();

	void SetParent( IEveSpaceObject2 *parent );

private:
	bool m_enabled;
	int32_t m_priority;
	int m_rand;
	float m_arrivedRadius;
	float m_slowDownRadius;
	float m_backAndForthWeight;
	float m_seconds;
	float m_distFromOrigin;

	Vector3 m_arrivalPoint; //debugging

	IBehavior* m_fxBehavior;

	EveSpaceObject2* m_target;
	EveSpaceObject2* m_parent;

	//Locators
	const LocatorStructureList* GetLocatorsForSet( const BlueSharedString& setName ) const;
	PEveLocatorSetsVector m_locatorSets;
	BlueSharedString m_locatorSetName;
	void AddLocatorSet();
	LocatorType m_locatorType;
	void GetParentLocatorPosition( int locatorIndex, Vector3* locatorPosition, Vector3* locatorDirection );
	void GetTargetLocatorPosition( int locatorIndex, Vector3* locatorPosition, Vector3* locatorDirection );
};

TYPEDEF_BLUECLASS( BackAndForth );

#endif