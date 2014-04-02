#pragma once
#ifndef Tr2InteriorScene_H
#define Tr2InteriorScene_H


#include "ITr2PickableScene.h"
#include "ITr2VisibilityQueryable.h"
#include "Tr2PickBuffer.h"
#include "Tr2UmbraScene.h"
#include "Tr2InteriorVisualization.h"
#include "Tr2InteriorEnlightenUpdateTaskManager.h"
#include "include/ITr2MultiPassScene.h"
#include "ITr2VisualizationModeRenderer.h"
#include "Tr2InteriorSHLightingSolver.h"
#include "Tr2InteriorLightSet.h"
#include "Tr2InteriorEnlightenSystemImpl.h"
#include "Include/ITr2Scene.h"

// Forward declarations
struct Tr2VisibilityEvent;
struct Tr2PerFramePSData;
struct Tr2PerFrameVSData;
class TriProjection;
class TriVariable;
class TriView;
class TriViewport;
enum Tr2InteriorBatchGroup;
class TriEnlightenProgressBar;

// lod resource unloading
extern float g_wodAvatarResourceUnloadingTimeThreshold;

// Blue forward declarations
BLUE_DECLARE( Tr2ApexScene );
BLUE_DECLARE( Tr2InteriorScene );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2VisibilityResults );
BLUE_DECLARE( Tr2TextureAtlas );
BLUE_DECLARE_VECTOR( Tr2TextureAtlas );
BLUE_DECLARE( Tr2ShaderMaterial );
BLUE_DECLARE_INTERFACE( ITr2PhysicsUpdater );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( Tr2InteriorCell );
BLUE_DECLARE_VECTOR( Tr2InteriorCell );
BLUE_DECLARE( Tr2InteriorPhysicalPortal );
BLUE_DECLARE_VECTOR( Tr2InteriorPhysicalPortal );
BLUE_DECLARE( TriLineSet );

class Tr2InteriorScene:
	public IInitialize,
	public INotify,
	public IListNotify,
	public ITr2Scene,
	public Tr2UmbraScene,
	public ITr2MultiPassScene,
	public ITr2PickableScene,
	public ITr2VisibilityQueryable,
	public IBlueAsyncResNotifyTarget,
	public Tr2DeviceResource,
	public ITr2VisualizationModeRenderer
{
public:
	Tr2InteriorScene( IRoot* lockobj = NULL );
	~Tr2InteriorScene();

	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
    virtual bool OnModified( Be::Var* value );
	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );

	//////////////////////////////////////////////////////////////////////////
	// ITr2Scene (via Tr2UmbraScene)
	virtual void Update( Be::Time time );
	virtual void Render( Tr2RenderContext& renderContext );
	virtual void RenderDebugInfo( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////
	// ITr2MultiPassScene
	virtual RenderPassResult RenderPass( PassType pass, Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////
	// ITr2VisibilityQueryable
	virtual void VisibilityQuery( Tr2VisibilityResults* visibilityResults );
	virtual void SetVisibilityResults( Tr2VisibilityResults* visibilityResults );

	//////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	virtual void ReleaseCachedData( BlueAsyncRes* p ) {}
	virtual void RebuildCachedData( BlueAsyncRes* p );

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:	

	//////////////////////////////////////////////////////////////////////////
	// ITr2VisualizationModeRenderer
	virtual void SetVisualizationMode( int visualizationMode );

	void SetRenderBackgroundCubeMap( bool renderBackgroundCubemap );
	
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	void SaveEnlighten();
	bool BuildEnlighten( TriEnlightenProgressBar &progress );
	bool PreviewEnlighten( TriEnlightenProgressBar &progress );
	bool BuildLightProbes();
#endif

	void RebuildSceneData( void );
	// Populate probe volumes vector with data from SH file for all cells
	bool PopulateProbeVolumes();

	// Shader overrides for effect debugging
	void SetVisualizeMethod( void );

	// Add a light source to the scene
	void AddLightSource( ITr2InteriorLight* lightSource );
	// Remove a light source from the scene
	void RemoveLightSource( ITr2InteriorLight* lightSource );
	// Add a dynamic to the scene
	void AddDynamic( ITr2InteriorDynamic* dynamic );
	// Remove a dynamic from the scene
	void RemoveDynamic( ITr2InteriorDynamic* dynamic );

	void ReorderDynamic( ITr2InteriorDynamic* object, ITr2InteriorDynamic* insertAfter );

	// Forces update on all spotlight shadows
	void UpdateSpotlightShadows();

protected:

	//////////////////////////////////////////////////////////////////////////
	// Umbra callbacks

	// Handle Umbra::Commander::QUERY_BEGIN
	virtual void OnQueryBegin( void );
	// Handle Umbra::Commander::QUERY_END
	virtual void OnQueryEnd( void );
	// Handle Umbra::Commander::PORTAL_ENTER
	virtual void OnPortalEnter( void );
	// Handle Umbra::Commander::PORTAL_EXIT
	virtual void OnPortalExit( void );
	// Handle Umbra::Commander::PORTAL_PRE_EXIT
	virtual void OnPortalPreExit( void );
	// Handle Umbra::Commander::VIEW_PARAMETERS_CHANGED
	virtual void OnViewParametersChanged( void );
	// Handle Umbra::Commander::INSTANCE_VISIBLE
	virtual void OnInstanceVisible( void );
	// Handle Umbra::Commander::REGION_OF_INFLUENCE_ACTIVE
	virtual void OnRegionOfInfluenceActive( void );
	// Handle Umbra::Commander::REGION_OF_INFLUENCE_INACTIVE
	virtual void OnRegionOfInfluenceInactive( void );
	// Handle Umbra::Commander::STENCIL_MASK
	virtual void OnStencilMask( void );
	// Handle Umbra::Commander::CELL_IMMEDIATE_REPORT
	virtual void OnCellImmediateReport( void );
	// Handle Umbra::Commander::DRAW_LINE_3D
	virtual void OnDrawLine3D( void );

	//////////////////////////////////////////////////////////////////////////
	// Visibility event handlers

	enum BatchGatherType
	{
		IMMEDIATE_GATHER,
		PREPASS_GATHER,
		LIGHT_GATHER,
		PREPASS_FORWARD_GATHER,
		FULL_FORWARD_GATHER,
		FLARE_GATHER,
	};

	// Handle Tr2VisibilityEvent::QUERY_BEGIN
	void DoQueryBegin( const Tr2VisibilityEvent& event, BatchGatherType gatherType );
	// Handle Tr2VisibilityEvent::QUERY_END
	void DoQueryEnd( const Tr2VisibilityEvent& event, BatchGatherType gatherType );
	// Handle Tr2VisibilityEvent::PORTAL_ENTER
	void DoPortalEnter( const Tr2VisibilityEvent& event, BatchGatherType gatherType );
	// Handle Tr2VisibilityEvent::PORTAL_EXIT
	void DoPortalExit( const Tr2VisibilityEvent& event, BatchGatherType gatherType );
	// Handle Tr2VisibilityEvent::PORTAL_PRE_EXIT
	void DoPortalPreExit( const Tr2VisibilityEvent& event, BatchGatherType gatherType );
	// Handle Tr2VisibilityEvent::VIEW_PARAMETERS_CHANGED
	void DoViewParametersChanged( const Tr2VisibilityEvent& event, 
		BatchGatherType gatherType );
	// Handle Tr2VisibilityEvent::INSTANCE_VISIBLE
	void DoInstanceVisible( const Tr2VisibilityEvent& event, 
		BatchGatherType gatherType );
	// Handle Tr2VisibilityEvent::REGION_OF_INFLUENCE_ACTIVE
	void DoRegionOfInfluenceActive( const Tr2VisibilityEvent& event, 
		BatchGatherType gatherType );
	// Handle Tr2VisibilityEvent::REGION_OF_INFLUENCE_INACTIVE
	void DoRegionOfInfluenceInactive( const Tr2VisibilityEvent& event, 
		BatchGatherType gatherType );
	// Handle Tr2VisibilityEvent::STENCIL_MASK
	void DoStencilMask( const Tr2VisibilityEvent& event, BatchGatherType gatherType );

	// Gather batches for pre-pass (ignoring transparency)
	void GatherPrePassBatches( Tr2VisibilityResults* results );
	// Gather batches from lights for the light pass
	void GatherLightBatches( Tr2VisibilityResults* results );
	// Gather batches for forward pass (prepass)
	void GatherPrepassForwardBatches( Tr2VisibilityResults* results );
	// Gather batches for full-forward pass (not prepass)
	void GatherFullForwardBatches( Tr2VisibilityResults* results );
	// Gather batches from lights for the flare pass
	void GatherFlareBatches( Tr2VisibilityResults* results );

private:
	// Create Root cell (holds camera when camera is outside any physical cell)
	void CreateRootCell( void );
	// Release Root cell
	void ReleaseRootCell( void );
	// Updates Root portals (connecting root cell to interior cells)
	void UpdateRootPortals( void );
	// Updates the root portal for a particular cell
	void UpdateRootPortalForCell( Tr2InteriorCell* cell );
	// Release root portals
	void ReleaseRootPortals( void );
	void ResolveVisibility( Tr2UmbraCamera& camera );

	// Determine which cell the camera is in
	Umbra::Cell* GetCameraCell( void );
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	bool BuildEnlightenImpl( Tr2InteriorEnlightenSystemImpl::Quality quality, TriEnlightenProgressBar& progress );
#endif
	// Update lights, adding to cells as needed
	void UpdateLights( void );
	// Update dynamics, adding to cells as needed
	void UpdateDynamics( void );
	void UpdateCells();

	// Are we in a mirrored state?
	bool IsMirrored( void ) const;

	// Handle list-changed events for the lights list
	bool OnLightsListModified( long event, SSIZE_T key, SSIZE_T key2, IRoot* currvalue );
	// Handle light-changed events for the dynamics list
	bool OnDynamicsListModified( long event, SSIZE_T key, SSIZE_T key2, IRoot* currvalue );

	void BeginRender( Tr2RenderContext& renderContext );
	void RenderPrePass( Tr2RenderContext& renderContext );
	void RenderLightPass( Tr2RenderContext& renderContext );
	void RenderGatherPass( Tr2RenderContext& renderContext );
	void RenderFlarePass( Tr2RenderContext& renderContext );
	void EndRender( Tr2RenderContext& renderContext );
	void RenderFullForward( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////
	// Rendering utility functions

	// Queue up a background cubemap batch
	void PrepareBackgroundCubemapBatch( ITriRenderBatchAccumulator* batches );

private:

	// Holds the main render batch list for opaques, decals, and transparent batches
	ITriRenderBatchAccumulator* m_primaryRenderBatches;
	// Stores transparent batches during scene traversal for later insertion into primary batch list
	ITriRenderBatchAccumulator* m_transparentBatchStore;
	// Holds the batches for the scene shadowcasters
	ITriRenderBatchAccumulator* m_shadowBatches;
	// Holds the opaque batches for pickable objects during a picking operation
	ITriRenderBatchAccumulator* m_opaquePickingBatches;
	// Holds the opaque batches for pickable objects during a picking operation
	ITriRenderBatchAccumulator* m_pickingBatches;
	// Holds the batches for pre-pass
	ITriRenderBatchAccumulator* m_prepassBatches;

	// This is a handle to another batch accumulator, so the Umbra scene traversal can accumulate
	// renderable batches into different lists, depending on the scene query type
	ITriRenderBatchAccumulator* m_activePrimaryRenderBatches;
	// This is a handle to a temporary transparent batch store, so the Umbra scene traversal can
	// accumulate transparent batches into different lists
	ITriRenderBatchAccumulator* m_activeTransparentBatchStore;

	// Manages asynchronous Enlighten updates
	CTr2IntEnlightenTaskManager m_enlightenUpdateTaskManager;

	// Direction and color of sun light
	Vector3 m_sunDirection;
	Color m_sunDiffuseColor;
	Color m_sunSpecularColor;

	// Handle to "Sun.DirWorld" and "Sun.DiffuseColor" variables in variable store
	Tr2Variable m_sunDirectionVar;
	Tr2Variable m_sunDiffuseColorVar;
	Tr2Variable m_sunSpecularColorVar;

	Color m_ambientColor;
	Tr2Variable m_ambientColorVar;

	Tr2Variable m_cameraPosVar;

	// Hack flag to allow scene to ignore the root culling cell.  This is necessary
	// to avoid problems with unbounded cells.  
	// TODO: refactor this whole root cell business.
	// <delder>
	bool m_sceneUseRootCell;

	// filtering the results of umbra culling against a list of objects allowed to draw
	bool m_useFilterList;
	PITr2InteriorDynamicVector	m_filterList;

	// a scene is made of cells
	PTr2InteriorCellVector m_cells;
	// connected by portals
	PTr2InteriorPhysicalPortalVector m_portals;
	// lights
	PITr2InteriorLightVector m_lights;
	// dynamics
	PITr2InteriorDynamicVector m_dynamics;
	// dynamics pending load
	PITr2InteriorDynamicVector m_dynamicsPendingLoad;

	// Visibility result set
	Tr2VisibilityResultsPtr m_visibilityResults;

	// Stack of objects/lights visited during scene traversal
	std::vector<std::unordered_set<IRoot*>*> m_visitedObjects;

	// Set of visible lights
	std::set<ITr2InteriorLight*> m_visibleLights;
	bool m_enableLightCulling;

	enum VisibilityQueryType
	{
		PRIMARY_QUERY,
		SHADOW_QUERY,
		PICKING_QUERY
	};
	VisibilityQueryType m_visibilityQueryType;

	// Root cell - an Uber-cell to hold the camera when the camera is outside the 'proper' cells (as in Jessica)
	Umbra::Cell* m_rootCell;
	Umbra::Object* m_rootObject;
	Umbra::Model* m_rootModel;
	std::map<Umbra::Cell*, std::pair<Umbra::Model*,Umbra::PhysicalPortal*> > m_rootPortals;

	// Intersecting cell portals model (box around a camera)
	Umbra::Model* m_cameraPortalModel;
	// Intersecting cell portals
	std::vector<Umbra::PhysicalPortal*> m_cameraPortals;

	// Umbra cell containing the camera
	Umbra::Cell* m_currentCameraCell;

	// Umbra culling cameras
	Tr2UmbraCamera m_sceneCamera;
	Tr2UmbraCamera m_shadowCamera;

	// Miscellaneous Umbra bullshit
	bool m_isMirroredInLeftHandedSpace;
	int m_mirrorDepth;
	bool m_firstViewParameterChange;
	Matrix m_cameraToWorldMatrix;
	Matrix m_mirrorToWorldMatrix;
	bool m_gatherShadowCasterBatches;
	std::vector<Matrix> m_mirrorToWorldMatrixStack;
	std::vector<std::pair<int, int> > m_transparencyStack;
	std::vector<std::pair<int, int> > m_stencilStack;
	int m_currentObjectGroup;
	bool m_enteringPortal;


	// Active light set
	Tr2InteriorLightSet m_activeLightSet;

	// Maintain a list of visible objects (used for picking)
	std::vector<ITr2Renderable*> m_visibleObjects;

	// Enlighten environment
	Geo::GeoArray<Geo::v128> m_enlightenInputEnvironmentLightingCache;

	Tr2Variable m_backgroundCubeMapVar;
	std::string m_backgroundCubeMapPath;
	TriTextureResPtr m_backgroundCubeMapRes;
	Tr2EffectPtr m_backgroundEffect;
	bool m_renderBackgroundCubeMap;
	float m_enlightenEnvironmentMultiplicationFactor;

	Tr2ApexScenePtr	m_apexScene;

	void SetBackgroundCubemapResPath();
	void SetEnvironmentCubeMapToEnlighten( Tr2RenderContext &renderContext );
	void UpdateEnvironmentLightingFromCacheWithMultiplier();
	// Update each cell SH scale factor
	void UpdateSHScaleFactor();

	// shadows
	void RenderShadowMaps( Tr2RenderContext& renderContext );
	bool UpdateShadowMap( 
		ITr2InteriorLight::LightSourceItem& item, 
		TriPoolAllocator* allocator, 
		Tr2PerFrameVSData shadowPerFrameVS, 
		Tr2PerFrameShadowPSData shadowPerFramePS,
		Tr2RenderContext& renderContext );
	unsigned ReRenderShadowMaps( Tr2RenderContext& renderContext );

	void RenderGeometry( ITr2ShaderMaterial* overrideEffect, Tr2RenderContext& renderContext );

	// Clears the visibility results
	void ClearVisibilityResults( void );

	// Expose some Umbra stuff to Python - WoD needs this for minimap rendering
	// TODO_DME: refactor this out a bit, it's related to cameras in proposed VisibilityQuery render-step
	void SetUmbraProperties( unsigned int properties );
	unsigned int GetUmbraProperties();

	void SetupTransformsForPicking( float fx, float fy, TriProjection* proj, TriView* view, TriViewport* viewport );
	const std::vector<ITr2Renderable*>& GetPickingObjectsToRender( const Vector3& dirWorld );
	const std::vector<ITr2Renderable*>& GetPickingObjectsToRender( const Vector3& dirWorld, float fov, float aspect );

	virtual void SetPerFrameDataForPicking( void );

	virtual ITriRenderBatchAccumulator* GetOpaquePickingBatchAccumulator();
	virtual ITriRenderBatchAccumulator* GetPickingBatchAccumulator();

	virtual ITr2ShaderMaterial* GetPickingEffect( PickComponents pass );
	virtual bool RenderPickingAreasForComponents( PickComponents pass ) const { return true; }
	virtual Tr2PickBuffer& GetPickBuffer( void ) { return m_pickBuffer; }
	virtual unsigned int GetRequiredPasses( PickComponents requestedComponents, PickComponents* passes );
	virtual void DecodeBufferPixel( const void* pBuffer, PickComponents pass, BufferResults& results ) const;

	// Construct a sort key from object & batch groups
	__int64 ConstructKey( unsigned int objectGroup, Tr2InteriorBatchGroup batchGroup );

	// These should be moved over to a smaller per-frame data at some point without the cruft
	void PopulatePerFramePSData( Tr2PerFramePSData &data );
	void PopulatePerFrameVSData( Tr2PerFrameVSData &data );

	void LogNextVisibilityQuery( void );

	// This is a python only wrapper function for just picking an object
	IRoot* PickObjectOnly( int x, int y, TriProjection* proj, TriView* view, TriViewport* vp )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		PickResults results;
		results.components = PICK_OBJECT;
		PickObject( renderContext, x, y, proj, view, vp, results );
		return results.object;
	}

	Vector2 PickObjectUV( int x, int y, TriProjection* proj, TriView* view, TriViewport* vp )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		PickResults results;
		results.components = PICK_UV;
		PickObject( renderContext, x, y, proj, view, vp, results );
		return results.uv;
	}

	bool m_drawSorted;
	unsigned int m_sortedRenderBatchCount;
	unsigned int m_unsortedRenderBatchCount;

	// Enlighten update cut-off depth (distance from nearest visible cell)
	int m_enlightenVisibilityUpdateThreshold;

	PTriCurveSetVector m_curveSets;

	// Per-frame data
	Tr2PerFrameVSData m_perFrameVSData;
	Tr2PerFramePSData m_perFramePSData;

	// Mouse picking
	Tr2PickBuffer m_pickBuffer;
	Tr2ShaderMaterialPtr m_pickEffect;

	Be::Time m_lastUpdateTime;

	float m_apexLODResourceBudget;
	float m_apexLODResourceBudgetConsumed;

	// debug
	bool m_renderDebugInfo;
	bool m_renderCullingInfo;
    TriLineSetPtr m_debugLines;

	bool m_displayDynamics;
	// Used during shadow rendering to skip statics for certain lights
	bool m_displayStatics;

	// Set when generating the shadows for a particular light
	ITr2InteriorLight* m_lightGeneratingShadows;

	// visualization
	VisualizeMethod m_visualizeMethod;
	Tr2EffectPtr m_visualizerOverride;
	bool		 m_visualizerOverrideApplyPS;
	Tr2ShaderMaterialPtr m_visualizerEffects[VM_COUNT];

	// SH scale factor
	float m_shScale;

	// Ragdoll simulation
	ITr2PhysicsUpdaterPtr m_ragdollScene;

	// Focal position (reference point to use when calculating shadow caster importance)
	Vector3 m_shadowFocalPosition;
	// Flag to use m_shadowFocalPosition (if true) or camera position (if false)
	// to calculate shadow caster importance
	bool m_useShadowFocalPosition;

	// Previous frame time (for measuring frame length for shadow interpolation)
	Be::Time m_prevTime;
	// Time to completely fade shadow in or out
	float m_shadowFadeTime;

	// Maximum number of shadows to update per frame (including LOD switches)
	unsigned int m_shadowsUpdatesPerFrame;
	// Maximum number of shadow LOD switches per frame
	unsigned int m_shadowsLODSwitchesPerFrame;
	// Enable shadow resolution LODs
	bool m_useShadowLOD;
	// Depth buffer used during spot light shadow updates
	Tr2DepthStencilAL m_shadowMapDepthBuffer;
	// Temporary textures to use for shadow map rendering/blurring
	Tr2RenderTargetAL m_shadowMapTemporaryRTs[2];
	// Filter effect used for shadow map blurring
	Tr2EffectPtr m_shadowFilter;
	// Filter size variable handle used for shadow map blurring
	Tr2Variable m_shadowFilterVar;
	// Texture area variable handle used for shadow map blurring
	Tr2Variable m_shadowFilterAreaVar;

	// N dot L lookup texture (used during lighting pass)
	TriTextureResPtr m_nDotLTexture;
	// N dot L lookup texture variable handle (used during lighting pass)
	TriVariable* m_nDotLTextureHandle;

	// Maximum fog density amount (from 0 to 1)
	float m_maxFogAmount;
	// Distance where fog reaches maximum density
	float m_maxFogDistance;
	// Distance where fog density starts to grow from 0
	float m_minFogDistance;
	// Fog color
	Color m_fogColor;

	PTr2TextureAtlasVector m_shadowAtlases;
	// SH lighting solver
	Tr2InteriorSHLightingSolver m_shSolver;
	// Enable/disable solving for SH coefficients
	bool m_enableSHSolver;

	// --------------------------------------------------------------------------------------
	// Description:
	//   Per-shadow map information to enable shadow re-rendering on SLI architectures. 
	// --------------------------------------------------------------------------------------
	struct ShadowReRenderInfo
	{
		// Light source generating shadow
		BlueWeakRef<ITr2InteriorLight> lightSource;
		// Index of the shadow map in light source
		unsigned shadowMapIndex;

		// --------------------------------------------------------------------------------------
		// Description:
		//   Comparison operator (so that the structure can be used as a key in std::map. 
		// Arguments:
		//   info - Object to compare with
		// Return Value:
		//   true If this is less than info in some ordering
		//   false Otherwise
		// --------------------------------------------------------------------------------------
		bool operator<( const ShadowReRenderInfo& info ) const
		{
			if( lightSource < info.lightSource )
			{
				return true;
			}
			if( shadowMapIndex < info.shadowMapIndex )
			{
				return true;
			}
			return false;
		}
	};

	// Number of SLI (AFR) GPU groups
	unsigned int m_numSLIGroups;

	// Per-shadow map information (+number of GPUs updated) for shadow re-rendering on SLI architectures
	std::map<ShadowReRenderInfo, unsigned> m_shadowReRenderInfo;

	// Enable/disable Umbra regions of influence for light sources
	bool m_enableROIs;

	bool	m_debugInsideSetLOD;

	Tr2ConstantBufferAL	m_perFramePSBuffer, m_perFrameShadowPSBuffer;
	Tr2ConstantBufferAL	m_perFrameVSBuffer, m_perFrameShadowVSBuffer;
};

TYPEDEF_BLUECLASS( Tr2InteriorScene );

#endif
