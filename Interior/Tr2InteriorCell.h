#pragma once
#ifndef Tr2InteriorCell_H
#define Tr2InteriorCell_H

#include "include/ITr2Interior.h"
#include "Tr2InteriorEnlightenSystemImpl.h"
#include "Tr2InteriorVisualization.h"
#include "Tr2InteriorProbeVolume.h"
#include "Tr2InteriorEnlightenUpdateTaskManager.h"

// forwards
BLUE_DECLARE( Tr2InteriorCell );
BLUE_DECLARE( TriLineSet );
BLUE_DECLARE( Tr2SHProbeRes );
BLUE_DECLARE( Tr2InteriorProbeVisualizer );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2VariableStore );
BLUE_DECLARE( Tr2InteriorEnlightenSystem );
BLUE_DECLARE_VECTOR( Tr2InteriorEnlightenSystem );
BLUE_DECLARE( Tr2InteriorPortalSocket );
BLUE_DECLARE_VECTOR( Tr2InteriorPortalSocket );
BLUE_DECLARE( Tr2InteriorOccluder );
BLUE_DECLARE_VECTOR( Tr2InteriorOccluder );
BLUE_DECLARE( Tr2InteriorBoundingBox );
BLUE_DECLARE_VECTOR( Tr2InteriorBoundingBox );

class Tr2InteriorCell :
	public IInitialize,
	public INotify,
	public IListNotify,
	public IBlueAsyncResNotifyTarget,
	public ITr2InteriorEnlightenGeometryProvider
{
public:
	Tr2InteriorCell( IRoot* lockobj = 0 );
	~Tr2InteriorCell();

	EXPOSE_TO_BLUE();

    using IInitialize::Lock;
    using IInitialize::Unlock;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* value );

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );

	//////////////////////////////////////////////////////////////////////////
	// IAsyncLoadedResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* p );
	void RebuildCachedData( BlueAsyncRes* p );

	// Is the cell dirty (needs intersection with lights, dynamics, etc. update)
	bool IsDirty( void );
	// Resets the cell's dirty flag
	void ResetDirtyFlag( void );

	// query
	Umbra::Cell* GetUmbraCell() const		{	return m_umbraCell;			}
	void UpdateBoundingBox( void );
	bool IsBoundingBoxReady( void ) const;
	void RebuildBoundingBox( void );
	bool IsUnbounded( void ) const;
	const std::string& GetName( void ) const { return m_name; }
	const Matrix& GetWorldTransform() const { return m_worldTransform; }


	// timing
	void UpdateEnlightenWorkspace( int cutoffDepth );
	// Updates a list of Enlighten system workspaces
	void UpdateEnlightenWorkspaceList( int cutoffDepth );
	void Update( Be::Time time, int cutoffDepth );

	// Prepare Enlighten data for multithreaded update
	void PrepareUpdateEnlighten( int cutoffDepth, Tr2IntEnlightenTaskManager &task );
	// Update Enlighten system textures with data computed during multithreaded update, update dynamics lighting
	void ProcessEnlightenResults( Tr2IntEnlightenTaskManager &task );

	void SetEnvironmentCube( const Geo::v128* cube );

	// Reflection map
	TriTextureRes* GetReflectionMap();

	// changes
	void RebuildInternalData();

	void UpdateBoundingBox( const Vector3& minBounds, const Vector3& maxBounds );
	
	// Handle systems
	void AddSystem( Tr2InteriorEnlightenSystem* interiorSystem );
	void RemoveSystem( Tr2InteriorEnlightenSystem* interiorSystem );
	void ClearSystems();
	// Handle dynamics
	bool AddDynamic( ITr2InteriorDynamic* dynamic );
	bool RemoveDynamic( ITr2InteriorDynamic* dynamic );
	// Handle lights
	bool AddLight( ITr2InteriorLight* light );
	void RemoveLight( ITr2InteriorLight* light );

	// Debug
	void RenderDebugInfo( TriLineSetPtr lines );
	void RenderOcclusionGeometry() const;
	void SetVisualizeMethod( VisualizeMethod method );

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	// Enlighten
	bool BuildPackedSystems( Tr2InteriorEnlightenSystemImpl::Quality quality, Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog );
	bool BuildSystemPreClusters( Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog );
	bool BuildSystemClusters( Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog );
	void BuildLightProbes( Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog );
	void BuildEnlightenSolution( Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog );
	void DeletePrecompData();
	// Returns true iff the cell has Enlighten data ready to rebuild light probes
	bool CanBuildLightProbes() const;
	void GatherPackedSystems( void );
	void GatherSystemPreClusters( void );
	void GatherSystemClusters( void );
	void GetPackedSystems( std::vector<Enlighten::IPrecompPackedSystem*>& packedSystems ) const;
	void GetSystemPreClusters( std::vector<Enlighten::IPrecompSystemPreClustering*>& systemClusters ) const;
	void GetSystemClusters( std::vector<Enlighten::IPrecompSystemClustering*>& systemClusters ) const;

	void SaveEnlightenSystems();
	void SaveLightProbes();

	bool BuildEnlightenSystems();
	bool PreviewEnlightenSystems();
#endif

	void GetInputLightingBuffers( std::vector<const Enlighten::InputLightingBuffer*>& inputLightingBuffers ) const;

	void SetSHProbeResource();
	void SetReflectionMapPath();
	bool WriteSHProbeDataToDisk();
	// Populate probe volumes vector with data from SH file
	bool PopulateProbeVolumes();

	void GetLightProbeResolution( int* x, int* y, int* z ) const;
	bool IsVolumeReady() const;
	void BuildLightVolume( int xRes, int yRes, int zRes );

	// Umbra
	void DetermineVisibility( int curDepth );
	void SetVisibility( bool bVisible );
	bool GetVisibility( void ) const { return m_isVisible; }
	int GetCellDepth( void ) const { return m_cellDepth; }
	void AddCellNeighbor( Tr2InteriorCell* neighbor );
	void RemoveCellNeighbor( Tr2InteriorCell* neighbor );

	// Returns a probe resolution in local X direction
	int GetDefaultProbeVolumeResolutionX() const;
	// Sets probe resolution in local X direction (Enlighten data is invalidated after setting it)
	void SetDefaultProbeVolumeResolutionX( int );
	// Returns a probe resolution in local Y direction
	int GetDefaultProbeVolumeResolutionY() const;
	// Sets probe resolution in local Y direction (Enlighten data is invalidated after setting it)
	void SetDefaultProbeVolumeResolutionY( int );
	// Returns a probe resolution in local Z direction
	int GetDefaultProbeVolumeResolutionZ() const;
	// Sets probe resolution in local Z direction (Enlighten data is invalidated after setting it)
	void SetDefaultProbeVolumeResolutionZ( int );
	// Sets debug probe spheres brightness factor
	void SetSHScale( float scale );

	void AddPortalSocket( Tr2InteriorPortalSocket* socket );

	void MarkShadowsDirtyForDynamic( ITr2InteriorDynamic* dynamic );
	void MarkShadowsDirtyForSkinnedObjects();

	//	Utility Functions to determine cell Membership
	bool ContainsPoint( const Vector3& testPoint, float epsilon=0.0f );
	bool IntersectsAABB( const Vector3& boxMin, const Vector3& boxMax );
	bool IntersectsOBB( const Vector3& boxCenter, const Vector3& boxExtents, const Quaternion& boxOrientation );
	bool IntersectsSphere( const Vector3& center, float radius );

		//	Deprecated
	bool GetBoundingBox( Vector3& minBounds, Vector3& maxBounds );

	// Returns local variable store for this cell
	Tr2VariableStore& GetVariableStore() { return *m_variableStore; }

private:

	struct SampleVolume;

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	bool BuildEnlightenSystemsImpl( Tr2InteriorEnlightenSystemImpl::Quality quality );
#endif

	// Deletes all sample volume data
	void DeleteSampleVolumes();
	// Deletes sample volume data
	void DeleteSampleVolume( SampleVolume& volume, int pointerCount = GeoEngine::VOL_INTERP_TRI );
	// Creates sample volume based on a given probe volume parameters
	void CreateSampleVolume( const Tr2InteriorProbeVolume &volume );
	// Creates sample volume based data read from file
	void CreateSampleVolume( int resX, int resY, int resZ, const Matrix &transform, Enlighten::RadProbeSetCore* core );
	// Prepares data for calculating lighting data
	void CreateProbeTask( SampleVolume& volume, const Enlighten::RadProbeSetCore* core, int solveCount = GeoEngine::VOL_INTERP_TRI );
	// Render SH probe spheres for the given volume
	void RenderDebugProbeSpheres( const SampleVolume &volume ) const;
	// Render SH probe cull status spheres for the given volume
	void RenderDebugProbeCullStatus( const SampleVolume &volume ) const;
	// Create probe lighting data for sphere probe visualization
	void CreateDebugProbeData( SampleVolume &volume, std::vector<const Enlighten::InputLightingBuffer*>& inputLightingBuffers );

	void UpdateLightProbes( std::vector<const Enlighten::InputLightingBuffer*>& mergedWorkspaces );
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	void BuildLightProbe( Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog, SampleVolume& volume, 
                              const Geo::GeoGuid* systemIDBegin, const Geo::GeoGuid* systemIDEnd, 
                              const Enlighten::IPrecompSystemClustering* const* clusterings, int numClusterings );
	
	//////////////////////////////////////////////////////////////////////////
	// ITr2InteriorEnlightenGeometryProvider
	bool GetEnlightenPackedGeometry( Enlighten::IPrecompute* precompute, 
									 Enlighten::IPrecompInputSystem* inputSystem, 
									 TriEnlightenProgressBar& prog, 
									 float outputPixelSize, 
									 std::vector<Enlighten::IPrecompPackedGeometry*>& geometries );
#endif
public:
	// Default (fallback) light probe volume
	PTr2InteriorProbeVolume m_defaultProbeVolume;
private:
	const std::vector<Vector3>& GetLightProbeList();
	// name
	std::string m_name;

	// World transform
	Vector3 m_position;
	Quaternion m_rotation;
	Matrix m_worldTransform;

	// Reflection map
	std::string m_reflectionMapPath;
	TriTextureResPtr m_reflectionMapRes;

	bool m_isDirty;

	// cell has a size
	Vector3 m_minBounds, m_maxBounds;
	Vector3 m_minBoxGutter, m_maxBoxGutter;
	bool m_boundingBoxReady;
	bool m_isUnbounded;

	// cell content
	PTr2InteriorEnlightenSystemVector m_systems;
	PITr2InteriorLightVector m_lights;
	PITr2InteriorDynamicVector m_dynamics;
	PITr2InteriorDynamicVector m_skinnedObjects;
	PTr2InteriorProbeVolumeVector m_probeVolumes;
	PTr2InteriorOccluderVector m_occluders;
	PTr2InteriorBoundingBoxVector m_boundingBoxes;
	
	// Cell neighbors - implied by portal connections
	// the int value tracks the number of connections from the given cell - which could be > 1
	std::map<Tr2InteriorCellPtr, int> m_cellNeighbors;

	Tr2SHProbeResPtr m_shProbeResource;
	std::string m_shProbeResPath;

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	// Some scratch-pad storage for building Enlighten, not persisted beyond the end of a build
	std::vector<Enlighten::IPrecompPackedSystem*> m_packedSystems;
	std::vector<Enlighten::IPrecompPackedSystem*> m_packedSystemsNeighbors;
	std::vector<Enlighten::IPrecompSystemPreClustering*> m_systemPreClusters;
	std::vector<Enlighten::IPrecompSystemPreClustering*> m_systemPreClustersNeighbors;
	std::vector<Enlighten::IPrecompSystemClustering*> m_systemClusters;
	std::vector<Enlighten::IPrecompSystemClustering*> m_systemClustersNeighbors;
#endif

	Geo::GeoArray<Geo::GeoGuid> m_systemIDs;

	std::vector<const Enlighten::InputLightingBuffer*> m_inputLightingBuffers;

	Enlighten::EmissiveEnvironment* m_enlightenEnvironment;
	// Original cubemap values in world space
	Geo::v128* m_enlightenEnvironmentWorld;

	// Umbra
	Umbra::Cell* m_umbraCell;
	bool m_isVisible;
	// Distance from the visible cell set
	int m_cellDepth;

	// A per-light probe volume data structure
	struct SampleVolume
	{
		Enlighten::RadProbeTask  		m_task;
		Geo::GeoArray<Geo::v128>		m_samples;
		GeoEngine::NonAAVolume*			m_volumeBox;
		Matrix							m_transform;
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
		Enlighten::IPrecompProbeSetRadiosity* m_precomputedProbeSet;
#endif
		Matrix*							m_debugProbeLighting;
	};

	// Light probe volumes used for illuminating dynamic objects
	std::vector<SampleVolume> m_sampleVolumes;

	// debug
	bool m_drawBoundingBox;

	// Light probe visualizer
	Tr2InteriorProbeVisualizerPtr m_probeVisualizer;
	// Sphere probe brightness factor (initialized by Tr2InteriorScene)
	float m_shScale;

	// Pointer to the per-scene object for multithreaded Enlighten update
	Tr2IntEnlightenTaskManager::TaskInfo m_updateTaskInfo;

	PTr2InteriorPortalSocketVector m_portalSockets;
	std::vector<Tr2InteriorPortalSocket*> m_packedPortalSockets;

	// SH probes for portal sockets
	SampleVolume m_portalSocketSamples;
	// Mapping between portal sockets and sample indexes in m_portalSocketSamples
	std::map<Tr2InteriorPortalSocket*, int> m_portalSocketIndexes;

	// Draw debug colored spheres in portal socket probe positions
	bool m_drawSocketProbes;
	// Draw portal socket probe cull status (only when drawSocketProbes is on)
	bool m_drawSocketProbeCulling;

	Tr2InteriorEnlightenSystemImpl m_socketSystem;

	// Buffer for Enlighten socket system solution output
	short* m_socketOutputBuffer;

	// Local variable store for this cell
	Tr2VariableStorePtr m_variableStore;
};

TYPEDEF_BLUECLASS( Tr2InteriorCell );
BLUE_DECLARE_VECTOR( Tr2InteriorCell );


#endif // Tr2InteriorCell_H
