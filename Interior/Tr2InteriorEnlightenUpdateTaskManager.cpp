#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorEnlightenUpdateTaskManager.h"
#include "Tr2InteriorCell.h"
#include "Tr2InteriorEnlightenSystem.h"
#include "TriDevice.h"

// --------------------------------------------------------------------------------------
// Description:
//   Per-cell data needed to compute SH probe coefficients. Contains an array of SH
//   sample volume data and emissive environment values.
// --------------------------------------------------------------------------------------
struct Tr2IntEnlightenTaskManager::CellData
{
	struct Volume
	{
		// Geometrical volume for SH volume
		const GeoEngine::NonAAVolume* volume;
		// Enlighten core object
		const Enlighten::RadProbeSetCore* core;
		// Array of input lighting buffers
		const Enlighten::InputLightingBuffer** inputLighting;
	};
	typedef std::vector<Volume> Volumes;

	CellData()
		:environment( NULL ) 
	{
	}

	// SH sample volumes
	Volumes volumes;
	// Emissive environment
	const Enlighten::EmissiveEnvironment* environment;
};

// --------------------------------------------------------------------------------------
// Description:
//   Contains data for computing SH lighting sample.
// --------------------------------------------------------------------------------------
struct Tr2IntEnlightenTaskManager::SHSample
{
	struct CellData
	{
		Tr2InteriorCell* cell;
		Vector3 minBounds;
		Vector3 maxBounds;
		Matrix transform;
	};
	// Cell for which the sample is computed
	std::vector<CellData> cells;
	// Sample position (in cell space)
	Vector3 position;
	// Resulting coefficients
	Vector3 coefficients[9];
	// Valid result
	bool valid;
};

// --------------------------------------------------------------------------------------
// Description:
//   TBB task for updating input lighting for a single Enlighten system.
// --------------------------------------------------------------------------------------
class Tr2IntEnlightenTaskManager::LightingUpdateTask: public tbb::task
{
public:
	LightingUpdateTask(Tr2InteriorEnlightenSystem* system)
		:m_system(system)
	{
	}

	tbb::task* execute()
	{
		m_system->UpdateEnlightenLightingThreaded();
		return 0;
	}
private:
	Tr2InteriorEnlightenSystem* m_system;
};

// --------------------------------------------------------------------------------------
// Description:
//   TBB task for updating radiosity for a single Enlighten system.
// --------------------------------------------------------------------------------------
class Tr2IntEnlightenTaskManager::SystemUpdateTask: public tbb::task
{
public:
	SystemUpdateTask(Tr2InteriorEnlightenSystem* system)
		:m_system(system)
	{
	}

	tbb::task* execute()
	{
		m_system->UpdateEnlightenSolutionThreaded();
		return 0;
	}
private:
	Tr2InteriorEnlightenSystem* m_system;
};

// --------------------------------------------------------------------------------------
// Description:
//   TBB task for updating radiosity solutions for all systems and then computing SH
//   lighting for all sampling points.
// --------------------------------------------------------------------------------------
class Tr2IntEnlightenTaskManager::UpdateTask: public tbb::task
{
public:
	UpdateTask(Tr2IntEnlightenTaskManager &manager)
		:m_manager( manager )
	{
	}

	tbb::task* execute() 
	{
		// Spawn tasks for updating lighting buffers
		tbb::task_list lightingList;
		int count = 1;

		for( Systems::iterator it = m_manager.m_systems.begin(); it != m_manager.m_systems.end(); ++it )
		{
			lightingList.push_back( *new( allocate_child() ) LightingUpdateTask( *it ) );
			++count;
		}
		set_ref_count( count );

		spawn_and_wait_for_all( lightingList );

		// Spawn tasks for updating radiosity
		tbb::task_list list;
		for( Systems::iterator it = m_manager.m_systems.begin(); it != m_manager.m_systems.end(); ++it )
		{
			list.push_back( *new( allocate_child() ) SystemUpdateTask( *it ) );
		}
		set_ref_count( count );

		spawn_and_wait_for_all( list );

		unsigned int controlPrecision;
		unsigned int controlDenormalize;
		_controlfp_s( &controlPrecision, _PC_24, _MCW_PC );
		_controlfp_s( &controlDenormalize, _DN_FLUSH, _MCW_DN );

		for( SamplePoints::iterator it = m_manager.m_samplePoints.begin(); it != m_manager.m_samplePoints.end(); ++it )
		{
			if( it->cells.size() == 1 )
			{
				CalculateSHCoefficients( m_manager.m_cellData[it->cells[0].cell], *it );
				it->valid = true;
			}
			else
			{
				// If the sample point is in several cells, interpolate lighting based on the distance
				// to cell boundaries.
				Vector3 coefficients[9];
				for( int i = 0; i < 9; ++i )
				{
					coefficients[i] = Vector3( 0.0f, 0.0f, 0.0f );
				}
				float totalDistance = 0.f;
				for( std::vector<SHSample::CellData>::iterator data = it->cells.begin(); data != it->cells.end(); ++data )
				{
					if( CalculateSHCoefficients( m_manager.m_cellData[data->cell], *it ) )
					{
						float distance = GetCellDistance( *data, it->position );
						totalDistance += distance;
						for( int i = 0; i < 9; ++i )
						{
							coefficients[i] += it->coefficients[i] * distance;
						}
					}
				}
				if( totalDistance > 0.0f )
				{
					for( int i = 0; i < 9; ++i )
					{
						it->coefficients[i] = coefficients[i] / totalDistance;
					}
					it->valid = true;
				}
				else
				{
					it->valid = false;
				}
			}
		}

		_controlfp_s( &controlDenormalize, controlDenormalize, _MCW_PC | _MCW_DN );

		return 0;
	}
private:
	// --------------------------------------------------------------------------------------
	// Description:
	//   Calculates distance (signed) from given point to cell bounds.
	// Arguments:
	//   cellData - Cell data (bounding box and transform).
	//   point - Sample position.
	// --------------------------------------------------------------------------------------
	float GetCellDistance( const SHSample::CellData& cellData, const Vector3& point ) const
	{
		Vector3 localPoint;
		Matrix invTransform;
		D3DXMatrixInverse( &invTransform, NULL, &cellData.transform );
		D3DXVec3TransformCoord( &localPoint, &point, &invTransform );
		float distance;
		distance = localPoint.x - cellData.minBounds.x;
		distance = min( distance, localPoint.y - cellData.minBounds.y );
		distance = min( distance, localPoint.z - cellData.minBounds.z );
		distance = min( distance, cellData.maxBounds.x - localPoint.x );
		distance = min( distance, cellData.maxBounds.y - localPoint.y );
		distance = min( distance, cellData.maxBounds.z - localPoint.z );
		return max( distance, 0.0f );
	}
	// --------------------------------------------------------------------------------------
	// Description:
	//   Calculates SH lighting coefficients for the given sample position.
	// Arguments:
	//   data - Cell lighting data.
	//   point - Sample position.
	// --------------------------------------------------------------------------------------
	bool CalculateSHCoefficients( const CellData &data, SHSample& point )
	{
		float totalWeight = 0.0f;
		for( int i = 0; i < 9; ++i )
		{
			point.coefficients[i] = Vector3( 0.0f, 0.0f, 0.0f );
		}

		Vector3 coeff[9];
		CellData::Volumes::const_iterator it = data.volumes.begin();
		++it;
		for ( ; it != data.volumes.end(); ++it)
		{
			float weight = GetMixWeight( it->volume, point.position );
			if (weight > 0)
			{
				if( GetLightingCoefficients( *it, data, point.position, coeff ) )
				{
					for( int i = 0; i < 9; ++i )
					{
						point.coefficients[i] += coeff[i] * weight;
					}
					totalWeight += weight;
				}
			}
		}
		if (totalWeight > 1.0f)
		{
			for( int i = 0; i < 9; ++i )
			{
				point.coefficients[i] /= totalWeight;
			}
		}
		else if (totalWeight < 1.0f)
		{
			if( GetLightingCoefficients( data.volumes[0], data, point.position, coeff ) )
			{
				for( int i = 0; i < 9; ++i )
				{
					point.coefficients[i] += coeff[i] * (1.0f - totalWeight);
				}
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	// --------------------------------------------------------------------------------------
	// Description:
	//   Returns a weight value for a given volume in the mix of all volumes for the computed
	//   sample position.
	// Arguments:
	//   volume - Volume for which the weight value is requested.
	//   position - Sample position.
	// Return Value:
	//   Weight that this volume has in final mix of SH coefficients.
	// --------------------------------------------------------------------------------------
	float GetMixWeight( const GeoEngine::NonAAVolume* volume, const Vector3& position ) const
	{
		Geo::v128 pos;
		pos = Geo::VConstruct( position.x, position.y, position.z, 1.0f );
		Geo::v128 localPos = volume->MapToLocalUnitCube( pos );
		Vector3 localPosition( Geo::VGetX(localPos), Geo::VGetY(localPos), Geo::VGetZ(localPos) );

		float mixWeight = 0.0f;

		if (localPosition.x < 0.0f)
		{
			mixWeight = std::max( mixWeight, -localPosition.x );
		}
		else if (localPosition.x > 1.0f)
		{
			mixWeight = std::max( mixWeight, localPosition.x - 1 );
		}
		if (localPosition.y < 0.0f)
		{
			mixWeight = std::max( mixWeight, -localPosition.y );
		}
		else if (localPosition.y > 1.0f)
		{
			mixWeight = std::max( mixWeight, localPosition.y - 1 );
		}
		if (localPosition.z < 0.0f)
		{
			mixWeight = std::max( mixWeight, -localPosition.z );
		}
		else if (localPosition.z > 1.0f)
		{
			mixWeight = std::max( mixWeight, localPosition.z - 1 );
		}

		mixWeight *= std::max( std::max( volume->GetXRes(), volume->GetYRes() ), volume->GetZRes() );

		mixWeight = 1.0f - std::min( mixWeight, 1.0f );

		return mixWeight;
	}
	// --------------------------------------------------------------------------------------
	// Description:
	//   Computes SH coefficients for SH volume points around the sample point and 
	//   performs linear interpolation to give final SH coefficients for the given SH
	//   sample volume.
	// Arguments:
	//   volume - Volume to use for computing SH coefficients.
	//   data - Cell lighting data.
	//   position - Sample position.
	//   lcoeff - Resulting SH coefficients (expects 9-element array).
	// Return value:
	//	 true - If valid coefficients were calculated
	//	 false - If all probes were culled
	// --------------------------------------------------------------------------------------
	bool GetLightingCoefficients( const CellData::Volume& volume, const CellData &data, const Vector3& position, Vector3* lcoeff )
	{
		__declspec(align(32)) Geo::s32 indicesToSolve[GeoEngine::VOL_INTERP_TRI];
		__declspec(align(32)) float result[GeoEngine::VOL_INTERP_TRI][Enlighten::SH_ORDER_L2 * 3];
		__declspec(align(32)) float* outputPointers[GeoEngine::VOL_INTERP_TRI] = {
			result[0],
			result[1],
			result[2],
			result[3],
			result[4],
			result[5],
			result[6],
			result[7],
		};

		Enlighten::RadProbeTask task;
		task.m_CoreProbeSet = volume.core;
		task.m_InputLighting = volume.inputLighting;
		task.m_Environment = data.environment;
		task.m_NumIndicesToSolve = GeoEngine::VOL_INTERP_TRI;
		task.m_IndicesToSolve = indicesToSolve;
		task.m_OutputPointers = outputPointers;

		// Find the closest sample		
		float weights[GeoEngine::VOL_INTERP_TRI];
		Geo::v128 point = Geo::VConstruct( position.x, position.y, position.z, 1.0f );
		if( !volume.volume->GetNearestSamples( indicesToSolve, weights, point, GeoEngine::VOL_INTERP_TRI ) )
		{
			return false;
		}

		// Run the task
		Geo::u32 probeTaskSolveTime;
		Enlighten::SolveProbeTaskL2( &task, probeTaskSolveTime );

		for( int i = 0; i < 9; ++i )
		{
			lcoeff[i] = Vector3( 0.0f, 0.0f, 0.0f );
		}

		// Zero-out culled probe weights
		float totalWeight = 1.0f;
		unsigned culledCount = 0;
		for( Geo::s32 j = 0; j < GeoEngine::VOL_INTERP_TRI; ++j )
		{
			if( Enlighten::IsProbeCulled( indicesToSolve[j], volume.core ) )
			{
				totalWeight -= weights[j];
				weights[j] = 0.0f;
				++culledCount;
			}
		}

		if( culledCount == GeoEngine::VOL_INTERP_TRI || totalWeight <= 0.00001f )
		{
			return false;
		}

		if( totalWeight < 1.0f )
		{
			for( Geo::s32 j = 0; j < GeoEngine::VOL_INTERP_TRI; ++j )
			{
				weights[j] /= totalWeight;
			}
		}

		// interpolate the final value
		for( Geo::s32 j = 0; j < GeoEngine::VOL_INTERP_TRI; ++j )
		{
			float* out = outputPointers[j];
			float weight = weights[j];

			for( int c = 0; c < 3; ++c )
			{
				for( int i = 0; i < 9; ++i )
				{
					float v = weight * (*out++);

					if( c == 0 )
					{
						lcoeff[i].x += v;
					}
					else if( c == 1 )
					{
						lcoeff[i].y += v;
					}
					else
					{
						lcoeff[i].z += v;
					}
				}
			}
		}
		return true;
	}

	Tr2IntEnlightenTaskManager &m_manager;
};

// ------------------------------------------------------------------------------------------------------
Tr2IntEnlightenTaskManager::Tr2IntEnlightenTaskManager( IRoot* lockobj )
:	m_cellsLocked( false )
{
	// Allocate TBB root task (TBB way of allocating)
	m_root = new( tbb::task::allocate_root() ) tbb::empty_task;
	m_root->set_ref_count( 1 );

	TriDevice::RegisterForUpdates( this );
}

// ------------------------------------------------------------------------------------------------------
Tr2IntEnlightenTaskManager::~Tr2IntEnlightenTaskManager()
{
	TriDevice::UnregisterForUpdates( this );

	m_root->wait_for_all();
	m_root->destroy( *m_root );

	for( std::set<TaskInfo*>::iterator it = m_infoObjects.begin(); it != m_infoObjects.end(); ++it )
	{
		(*it)->m_taskManager = 0;
	}

	if( m_cellsLocked )
	{
		for( Systems::iterator it = m_systems.begin(); it != m_systems.end(); ++it )
		{
			// Unlock locked system
			(*it)->Unlock();
		}
		for( CellMap::iterator it = m_cellData.begin(); it != m_cellData.end(); ++it )
		{
			// Unlock locked cell
			it->first->Unlock();
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2IntEnlightenTaskManager::AddSystem( Tr2InteriorEnlightenSystem* system )
{
	m_systems.insert( system );
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds a SH probe volume to use for computing SH lighting. 
// Arguments:
//   cell - Cell that contains this volume
//   volume - Geometrical volume representation
//   core - Enlighten core object
//   buffer - Array of Enlighten input buffers
// --------------------------------------------------------------------------------------
void Tr2IntEnlightenTaskManager::AddSHVolume( Tr2InteriorCell* cell, 
											  const GeoEngine::NonAAVolume* volume, 
											  const Enlighten::RadProbeSetCore* core, 
											  const Enlighten::InputLightingBuffer** buffer )
{
	CellData &cellData = m_cellData[cell];
	CellData::Volume newVolume;
	newVolume.volume = volume;
	newVolume.core = core;
	newVolume.inputLighting = buffer;
	cellData.volumes.push_back( newVolume );
}

// --------------------------------------------------------------------------------------
// Description:
//   Provide the manager with lighting data for a cell needed to perform SH lighting
//   computations. This function should be called after all SH probe volumes where
//   added to the manager with Tr2IntEnlightenTaskManager::AddSHVolume function.
// Arguments:
//   cell - Cell for which lighting data is provided
//   inputLighting - Vector of input lighting buffers (gathered from cell's systems and
//                   neighbors)
//   environment - Enlighten emissive environment
// --------------------------------------------------------------------------------------
void Tr2IntEnlightenTaskManager::SetCellInputLighting( Tr2InteriorCell* cell, 
													   std::vector<const Enlighten::InputLightingBuffer*>& inputLighting, 
													   const Enlighten::EmissiveEnvironment* environment )
{
	CellData &cellData = m_cellData[cell];
	cellData.environment = environment;

	for( CellData::Volumes::iterator it = cellData.volumes.begin(); it != cellData.volumes.end(); ++it )
	{
		Enlighten::PrepareInputLightingList(
			it->core,
			&inputLighting[0],			// Input
			(Geo::s32)inputLighting.size(),
			it->inputLighting );

	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds a point to compute SH lighting for.
// Arguments:
//	 sampleIndex - Existing index for sample or UninitializedIndex for new sample
//   cell - Cell that contains this point
//   position - Sample position in cell coordinate system
// Return Value:
//   Index of the sample. It is used to get the results of computation (see 
//   Tr2IntEnlightenTaskManager::GetSHProbeResult)
// --------------------------------------------------------------------------------------
unsigned int Tr2IntEnlightenTaskManager::AddSHProbeSample( unsigned int sampleIndex, Tr2InteriorCell* cell, const Vector3& position )
{
	SHSample::CellData data;
	data.cell = cell;
	data.transform = cell->GetWorldTransform();
	cell->GetBoundingBox( data.minBounds, data.maxBounds );
	if( sampleIndex == UninitializedIndex )
	{
		SHSample sample;
		sample.cells.push_back( data );
		sample.position = position;
		sample.valid = false;
		m_samplePoints.push_back( sample );
		return unsigned int( m_samplePoints.size() - 1 );
	}
	else
	{
		m_samplePoints[sampleIndex].cells.push_back( data );
		return sampleIndex;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns results of computing SH lighting coefficient for a sample point.
// Arguments:
//   sampleIndex - Sample index (result of previous call to 
//                 Tr2IntEnlightenTaskManager::AddSHProbeSample function
// Return Value:
//   SH coefficients (array of 9 RGB vectors) or NULL if the sample index provided was
//   invalid.
// --------------------------------------------------------------------------------------
const Vector3* Tr2IntEnlightenTaskManager::GetSHProbeResult( unsigned int sampleIndex )
{
	if( sampleIndex < m_samplePoints.size() && m_samplePoints[sampleIndex].valid )
	{
		return m_samplePoints[sampleIndex].coefficients;
	}
	return NULL;
}

// ------------------------------------------------------------------------------------------------------
void Tr2IntEnlightenTaskManager::Execute()
{
	if( !m_systems.empty() )
	{
		m_cellsLocked = true;
		for( Systems::iterator it = m_systems.begin(); it != m_systems.end(); ++it )
		{
			(*it)->Lock();
		}
		for( CellMap::iterator it = m_cellData.begin(); it != m_cellData.end(); ++it )
		{
			// Unlock locked cell
			it->first->Lock();
		}
		m_root->wait_for_all();
		m_root->destroy( *m_root );
		m_root = new( tbb::task::allocate_root() ) tbb::empty_task;
		// set ref count to 2 because it will be decreased by 1 when the
		// update task finishes and we still need root task alive after that
		m_root->set_ref_count( 2 );

		m_root->spawn( *new( m_root->allocate_child() ) UpdateTask( *this ) );
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2IntEnlightenTaskManager::IsExecuting() const
{
	return m_root->ref_count() > 1;
}

// ------------------------------------------------------------------------------------------------------
void Tr2IntEnlightenTaskManager::WaitToComplete()
{
	m_root->wait_for_all();
	m_root->destroy( *m_root );
	m_root = new( tbb::task::allocate_root() ) tbb::empty_task;
	m_root->set_ref_count( 1 );
}

// ------------------------------------------------------------------------------------------------------
void Tr2IntEnlightenTaskManager::InvalidateResult()
{
	UnlockCells();

	for( CellMap::iterator it = m_cellData.begin(); it != m_cellData.end(); ++it )
	{
		it->second.volumes.clear();
	}
	m_cellData.clear();
	m_systems.clear();
	m_samplePoints.clear();
}

// ------------------------------------------------------------------------------------------------------
void Tr2IntEnlightenTaskManager::Update( Be::Time time )
{
	if( !IsExecuting() )
	{
		UnlockCells();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Unlocks Enlighten systems and cells that were locked for computing Enlighten.
// --------------------------------------------------------------------------------------
void Tr2IntEnlightenTaskManager::UnlockCells()
{
	if( m_cellsLocked )
	{
		m_cellsLocked = false;

		// Store systems/cells in a temporary vector before
		// unlocking them to prevent re-eterant calls
		// (Tr2InteriorCell and Tr2InteriorEnlightenSystem
		// call InvalidateResult in destructor).
		std::vector<IRoot*> objectsToUnlock;

		for( Systems::iterator it = m_systems.begin(); it != m_systems.end(); ++it )
		{
			objectsToUnlock.push_back( (* it )->GetRawRoot() );
		}
		for( CellMap::iterator it = m_cellData.begin(); it != m_cellData.end(); ++it )
		{
			objectsToUnlock.push_back( it->first->GetRawRoot() );
		}

		for( std::vector<IRoot*>::iterator it = objectsToUnlock.begin(); it != objectsToUnlock.end(); ++it )
		{
			(* it )->Unlock();
		}
	}
}


// ------------------------------------------------------------------------------------------------------
Tr2IntEnlightenTaskManager::TaskInfo::TaskInfo()
:	m_taskManager( 0 )
{
}

// ------------------------------------------------------------------------------------------------------
Tr2IntEnlightenTaskManager::TaskInfo::~TaskInfo()
{
	if( m_taskManager )
	{
		m_taskManager->m_infoObjects.erase( this );
	}
}

// ------------------------------------------------------------------------------------------------------
Tr2IntEnlightenTaskManager::TaskInfo::TaskInfo( const TaskInfo& info )
:	m_taskManager( info.m_taskManager )
{
	if( m_taskManager )
	{
		m_taskManager->m_infoObjects.insert( this );
	}
}

// ------------------------------------------------------------------------------------------------------
Tr2IntEnlightenTaskManager::TaskInfo& Tr2IntEnlightenTaskManager::TaskInfo::operator=( const TaskInfo& info )
{
	if( m_taskManager == info.m_taskManager )
	{
		return *this;
	}
	if( m_taskManager )
	{
		m_taskManager->m_infoObjects.erase( this );
	}
	m_taskManager = info.m_taskManager;
	if( m_taskManager )
	{
		m_taskManager->m_infoObjects.insert( this );
	}
	return *this;
}

// -------------------------------------------------------------
// Description:
//   Assigns task manager pointer. Removes this object from previous
//   task manager TaskInfo list and adds it to the new one's list.
// Arguments:
//   taskManager - A new task manager to point to
// -------------------------------------------------------------
void Tr2IntEnlightenTaskManager::TaskInfo::SetTaskManager( Tr2IntEnlightenTaskManager* taskManager )
{
	if( m_taskManager == taskManager )
	{
		return;
	}
	if( m_taskManager )
	{
		m_taskManager->m_infoObjects.erase( this );
	}
	m_taskManager = taskManager;
	if( m_taskManager )
	{
		m_taskManager->m_infoObjects.insert( this );
	}
}

// -------------------------------------------------------------
// Description:
//   Call task manager's IsExecuting function or pretend it's not
//   executing if the pointer to the manager is zero.
// Return value:
//   true - If any manager task is still executing
//   false - If all manager tasks finished executing
// -------------------------------------------------------------
bool Tr2IntEnlightenTaskManager::TaskInfo::IsExecuting() const
{
	if( m_taskManager )
	{
		return m_taskManager->IsExecuting();
	}
	return false;
}

// -------------------------------------------------------------
// Description:
//   Waits for all task manager tasks to complete or pretends 
//   everything has completed if the pointer to the manager is zero.
// -------------------------------------------------------------
void Tr2IntEnlightenTaskManager::TaskInfo::WaitToComplete()
{
	if( m_taskManager )
	{
		m_taskManager->WaitToComplete();
	}
}

// -------------------------------------------------------------
// Description:
//   Invalidates results of the previous computation for the task
//   manager, unlocks cells/systems used in computation. 
//   Doesn't do anything if the pointer to the manager is zero.
// -------------------------------------------------------------
void Tr2IntEnlightenTaskManager::TaskInfo::InvalidateResult()
{
	if( m_taskManager )
	{
		m_taskManager->InvalidateResult();
	}
}

BLUE_DEFINE( Tr2IntEnlightenTaskManager );

// ------------------------------------------------------------------------------------------------------
const Be::ClassInfo* Tr2IntEnlightenTaskManager::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2IntEnlightenTaskManager, "" )
        MAP_INTERFACE( Tr2IntEnlightenTaskManager )
	EXPOSURE_END()
}

#endif
