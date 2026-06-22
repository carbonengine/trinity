// Copyright © 2023 CCP ehf.

#pragma once
#ifndef IBehavior_h
#define IBehavior_h

BLUE_INTERFACE( IBehavior ) :
	public IRoot
{
public:
	enum TunnelGroupType
	{
		EXIT_TUNNELS = 0,
		ENTRANCE_TUNNELS = 1,
		OTHER_TUNNELS = 2,
	};

	enum ProcessPriority
	{
		// Remember to change the enum chooser in Allign_Blue.cpp if this one changes
		LEAST_PRIORITY = 0, // This should be the default and is for all behavior-decisions the 'ship's pilot' is making
		LESS_PRIORITY = 1, // other effects that are affected by or effecting the first group
		MORE_PRIORITY = 3, // This group is for behaviors that want to override the earlier categories and directly control movement (animations etc)
		MOST_PRIORITY = 4, // things that should have more priority than direct control (ships pushing each other away, wind, or similar effects)

		COUNT,
	};

	virtual size_t GetScratchMemorySize() const
	{
		return 0;
	}

	virtual void InitializeScratch( void* scratchMemory )
	{
	}

	virtual int GetProcessPriority()
	{
		return LEAST_PRIORITY;
	}

	virtual std::string GetBehaviorName()
	{
		return "unnamed";
	}

	// Used in Formation
	virtual void Reset()
	{
	}

	// This function should apply a force to the acceleration and return an array with pos and force vector for each agent
	virtual std::vector<Vector3> CalculateBehavior( std::vector<DroneAgent> & agents, void* scratchData, const float deltaTime, BehaviorGroup& sys, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius ) = 0;

	// Debug renderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions & options )
	{
	}
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer, std::vector<DroneAgent> & agents, Matrix & parentWorldLocation )
	{
	}

	// this is for Groups to do all range detections at the same time. ( return -1 if you don't care about other agents )
	// this function could utilize deltaTime but it's probably a good thing that it updates less frequently when the system is tanking
	virtual float GetBehaviorSearchRadius()
	{
		return -1;
	}

	// Can be used in behaviors to set state of another behavior, e.g. set to active/inactive or behaviorWeight
	virtual void UpdateState( bool state )
	{
	}
};

#endif
