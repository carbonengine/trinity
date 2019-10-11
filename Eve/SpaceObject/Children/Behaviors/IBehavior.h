#pragma once
#ifndef IBehavior_h
#define IBehavior_h

BLUE_INTERFACE( IBehavior ): public IRoot
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
		PROCESS_FIRST = 0,	// This should be the default and is for all behavior-decisions the 'ship's pilot' is making
		PROCESS_NEXT = 1,	// other effects that are affected by or effecting the first group
		PROCESS_INERTIA = 2, // Inertia
		PROCESS_LATER = 3,	// This group is for behaviors that want to override the earlier categories and directly control movement (animations etc)
		PROCESS_LAST = 4,	// things that should have more priority than direct control (ships pushing each other away, wind, or similar effects)
	};

	virtual size_t GetScratchMemorySize() const // per-agent
	{
		return 0;
	}

	virtual void InitializeScratch( const DroneAgent& drone, void* scratchMemory )
	{
	}

	virtual int GetProcessPriority()
	{
		return PROCESS_FIRST;
	}

	// This function should apply a force to the acceleration and return an array with pos and force vector for each agent
	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& sys, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius) = 0;
	
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation ) = 0;
	
	// this is for Groups to do all range detections at the same time. ( return -1 if you don't care about other agents ) 
	// this function could utilize deltaTime but it's probably a good thing that it updates less frequently when the system is tanking
	virtual float GetBehaviorSearchRadius()
	{
		return -1;
	}

	
};

#endif
