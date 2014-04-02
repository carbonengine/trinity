#pragma once
#ifndef Tr2InteriorEnlightenSystem_H
#define Tr2InteriorEnlightenSystem_H

#include "Include/ITr2Interior.h"
#include "Tr2DeviceResource.h"
#include "ID3DTexture.h"
#include "Tr2InteriorVisualization.h"
#include "Tr2InteriorEnlightenUpdateTaskManager.h"
#include "Tr2InteriorEnlightenSystemImpl.h"

// forwards
BLUE_DECLARE( TriLineSet );
BLUE_DECLARE( Tr2InteriorCell );
BLUE_DECLARE( Tr2RadiosityRes );
BLUE_DECLARE( Tr2VariableStore );
BLUE_DECLARE( Tr2InteriorStatic );
BLUE_DECLARE_VECTOR( Tr2InteriorStatic );
BLUE_DECLARE( Tr2EnlightenArea );

struct EnlightenMaterialProperties
{
	Color m_albedoColor;
	Color m_emissiveColor;
	bool m_isEmissive;
	unsigned int m_areaLightID;
};

class Tr2InteriorEnlightenSystem :
	public IInitialize,
	public INotify,
	public IListNotify,
	public Tr2DeviceResource,
	public IBlueAsyncResNotifyTarget,
	public ITr2InteriorEnlightenGeometryProvider,
	public ID3DTexture					// This allows us to attach the system to a texture render step and view the irradiance map
{
public:
	Tr2InteriorEnlightenSystem( IRoot* lockobj = 0 );
	~Tr2InteriorEnlightenSystem();

	EXPOSE_TO_BLUE();

    using IInitialize::Lock;
    using IInitialize::Unlock;
	
	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource

	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:	

#if TRINITYDEV
	virtual void GetDescription( std::string& desc ) { desc = "Tr2InteriorEnlightenSystem"; }
#endif

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* value );

	//////////////////////////////////////////////////////////////////////////
	// IAsyncLoadedResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* p );
	void RebuildCachedData( BlueAsyncRes* p );

	//////////////////////////////////////////////////////////////////////////
	// ID3DTexture
	virtual Tr2TextureAL* GetTexture() { return &m_enlightenIrradianceTexture; }

	//////////////////////////////////////////////////////////////////////////
	// init
	void SetParentCell( Tr2InteriorCell* parentCell );
	Tr2InteriorCell* GetParentCell() const { return m_parentCell; }
	const Matrix& GetParentTransform() const;

	// handle statics
	void AddStatic( Tr2InteriorStatic* interiorStatic );
	void RemoveStatic( Tr2InteriorStatic* interiorStatic );
	void ClearStatics();

	// query
	bool GetBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const;
	// Check if the system is dirty
	bool IsDirty( void );
	// Reset the dirty flag
	void ResetDirtyFlag( void );

	// timing
	void Update( Be::Time time );

	// Prepare Enlighten data for multithreaded update
	bool PrepareUpdateEnlighten( std::vector<const Enlighten::InputLightingBuffer*>& inputLighting, 
								 const Enlighten::EmissiveEnvironment* environment, 
								 Tr2IntEnlightenTaskManager &updateTask );
	// Update Enlighten textures with data computed during multithreaded update
	void UpdateEnlightenTextures();
	// Update Enlighten lighting buffers (called from separate threads)
	void UpdateEnlightenLightingThreaded();
	// Update Enlighten data (called from separate threads)
	void UpdateEnlightenSolutionThreaded();

	void RenderEnlightenMaterialTextures();
	void UpdateEnlightenMaterialTextures();

	void UpdateEnlightenWorkspace( const PITr2InteriorLightVector& lightSources );
	void UpdateEnlightenInputLighting( const PITr2InteriorLightVector &lightSources );
	void UpdateEnlightenMaterials();

	// Return input lighting buffer for Enlighten update
	Enlighten::InputLightingBuffer* const GetEnlightenInputLightingBuffer() const { return m_enlightenSystem.GetCurrentInputLightingBuffer(); }
	
	// culling
	void SetVisibility( bool bVisible );
	void ClearVisibility( void );
	void RebuildBoundingBox();
	void UpdateBoundingBox( const Vector3& minBounds, const Vector3& maxBounds );

	// debug
	void RenderDebugInfo( TriLineSetPtr lines ) const;
	void RenderOcclusionGeometry() const;
	void SetVisualizeMethod( VisualizeMethod method );
	void UpdateLightCount( const PITr2InteriorLightVector& lightSources );
	bool RebuildEnlightenTextures();
	void ReleaseEnlightenD3DResources();

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	// Enlighten
	void BuildEnlightenPreClustering( Enlighten::IPrecompute* pPrecompute, std::vector< Enlighten::IPrecompPackedSystem* >& neighbours, TriEnlightenProgressBar& prog );
	void BuildEnlightenClustering( Enlighten::IPrecompute* pPrecompute, std::vector< Enlighten::IPrecompSystemPreClustering* >& neighbours, TriEnlightenProgressBar& prog );
	void BuildEnlightenSystem( Enlighten::IPrecompute* pPrecompute, std::vector< Enlighten::IPrecompPackedSystem* >& neighbours, std::vector< Enlighten::IPrecompSystemClustering* >& systemClusters, TriEnlightenProgressBar& prog );
	void DeletePrecompData();
	void PackEnlightenSystem( Tr2InteriorEnlightenSystemImpl::Quality quality, Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog, unsigned int systemInCellIdx );
	
	Enlighten::IPrecompPackedSystem* GetPackedSystem();
	Enlighten::IPrecompSystemPreClustering* GetSystemPreClustering();
	Enlighten::IPrecompSystemClustering* GetSystemClustering();
	
	void SaveEnlightenSystem();
#endif

	void InvalidateLightCache( void ) { m_isInputLightingCacheDirty = true; }

	// For chart visualisation
	// Used to generate vis textures (x,y are pixel position)
	int GetChartIndex( int x, int y ) const;
	int GetChartWidth() const;
	int GetChartHeight() const;

	// System integrity 
	void SetSystemInCellIdx( unsigned int idx );

	// Make the list of statics available externally to allow poking data into the statics from the cells
	const PTr2InteriorStaticVector* GetStatics() { return &m_statics; }

	// Returns local variable store for this system
	Tr2VariableStore& GetVariableStore() { return *m_variableStore; }

	// Type (quality) of Enlighten textures
	enum EnlightenQuality
	{
		// Irradiance-only textures
		IRRADIANCE,
		// Directional irradiance textures
		DIRECTIONAL_IRRADIANCE,
		// Spherical harmonics textures
		SPHERICAL_HARMONICS,
	};

	EnlightenQuality GetEnlightenQuality() const;

private:

	//////////////////////////////////////////////////////////////////////////
	// ITr2InteriorEnlightenGeometryProvider
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	bool GetEnlightenPackedGeometry( Enlighten::IPrecompute* precompute, 
									 Enlighten::IPrecompInputSystem* inputSystem, 
									 TriEnlightenProgressBar& prog, 
									 float outputPixelSize, 
									 std::vector<Enlighten::IPrecompPackedGeometry*>& geometries );
#endif
	void ReleaseInputLightingCache( void );
	void AllocateInputLightingCache( void );

	bool NeedToRebuildInputLightingCache( const PITr2InteriorLightVector& lightSources );

	void AddLightToInputWorkspace( const ITr2InteriorLight* light, Enlighten::InputLightingBuffer* inputLightingCache );

	// For chart visualisation
	// Call before DeletePrecompData()
	void CaptureChartMapping();

	void UpdateEnlightenDebugTextures();
private:
	std::string m_name;
	// cell contains static renderables
	PTr2InteriorStaticVector m_statics;

	// sub-cell has a size
	Vector3 m_minBounds, m_maxBounds;

	// System texture width and height
	Vector2 m_pixelDimensions;

	// sub-cell knows its parent
	Tr2InteriorCell* m_parentCell;

	// Visibility, determined by Umbra
	bool m_isVisible;

	// Dirty flag, set when statics are added or removed
	bool m_isDirty;

	// debug
	bool m_drawBoundingBox;
	bool m_debugRenderDusterLighting;
	VisualizeMethod m_visualizeMethod;

	// Use albedo colors from texture or from Enlighten areas
	bool m_useTextureAlbedo;

	// optimization
	bool m_updateRadiosity;
	bool m_updateInputLighting;

	// static light caching
	bool m_isInputLightingCacheDirty;

	//////////////////////////////////////////////////////////////////////////
	// Enlighten stuff follows...
	//////////////////////////////////////////////////////////////////////////
	Tr2InteriorEnlightenSystemImpl m_enlightenSystem;

	// Resources for Enlighten output
	Tr2TextureAL m_enlightenIrradianceTexture;
	Tr2TextureAL m_enlightenSHRTexture;
	Tr2TextureAL m_enlightenSHBTexture;
	Tr2TextureAL m_enlightenSHGTexture;
	Tr2TextureAL m_enlightenDirectionalIrradianceTexture;
	// Debug visualization mode Enlighten textures
	enum { DebugDensity, DebugChart, DebugNaughtyPixels, DebugAlbedo, DebugEmissive, DebugMaxCount };
	Tr2TextureAL	m_enlightenDebugTexture[ DebugMaxCount ];	
	// Enlighten textures need updating
	bool m_enlightenTexturesDirty;

	// Material Caching for Enlighten
	typedef std::map<Tr2EnlightenArea*,EnlightenMaterialProperties> MaterialCacheMap;
	MaterialCacheMap m_enlightenMaterialCache;

	unsigned int m_inputLightingCacheSize;
	Enlighten::InputLightingBuffer* m_inputLightingCache;

	float m_bounceScale;
	float m_irradianceScale;

	// Pointer to the per-scene object for multithreaded Enlighten update
	Tr2IntEnlightenTaskManager::TaskInfo m_updateTaskInfo;

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	// Map between TriGeometryRes and its packed geometry. Used during Enlighten precompute.
	typedef std::map<TriGeometryRes*, Enlighten::IPrecompPackedGeometry*> PackedGeometryMap;
	PackedGeometryMap m_packedGeometry;
#endif

	// Local variable store for this system
	Tr2VariableStorePtr m_variableStore;
	
	// --------------------------------------------------------------------------------------
	// Description:
	//   All parameters needed for Enlighten update. They are in a separate structure to
	//   explicitely isolate them from other member variables as these are used from
	//   separate threads.
	// --------------------------------------------------------------------------------------
	struct SolveData
	{
		const Enlighten::InputWorkspace* inputWorkspace;
		Enlighten::InputLightingBuffer*	lightingBuffer;
		const Enlighten::InputLight* lightArray;
		Geo::s32 numLights;

		Geo::v128 *dusters;
		Enlighten::InputLightingBuffer*	lightingCache;

		const Enlighten::InputLightingBuffer* previousLightingBuffer;
		float bounceScale;
		const Geo::u32*	albedoTextureData;
		const Geo::u32* emissiveTextureData;
		Enlighten::ClusterAlbedoWorkspace* clusterAlbedoWorkspace;

		const Enlighten::RadSystemCore* coreSystem;
		const Enlighten::InputLightingBuffer** inputLighting;
		const Enlighten::EmissiveEnvironment* environment;
		float outputScale;
		short* redSphericalOutput;
		short* greenSphericalOutput;
		short* blueSphericalOutput;
		short* irradianceOutput;
		char* directionalIrradianceOutput;
		void* bounceData;
		bool enableTemporalCoherence;

		enum
		{
			INITIALIZED,
			LIGHTING_SOLVED,
			SOLVED,
			UNCHANGED,
		} state;
	} m_solveData;

	enum
	{
		TEXTURES_DIRTY,
		TEXTURES_RENDERED,
		TEXTURES_UPDATED,
	} m_albedoTextureState;

	Tr2RenderTargetAL	m_albedoTextureRenderTarget;
	Tr2RenderTargetAL	m_emissiveTextureRenderTarget;

	// Current Enlighten texture quality
	EnlightenQuality m_enlightenQuality;
	// Flag indicating that texture quality has
	// changed (need to disable temporal coherence
	// on the next Enlighten solve).
	bool m_enlightenQualityChanged;
};
TYPEDEF_BLUECLASS( Tr2InteriorEnlightenSystem );
BLUE_DECLARE_VECTOR( Tr2InteriorEnlightenSystem );


#endif // Tr2InteriorEnlightenSystem_H
