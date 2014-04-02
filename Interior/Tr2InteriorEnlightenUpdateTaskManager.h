#pragma once
#ifndef Tr2IntEnlightenTaskManager_H
#define Tr2IntEnlightenTaskManager_H

#include "include/ITr2Updateable.h"

BLUE_DECLARE( Tr2InteriorCell );
BLUE_DECLARE( Tr2InteriorEnlightenSystem );
BLUE_DECLARE( Tr2IntEnlightenTaskManager );

namespace GeoEngine
{
	class NonAAVolume;
}

// A class encapsulates multithreading tasks
// for Enlighten update
class Tr2IntEnlightenTaskManager :
	public ITr2Updateable
{
public:
	Tr2IntEnlightenTaskManager( IRoot* lockobj = 0 );
	~Tr2IntEnlightenTaskManager();

	EXPOSE_TO_BLUE();

	// -------------------------------------------------------------
	// Description:
	//   TaskInfo is used as wrapper for Tr2IntEnlightenTaskManager
	//   pointer. The task manager maintains a list of all TaskInfo
	//   objects that reference it and can inform them when the 
	//   manager is about to destruct.
	//   TaskInfo also contains helper methods that just call
	//   corresponding referenced task manager methods.
	// -------------------------------------------------------------
	class TaskInfo
	{
	public:
		TaskInfo();
		~TaskInfo();
		TaskInfo( const TaskInfo& info );
		TaskInfo& operator=( const TaskInfo& info );

		void SetTaskManager( Tr2IntEnlightenTaskManager* taskManager );
		bool IsExecuting() const;
		void WaitToComplete();
		void InvalidateResult();
	private:
		Tr2IntEnlightenTaskManager* m_taskManager;
		friend class Tr2IntEnlightenTaskManager;
	};

	// Add Enlighten system to be solved
	void AddSystem( Tr2InteriorEnlightenSystem* system );

	static const unsigned UninitializedIndex = -1;

	// Adds SH sample volume information
	void AddSHVolume( Tr2InteriorCell* cell, const GeoEngine::NonAAVolume* volume, const Enlighten::RadProbeSetCore* core, const Enlighten::InputLightingBuffer** buffer );
	// Sets lighting data for a cell for computing SH probes
	void SetCellInputLighting( Tr2InteriorCell* cell, std::vector<const Enlighten::InputLightingBuffer*>& inputLighting, const Enlighten::EmissiveEnvironment* environment );
	// Adds a sample point for computing SH coefficients
	unsigned int AddSHProbeSample( unsigned int sampleIndex, Tr2InteriorCell* cell, const Vector3& position );
	// Get results of computing an SH probe
	const Vector3* GetSHProbeResult( unsigned int sampleIndex );


	// Execute tasks for each used System
    void Execute();
	// Invalidates result of previous computation, unlocks cells and Systems for each used System record
	void InvalidateResult();
	// Returns true iff some tasks are still executing
	bool IsExecuting() const;
	// Waits for completion of all tasks
	void WaitToComplete();
	// Fetch results of Enlighten update if they are ready
	void Update( Be::Time time );
private:
	void UnlockCells();

	typedef std::set<Tr2InteriorEnlightenSystem*> Systems;
	// Systems that need to be solved
	Systems m_systems;
	// Flag indicating that Enlighten systems and cells we locked
	bool m_cellsLocked;

	struct CellData;
	typedef std::map<Tr2InteriorCell*, CellData> CellMap;
	// Per-cell data needed for computing SH coefficients
	CellMap m_cellData;

	struct SHSample;
	typedef std::vector<SHSample> SamplePoints;
	// Per-SH sample data for computing SH coefficients
	SamplePoints m_samplePoints;

	// A set of TaskInfo objects referencing this manager
	std::set<TaskInfo*> m_infoObjects;

	// The root task
	tbb::empty_task* m_root;

	class LightingUpdateTask;
	class SystemUpdateTask;
	class UpdateTask;
};

TYPEDEF_BLUECLASS( Tr2IntEnlightenTaskManager );


#endif // Tr2IntEnlightenTaskManager_H