#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorScene.h"

#include "TriFrustum.h"
#include "TriProjection.h"
#include "TriSettingsRegistrar.h"
#include "TriView.h"
#include "TriViewport.h"
#include "Apex/Apex.h"
#include "Apex/Tr2ApexScene.h"
#include "Tr2VisibilityResults.h"
#include "Tr2InteriorMirror.h"
#include "Tr2ShaderMaterial.h"
#include "Tr2LitPerObjectData.h"
#include "Tr2AtlasTexture.h"
#include "Tr2InteriorParticleObject.h"
#include "Tr2PushPopDS.h"
#include "Tr2PushPopRT.h"

#include "include/ITr2DebugRenderer.h"
#include "ITr2UmbraUserData.h"

#include "TriLineSet.h"
#include "Tr2InteriorPlaceable.h"
#include "Tr2InteriorStatic.h"
#include "ITr2PhysicsUpdater.h"
#include "Curves/TriCurveSet.h"
#include "Tr2InteriorCell.h"
#include "Tr2InteriorPhysicalPortal.h"
#include "Tr2TextureAtlas.h"
#include "Tr2Effect.h"

using namespace Tr2RenderContextEnum;

extern ITr2DebugRendererPtr g_debugRenderer;

bool g_useRootCell = false;

#if !defined( NDEBUG )
// In release builds, you can turn this on using a trinity setting
bool g_outputEnlightenDebugBuildInfo = false;
#else
// In debug builds, this defaults to true
bool g_outputEnlightenDebugBuildInfo = true;
#endif

TRI_REGISTER_SETTING( "useRootCell", g_useRootCell );
TRI_REGISTER_SETTING( "outputEnlightenDebugBuildInfo", g_outputEnlightenDebugBuildInfo );

extern bool g_enlightenBreakOnErrors;

CCP_STATS_DECLARE( wodInteriorSceneShadowsNeedUpdating, "Trinity/Tr2InteriorScene/ShadowsNeedUpdating", true, CST_COUNTER_LOW, "Number of spotlight shadows that need updating" );
CCP_STATS_DECLARE( wodInteriorSceneShadowsUpdated, "Trinity/Tr2InteriorScene/ShadowsUpdated", true, CST_COUNTER_LOW, "Number of spotlight shadows updated" );
CCP_STATS_DECLARE( wodInteriorSceneIntersectingCellPortals, "Trinity/Tr2InteriorScene/IntersectingCellPortals", true, CST_COUNTER_LOW, "Number of temporary portals to fix intersecting cells" );

BLUE_DEFINE_INTERFACE( ITr2Interior );

const unsigned int INTERIOR_SHADOW_MAP_FOM_SAMPLER = 9;
const unsigned int INTERIOR_SHADOW_MAP_MINMAX_SAMPLER = 9;	// recycle FOM

const unsigned int INTERIOR_SHADOW_MAP0_SAMPLER = 10;
const unsigned int INTERIOR_SHADOW_MAP1_SAMPLER = 11;

const unsigned int INTERIOR_SHADOW_MAP_MAX_RESOLUTION = 1024;
const unsigned int INTERIOR_SHADOW_ATLAS_RESOLUTION = 2048;

// Name of the High-level shader used for picking
static const char* s_pickingEffectName   = "Picking";

// lod resource unloading
float g_wodAvatarResourceUnloadingTimeThreshold = 10.0f;
TRI_REGISTER_SETTING( "wodAvatarResourceUnloadingTimeThreshold", g_wodAvatarResourceUnloadingTimeThreshold );

CcpLogChannel_t g_enlightenBuildChannel = CCP_LOG_DEFINE_CHANNEL( "EnlightenBuild" );

namespace
{
	const char* VISUALIZER_NAME[VM_COUNT] =
	{
		"",
		"Visualizer_White",
		"Visualizer_ObjectNormal",
		"Visualizer_Tangent",
		"Visualizer_Bitangent",
		"Visualizer_TexCoord0",
		"Visualizer_TexCoord1",
		"Visualizer_TexelDensity",
		"Visualizer_NormalMap",
		"Visualizer_DiffuseMap",
		"Visualizer_SpecularMap",
		"Visualizer_Overdraw",
		"Visualizer_EnlightenOnly",
		"Visualizer_EnlightenTargetDetail",
		"Visualizer_EnlightenOutputDensity",
		"Visualizer_EnlightenAlbedo",
		"Visualizer_EnlightenAlbedo",
		"Visualizer_TexCoord1",
		"Visualizer_EnlightenOutputDensity", //naughty pixels
		"Visualizer_EnlightenOutputDensity", //chart viz
		"Visualizer_EnlightenOutputDensity", //target chart viz
		"Visualizer_Depth",
		"Visualizer_AllLighting",
		"Visualizer_LightPrePassNormals",
		"Visualizer_LightPrePassDepth",
		"Visualizer_LightPrePassWorldPosition",
		"Visualizer_LightPrePassLighting",
		"Visualizer_LightPrePassLightOverdraw",
		"Visualizer_LightPrePassLightingDiffuse",
		"Visualizer_LightPrePassLightingSpecular",
		"Visualizer_White",
	};

	TriVariable* GetSunDirWorldHandle()
	{
		static TriVariable* s_sunDirWorldHandle = NULL;

		if( s_sunDirWorldHandle == NULL )
		{
			s_sunDirWorldHandle = GlobalStore().FindVariable( "Sun.DirWorld" );
		}

		return s_sunDirWorldHandle;
	}

	TriVariable* GetSunDiffuseColorHandle()
	{
		static TriVariable* s_handle = NULL;

		if( s_handle == NULL )
		{
			s_handle = GlobalStore().FindVariable( "Sun.WodDiffuseColor" );
		}

		return s_handle;
	}

	TriVariable* GetSunSpecularColorHandle()
	{
		static TriVariable* s_handle = NULL;

		if( s_handle == NULL )
		{
			s_handle = GlobalStore().FindVariable( "Sun.SpecularColor" );
		}

		return s_handle;
	}

	TriVariable* GetAmbientColorHandle()
	{
		static TriVariable* s_handle = NULL;

		if( s_handle == NULL )
		{
			s_handle = GlobalStore().FindVariable( "Scene.AmbientColor" );
		}

		return s_handle;
	}
}

Tr2InteriorScene::Tr2InteriorScene( IRoot* lockobj /*= NULL */ ):
	PARENTLOCK( m_cells ),
	PARENTLOCK( m_portals ),
	PARENTLOCK( m_lights ),
	PARENTLOCK( m_dynamics ),
	PARENTLOCK( m_dynamicsPendingLoad ),
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_filterList ),
	m_renderDebugInfo( false ),
	m_renderCullingInfo( false ),
	m_sunDirection( 0.0f, 0.0f, 1.0f ),
	m_sunDiffuseColor( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_sunSpecularColor( 0.8f, 0.8f, 0.8f, 1.0f ),
	m_ambientColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_enlightenEnvironmentMultiplicationFactor( 1.0f ),
	m_lastUpdateTime( 0 ),
	m_apexLODResourceBudget( 100000.0f ),
	m_apexLODResourceBudgetConsumed( 0.0f ),
	m_visualizeMethod( VM_NONE ),
	m_pickBuffer( NULL, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT, 1 ),
	m_useFilterList( false ),
	m_shadowFocalPosition( 0.0f, 0.0f, 0.0f ),
	m_useShadowFocalPosition( false ),
	m_shadowsUpdatesPerFrame( 2 ),
	m_shadowsLODSwitchesPerFrame( 1 ),
	m_useShadowLOD( true ),	
	m_cameraPortalModel( NULL ),
	m_sceneUseRootCell( true ),
	m_maxFogAmount( 0.0f ),
	m_maxFogDistance( 1000.0f ),
	m_minFogDistance( 0.0f ),
	m_fogColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_renderBackgroundCubeMap( true ),
	PARENTLOCK( m_shadowAtlases ),
	m_debugInsideSetLOD( false ),
	m_enableSHSolver( true ),
	m_enableROIs( true ),
	m_numSLIGroups( 1 )
	, m_sunDiffuseColorVar( "Sun.WodDiffuseColor", m_sunDiffuseColor )
	, m_sunSpecularColorVar( "Sun.SpecularColor", m_sunSpecularColor )	
	, m_ambientColorVar( "Scene.AmbientColor", m_ambientColor )
	, m_cameraPosVar( "Camera.eyePosWorld", Vector3( 0.0f, 0.0f, 0.0f ) )
	, m_shadowFilterVar( "ShadowFilterTexelSize", Vector4( 0.0, 0.0, 0.0, 0.0 ) )
	, m_shadowFilterAreaVar( "ShadowFilterTextureArea", Vector4( 0.0, 0.0, 1.0f, 1.0f ) )
{
	m_backgroundCubeMapVar.Register( "EnvMap1", m_backgroundCubeMapRes );

	GlobalStore().RegisterVariable( "LightPrePassMap", (TriTextureRes*)NULL );
	GlobalStore().RegisterVariable( "LightAccumulationMap", (TriTextureRes*)NULL );
	//GlobalStore().RegisterVariable( "LightAccumulationSpecularMap", m_lightSpecularRenderPassTexture );
	GlobalStore().RegisterVariable( "LightPrePassDepthMap", (TriTextureRes*)NULL );

	// Create render batch accumulators
	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	m_primaryRenderBatches = CCP_NEW( "Tr2InteriorScene/m_primaryRenderBatches" ) TriRenderBatchAccumulator<Tr2IntKeyGenerator>( allocator );
	m_transparentBatchStore = CCP_NEW( "Tr2InteriorScene/m_transparentBatchStore" ) TriRenderBatchStore( allocator );
	m_shadowBatches = CCP_NEW( "Tr2InteriorScene/m_shadowBatches" ) TriRenderBatchAccumulator<Tr2IntKeyGenerator>( allocator );
	m_opaquePickingBatches = CCP_NEW( "Tr2InteriorScene/m_opaquePickingBatches" ) TriRenderBatchAccumulator<>( allocator );
	m_pickingBatches = CCP_NEW( "Tr2InteriorScene/m_pickingBatches" ) TriRenderBatchAccumulator<>( allocator );

	m_prepassBatches = CCP_NEW( "Tr2InteriorScene/m_prepassBatches" ) TriRenderBatchAccumulator<Tr2IntKeyGenerator>( allocator );

	// Initialize accumulator handles to NULL - these can point to the above accumulators, depending on the Umbra scene query type
	m_activePrimaryRenderBatches = NULL;
	m_activeTransparentBatchStore = NULL;

	// picking
	m_pickBuffer.PrepareResources();
	m_pickEffect.CreateInstance();
	m_pickEffect->SetHighLevelShaderName( s_pickingEffectName );
	Tr2ShaderSituation situation;
	m_pickEffect->BindLowLevelShader( situation );
	m_pickBuffer.SetClearColor( 0x0 );

	// Variable Handles
	Vector3 dir( XMVectorMultiply(
		XMVector3Normalize( m_sunDirection ),
		XMVectorReplicate( -1.0f ) ) );

	m_sunDirectionVar.Register( "Sun.DirWorld", dir );	
	GlobalStore().RegisterVariable( "PickingComponents", Vector4() );
	
	// create debug renderer
	m_debugLines.CreateInstance();

	// Setup Umbra culling camera for main scene
	m_sceneCamera.SetScreenSize( Tr2Renderer::GetRenderTargetWidth(), Tr2Renderer::GetRenderTargetHeight() );

	// Setup Umbra culling camera for shadow casters
	m_shadowCamera.EnableVirtualPortals( false );

	m_displayDynamics = true;
	m_displayStatics = true;
	m_lightGeneratingShadows = NULL;

	m_enlightenInputEnvironmentLightingCache.Resize( 24, Geo::VZero() );

	m_enableLightCulling = true;

	// List notify
	m_lights.SetNotify( this );
	m_dynamics.SetNotify( this );
	m_cells.SetNotify( this );

	m_visualizerEffects[0] = NULL;
	for( unsigned int i = 1; i < VM_COUNT; ++i )
	{
		m_visualizerEffects[i].CreateInstance();
		m_visualizerEffects[i]->SetHighLevelShaderName( VISUALIZER_NAME[i] );
		Tr2ShaderSituation situation;
		m_visualizerEffects[i]->BindLowLevelShader( situation );
		// m_visualizerEffects[i]->GetEffectRes()->AddNotifyTarget( this );
	}

	// Initialize the Umbra root cell to NULL
	m_rootCell = NULL;
	m_rootObject = NULL;
	m_rootModel = NULL;

	// Initialize the camera cell to NULL
	m_currentCameraCell = NULL;

	// Initialize SH scale factor
	m_shScale = 1.0f;

	// Draw sorted by default
	m_drawSorted = true;

	// Enlighten cutoff depth
	m_enlightenVisibilityUpdateThreshold = 0;

	PrepareResources();

	m_shadowFilter.CreateInstance();
	m_shadowFilter->SetEffectPathName( "res:/Graphics/Effect/Managed/Interior/Shadows/ShadowFilter.fx" );

	BeResMan->GetResource( "res:/Texture/Global/NdotLLibrary.png", "", m_nDotLTexture );
	m_nDotLTextureHandle = GlobalStore().RegisterPlaceholderTextureVariable( "ColorNdotLLookupMap" );

    // only create apex scene if apex is initialised which at this point is denoted by having the sdk loaded and available
    if( g_Tr2Apex && g_Tr2Apex->GetApexSDK() )
	{
		m_apexScene.CreateInstance();
		m_apexScene->CreateScene();
	}
}

Tr2InteriorScene::~Tr2InteriorScene()
{
	if( m_apexScene )
	{
		m_apexScene->DeleteScene();
	}

	if( m_backgroundCubeMapRes )
	{
		m_backgroundCubeMapRes->RemoveNotifyTarget( this );
	}

	CCP_DELETE( m_primaryRenderBatches );
	CCP_DELETE( m_transparentBatchStore );
	CCP_DELETE( m_shadowBatches );
	CCP_DELETE( m_pickingBatches );
	CCP_DELETE( m_opaquePickingBatches );
	CCP_DELETE( m_prepassBatches );

	// release debug renderer
	if( m_debugLines )
	{
		m_debugLines = NULL;
	}

    m_pickBuffer.ReleaseResources( TRISTORAGE_ALL );

	for( unsigned int i = 1; i < VM_COUNT; ++i )
	{
		// m_visualizerEffects[i]->GetEffectRes()->RemoveNotifyTarget( this );
	}

	// Release the root cell & portals
	ReleaseRootCell();
	ReleaseRootPortals();

	if( m_cameraPortalModel )
	{
		m_cameraPortalModel->release();
		m_cameraPortalModel = NULL;
	}
	for( std::vector<Umbra::PhysicalPortal*>::iterator it = m_cameraPortals.begin(); it != m_cameraPortals.end(); ++it )
	{
		( *it )->release();
	}
	m_cameraPortals.clear();
}

bool Tr2InteriorScene::Initialize()
{
	SetBackgroundCubemapResPath();
	return true;
}

bool Tr2InteriorScene::OnModified( Be::Var* value )
{
    if( IsMatch( value, m_visualizeMethod ) )
    {
        SetVisualizeMethod();
    }
	else if( IsMatch( value, m_backgroundCubeMapPath ) )
	{
		SetBackgroundCubemapResPath();
		// If the texture is already loaded, or empty we need to do this here
		if( m_backgroundCubeMapPath.empty() || ( m_backgroundCubeMapRes && m_backgroundCubeMapRes->IsPrepared() ) )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			SetEnvironmentCubeMapToEnlighten( renderContext );
		}
		// If it's not loaded yet, this will happen when we get the async callback
	}
	else if( IsMatch( value, m_enlightenEnvironmentMultiplicationFactor ) )
	{
		UpdateEnvironmentLightingFromCacheWithMultiplier();
	}
	else if( IsMatch( value, m_shScale ) )
	{
		UpdateSHScaleFactor();
	}
	else if( IsMatch( value, m_enableROIs ) )
	{
		for( PITr2InteriorLightVector::iterator it = m_lights.begin(); it != m_lights.end(); ++it )
		{
			( *it )->EnableROI( m_enableROIs );
		}
	}

    return true;
}

// ---------------------------------------------------------------
void Tr2InteriorScene::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* currvalue, const IList* theList )
{
	if( theList == &m_lights )
	{
		OnLightsListModified( event, key, key2, currvalue );
	}
	else if( theList == &m_dynamics )
	{
		CCP_ASSERT( !m_debugInsideSetLOD );
		OnDynamicsListModified( event, key, key2, currvalue );
	}
	else if( theList == &m_cells )
	{
		// Set the dirty flags on all the lights
		for( PITr2InteriorLightVector::iterator it = m_lights.begin();
			it != m_lights.end(); ++it )
		{
			( *it )->SetDirtyFlag( true );
		}

		// Set the dirty flags on all the dynamics
		for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin();
			it != m_dynamics.end(); ++it )
		{
			( *it )->SetDirtyFlag( true );
		}

		if( ( event & BELIST_LOADING ) == 0  )
		{
			// Respond to an item removal event
			if( ( event & BELIST_EVENTMASK ) == BELIST_REMOVED )
			{
				if( currvalue )
				{
					// See if the removed item is a cell
					Tr2InteriorCell* cell = NULL;
					if( currvalue->QueryInterface( BlueInterfaceIID<Tr2InteriorCell>(), ( void** )&cell ) )
					{
						for( PITr2InteriorLightVector::iterator it = m_lights.begin(); it != m_lights.end(); ++it )
						{
							( *it )->CellRemoved( cell );
						}

						for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin();
							 it != m_dynamics.end(); ++it )
						{
							( *it )->CellRemoved( cell );
						}

						cell->Unlock();
					}
				}
				else if( ( event & BELIST_EVENTMASK ) == BELIST_INSERTED )
				{
					Tr2InteriorCell* cell = NULL;
					if( currvalue->QueryInterface( BlueInterfaceIID<Tr2InteriorCell>(), ( void** )&cell ) )
					{
						Geo::GeoArray<Geo::v128> postMultipliedEnvironmentLighting( 24, Geo::VZero() );
						Geo::v128 multiplier = Geo::VBroadcast( m_enlightenEnvironmentMultiplicationFactor );

						for( int i = 0; i < 24;  ++i )
						{
							using namespace Geo;
							postMultipliedEnvironmentLighting[i] = multiplier * m_enlightenInputEnvironmentLightingCache[i];
						}

						for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
						{
							( *it )->SetEnvironmentCube( postMultipliedEnvironmentLighting.GetArrayConst() );
						}

						cell->Unlock();
					}
				}
			}
		}
	}
}

void Tr2InteriorScene::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		m_perFramePSBuffer.Destroy();
		m_perFrameShadowPSBuffer.Destroy();
		m_perFrameVSBuffer.Destroy();
		m_perFrameShadowVSBuffer.Destroy();
	
		m_shadowMapDepthBuffer.Destroy();
		m_shadowMapTemporaryRTs[0].Destroy();
		m_shadowMapTemporaryRTs[1].Destroy();
	}
}

bool Tr2InteriorScene::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_shadowMapDepthBuffer.IsValid() )
	{
		CR_RETURN_VAL( 
				m_shadowMapDepthBuffer.Create( 
							INTERIOR_SHADOW_MAP_MAX_RESOLUTION, 
							INTERIOR_SHADOW_MAP_MAX_RESOLUTION, 
							DSFMT_D24S8, 
							1, 
							0, 
							renderContext )
				, false );
	}
	if( !m_shadowMapTemporaryRTs[0].IsValid() )
	{
		CR_RETURN_VAL( 
				m_shadowMapTemporaryRTs[0].Create( 
							INTERIOR_SHADOW_MAP_MAX_RESOLUTION, 
							INTERIOR_SHADOW_MAP_MAX_RESOLUTION, 
							1, 
							PIXEL_FORMAT_R16G16_UNORM, 
							1, 
							0, 
							renderContext )
				, false );
	}
	if( !m_shadowMapTemporaryRTs[1].IsValid() )
	{
		CR_RETURN_VAL( 
				m_shadowMapTemporaryRTs[1].Create( 
							INTERIOR_SHADOW_MAP_MAX_RESOLUTION, 
							INTERIOR_SHADOW_MAP_MAX_RESOLUTION, 
							1, 
							PIXEL_FORMAT_R16G16_UNORM, 
							1, 
							0, 
							renderContext )
				, false );
	}

	m_numSLIGroups = 1;
	CR_RETURN_VAL( 
		renderContext.GetAFRGroupCount( m_numSLIGroups )
		, false );

	return true;
}

void Tr2InteriorScene::SetVisualizationMode( int visualizationMode )
{
	m_visualizeMethod = VisualizeMethod( visualizationMode );
	SetVisualizeMethod();
}

void Tr2InteriorScene::Update( Be::Time time )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( time == m_lastUpdateTime)
		// NB: Multiple calls on the same 'frame' should be ignored,
		//     but are not strictly errors, according to Dan Speed.
		return;

    // create the apex scene when it is intialised, should probably be done through the same python event that inits the sdk
    // but that is incredibly byzantine at the moment
    if( !m_apexScene && g_Tr2Apex && g_Tr2Apex->GetApexSDK() )
    {
        m_apexScene.CreateInstance();
		m_apexScene->CreateScene();
    }

	if( m_apexScene )
	{
		m_apexScene->PreUpdate( time, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}

	m_lastUpdateTime = time;

	Vector3 dir( XMVectorMultiply(
		XMVector3Normalize( m_sunDirection ),
		XMVectorReplicate( -1.0f ) ) );

	m_sunDirectionVar		= dir;
	m_sunDiffuseColorVar	= m_sunDiffuseColor;
	m_sunSpecularColorVar	= m_sunSpecularColor;
	m_ambientColorVar		= m_ambientColor;
	m_cameraPosVar			= Tr2Renderer::GetViewPosition();

	if( m_ragdollScene != NULL )
    {
		m_ragdollScene->PrePhysics( time );
	}

	{
		CCP_STATS_ZONE( "UpdateCurves" );
		for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
		{
			( *it )->Update( TimeAsDouble( time ) );
		}
	}

	{
		CCP_STATS_ZONE( "UpdateBoundingBoxes" );
		// Update cell bounding boxes (so lights and dynamics can add on the first Update call)
		for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
		{
			( *it )->UpdateBoundingBox();
		}
	}

	{
		CCP_STATS_ZONE( "PendingLoads" );
		// Add any dynamics that have finished loading
		std::vector<ITr2InteriorDynamic*> dynamicsToRemove;
		for( ssize_t index = 0; index < m_dynamicsPendingLoad.GetSize(); ++index )
		{
			// If the add succeeded, remove from the pending load list
			if( m_dynamicsPendingLoad[index]->AddToScene( m_apexScene ) )
			{
				dynamicsToRemove.push_back( m_dynamicsPendingLoad[index] );
			}
		}
		for( std::vector<ITr2InteriorDynamic*>::iterator it = dynamicsToRemove.begin();
			 it != dynamicsToRemove.end(); ++it )
		{
			ssize_t index = m_dynamicsPendingLoad.FindKey( ( *it ) );
			if( index != -1 )
			{
				m_dynamicsPendingLoad.Remove( index );
			}
		}
	}

	// Update lights for Umbra
	UpdateLights();

	// Update dynamics for Umbra
	UpdateDynamics();

	// Update moved cells
	UpdateCells();

	if( !m_enlightenUpdateTaskManager.IsExecuting() )
	{
		CCP_STATS_ZONE( "EnlightenUpdateSynchronous" );

		// Fetch results of the previous run
		for( Tr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
		{
			( *it )->UpdateEnlightenWorkspaceList( m_enlightenVisibilityUpdateThreshold );
		}
		for( Tr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
		{
			( *it )->ProcessEnlightenResults( m_enlightenUpdateTaskManager );
		}
		// Unlock cells/systems used during threaded Enlighten update
		m_enlightenUpdateTaskManager.InvalidateResult();

		// Prepare data for a new Enlighten compute
		for( Tr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
		{
			( *it )->UpdateEnlightenWorkspace( m_enlightenVisibilityUpdateThreshold );
		}
		for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
		{
			(* it )->SetSHSampleIndex( Tr2IntEnlightenTaskManager::UninitializedIndex );
		}
		for( Tr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
		{
			( *it )->PrepareUpdateEnlighten( m_enlightenVisibilityUpdateThreshold, m_enlightenUpdateTaskManager );
		}

		// Fork Enlighten update threads
		m_enlightenUpdateTaskManager.Execute();
	}

	{
		CCP_STATS_ZONE( "CellUpdate" );
		// since everything in the scene is in a cell, this ::Update spreads to all renderables
		for( Tr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
		{
			( *it )->Update( time, m_enlightenVisibilityUpdateThreshold );
		}
	}

	{
		CCP_STATS_ZONE( "PrePhysicsUpdate" );
		// Do the pre-physics update on the dynamics
		// Note: this used to happen in the cell pre-physics update, but that can cause double animation ticks
		// when a skinned object is intersecting multiple cells (e.g. crossing a portal)
		for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
		{
			( *it )->PrePhysicsUpdate( time );
		}
	}

	// Reset light statuses, since Enlighten updates are done at this point
	for( PITr2InteriorLightVector::iterator it = m_lights.begin(); it != m_lights.end(); ++it )
	{
		( *it )->ResetStaticChangedFlag();
	}

	if( m_ragdollScene != NULL )
    {
		m_ragdollScene->SimulatePhysics();
	}

	{
		CCP_STATS_ZONE( "PostPhysicsUpdate" );
		// Do the post-physics update on the dynamics
		// Note: this used to happen in the cell post-physics update, but that can cause double animation ticks
		// when a skinned object is intersecting multiple cells (e.g. crossing a portal)
		for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
		{
			( *it )->PostPhysicsUpdate( time, m_apexScene );
		}
	}

	{
		CCP_STATS_ZONE( "Update" );
		for( PITr2InteriorLightVector::iterator it = m_lights.begin(); it != m_lights.end(); ++it )
		{
			( *it )->Update( time );
		}
	}

	for( PTr2InteriorPhysicalPortalVector::iterator it = m_portals.begin(); it != m_portals.end(); ++it )
	{
		( *it )->AddDoorToUmbra();
	}

	if( m_apexScene )
	{
		m_apexScene->PostUpdate( time, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}
}

void Tr2InteriorScene::Render( Tr2RenderContext& renderContext )
{
	D3DPERF_EVENT( L"Tr2InteriorScene::Render" );

	// If we don't have a visibilityResults object (i.e. the VisibilityQuery renderstep
	// wasn't called), then create one and call VisibilityQuery now.
	if( !m_visibilityResults )
	{
		m_visibilityResults.CreateInstance();
	}

	// Execute Umbra visibility query
	VisibilityQuery( m_visibilityResults );

	if( m_apexScene )
	{
		m_apexScene->PreRender( m_lastUpdateTime, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}

	// Only render the cubemaps here
	//RenderShadowMaps();

	// Do the full-forward render
	RenderFullForward( renderContext );

	// debug info
	RenderDebugInfo( renderContext );

	// Now update the visibility of all cells
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		if( ( *it )->GetVisibility() )
		{
			( *it )->DetermineVisibility( 0 );
		}
	}

	// Clear lights
	m_activeLightSet.Clear();

	if( m_apexScene )
	{
		m_apexScene->PostRender( m_lastUpdateTime, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Issues a visibility query and stores the results in the given visibility results set.
//   This function returns early and issues a log error message if the results set is
//   NULL.  The interior scene holds a pointer to the results set, so it can be reused
//   elsewhere.
// Arguments:
//   results - The results set populated by the visibility query
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::VisibilityQuery( Tr2VisibilityResults* results )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !results )
	{
		CCP_LOGERR( "Attempt to issue a visibility query on an interior scene with a NULL "
			"result set!" );
		return;
	}

	m_visibilityResults = results;
	m_visibilityResults->Clear();

	// something there to render?
	if( !m_cells.size() )
	{
		return;
	}

	// Setup the root cell and its portals
	if( g_useRootCell && m_sceneUseRootCell )
	{
		// Create the root cell, if necessary
		if( !m_rootCell )
		{
			CreateRootCell();
		}

		UpdateRootPortals();
	}
	else
	{
		// Clean-up the root cell, if necessary
		ReleaseRootCell();
		ReleaseRootPortals();
	}

	// Choose LOD for dynamics
	TriFrustum frustum;
	frustum.DeriveFrustum(
		&Tr2Renderer::GetViewTransform(),
		&Tr2Renderer::GetViewPosition(),
		&Tr2Renderer::GetProjectionTransform(),
		Tr2Renderer::GetViewport()
	);

    //this is to assist in tracking down the crash that's been popping up in this function.
    //The bools are here just in case logging breaks during the crash
    const size_t dynamicsSize = m_dynamics.size();

	m_debugInsideSetLOD = true;

	for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
        if( dynamicsSize != m_dynamics.size() )
        {
            CCP_LOGERR("VisibilityQuery-m_dynamics changed size while we iterated over it! (before setlod)");
        }
		if( !*it )
		{
            CCP_LOGERR("VisibilityQuery-null pointer in m_dynamics");
		}
		( *it )->SetLOD( &frustum );
        if( dynamicsSize != m_dynamics.size() )
        {
            CCP_LOGERR("VisibilityQuery-m_dynamics changed size while we iterated over it! (after setlod)");
        }
	}

	m_debugInsideSetLOD = false;

	// Prepare for Umbra visibility query
	m_cameraToWorldMatrix = Tr2Renderer::GetViewTransform();
	m_sceneCamera.SetViewParameters( Tr2Renderer::GetInverseViewTransform(),
		Tr2Renderer::GetProjectionTransform() );
	m_sceneCamera.SetScreenSize( Tr2Renderer::GetViewport().width, Tr2Renderer::GetViewport().height );
	m_gatherShadowCasterBatches = false;
	m_visibilityQueryType = PRIMARY_QUERY;

	ResolveVisibility( m_sceneCamera );
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns a visibility results set object to the interior scene
// Arguments:
//   visibilityResults - The results set object to assign to the interior scene
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::SetVisibilityResults( Tr2VisibilityResults* visibilityResults )
{
	m_visibilityResults = visibilityResults;
}

void Tr2InteriorScene::ResolveVisibility( Tr2UmbraCamera& camera )
{
	m_visitedObjects.push_back( new std::unordered_set<IRoot*> );

	Umbra::Cell* cameraCell = GetCameraCell();

	if( m_rootCell != NULL && cameraCell == m_rootCell )
	{
		for( auto it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
		{
			Tr2InteriorPlaceable* placeable = dynamic_cast<Tr2InteriorPlaceable*>( *it );
			if( placeable )
			{
				placeable->EnableMirrorPortals( false );
			}
		}
	}

	// Execute Umbra visibility query
	camera.ResolveVisibility( this, cameraCell, 1 );

	if( m_rootCell != NULL && cameraCell == m_rootCell )
	{
		for( auto it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
		{
			Tr2InteriorPlaceable* placeable = dynamic_cast<Tr2InteriorPlaceable*>( *it );
			if( placeable )
			{
				placeable->EnableMirrorPortals( true );
			}
		}
	}

	for( auto i = m_visitedObjects.begin(); i != m_visitedObjects.end(); ++i )
	{
		delete *i;
	}
	m_visitedObjects.clear();
}

ITr2MultiPassScene::RenderPassResult Tr2InteriorScene::RenderPass( PassType pass, Tr2RenderContext& renderContext )
{
	switch( pass )
	{
	case RP_BEGIN_RENDER:
		BeginRender( renderContext );
		break;
	case RP_PRE_PASS:
		RenderPrePass( renderContext );
		break;
	case RP_LIGHT_PASS:
		RenderLightPass( renderContext );
		break;
	case RP_GATHER_PASS:
		RenderGatherPass( renderContext );
		break;
	case RP_FLARE_PASS:
		RenderFlarePass( renderContext );
		break;
	case RP_END_RENDER:
		EndRender( renderContext );
		break;
	}

	return PASS_RESULT_OK;
}

void Tr2InteriorScene::BeginRender( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	D3DPERF_EVENT( L"BeginRender" );

	if( m_apexScene )
	{
		m_apexScene->PreRender( m_lastUpdateTime, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}

	RenderShadowMaps( renderContext );
}

void Tr2InteriorScene::RenderPrePass( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	D3DPERF_EVENT( L"Tr2InteriorScene::RenderPrePass" );

	// set per-frame data
	PopulatePerFramePSData( m_perFramePSData );
	PopulatePerFrameVSData( m_perFrameVSData );

	{
		D3DPERF_EVENT( L"Set per-frame shader constants" );
		FillAndSetConstants( m_perFrameVSBuffer, m_perFrameVSData, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
		FillAndSetConstants( m_perFramePSBuffer, m_perFramePSData, PIXEL_SHADER , Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
	}

	// Gather batches for the pre-pass
	m_cameraToWorldMatrix = Tr2Renderer::GetViewTransform();
	m_activePrimaryRenderBatches = m_prepassBatches;
	m_activeTransparentBatchStore = NULL;
	GatherPrePassBatches( m_visibilityResults );

	// Render the geometry
	RenderGeometry( NULL, renderContext );

	// Clear the pre-pass batches
	m_prepassBatches->Clear();
}

void Tr2InteriorScene::RenderLightPass( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	D3DPERF_EVENT( L"Tr2InteriorScene::RenderLightPass" );

	// set per-frame data
	PopulatePerFramePSData( m_perFramePSData );
	PopulatePerFrameVSData( m_perFrameVSData );

	{
		D3DPERF_EVENT( L"Set per-frame shader constants" );
		FillAndSetConstants( m_perFrameVSBuffer, m_perFrameVSData, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
		FillAndSetConstants( m_perFramePSBuffer, m_perFramePSData, PIXEL_SHADER , Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
	}

	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	TriRenderBatchAccumulator<Tr2IntKeyGenerator> lightBatches( allocator );

	// Gather light batches
	m_cameraToWorldMatrix = Tr2Renderer::GetViewTransform();
	m_activePrimaryRenderBatches = &lightBatches;
	m_activeTransparentBatchStore = m_transparentBatchStore;
	GatherLightBatches( m_visibilityResults );
	lightBatches.Finalize();

	renderContext.m_esm.BeginManagedRendering();

	m_nDotLTextureHandle->SetValue( m_nDotLTexture->GetTexture() );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_LIGHT );
	renderContext.SetReadOnlyDepth( true );
	if( m_visualizeMethod == VM_LIGHT_PRE_PASS_LIGHT_OVERDRAW )
	{
		renderContext.m_esm.RenderBatchesWithOverride( &lightBatches, m_visualizerEffects[VM_LIGHT_PRE_PASS_LIGHT_OVERDRAW], Tr2EffectStateManager::OM_DO_NOTHING );
	}
	else
	{
		renderContext.m_esm.RenderLightBatches( &lightBatches );
	}
	renderContext.m_esm.UnsetAllTextures();
	renderContext.SetReadOnlyDepth(	false );

	lightBatches.Clear();
	m_transparentBatchStore->Clear();

	// Restore original viewport and per-frame data
	{
		D3DPERF_EVENT( L"Cleanup State" );

		unsigned int width, height;
		Tr2Renderer::GetBackBufferDimensions( width, height );

		renderContext.SetScissorRect( 0, 0, width, height );
		renderContext.SetRenderState( RS_SCISSORTESTENABLE, FALSE );
		renderContext.m_esm.SetInvertedCullMode( false );
		renderContext.SetRenderState( RS_CLIPPLANEENABLE, 0x0 );
		renderContext.SetRenderState( RS_STENCILENABLE, FALSE );
	}

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
	renderContext.m_esm.EndManagedRendering();
}

void Tr2InteriorScene::RenderGatherPass( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	D3DPERF_EVENT( L"Tr2InteriorScene::RenderGatherPass" );

	// set per-frame data
	PopulatePerFramePSData( m_perFramePSData );
	PopulatePerFrameVSData( m_perFrameVSData );

	{
		D3DPERF_EVENT( L"Set per-frame shader constants" );
		FillAndSetConstants( m_perFrameVSBuffer, m_perFrameVSData, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
		FillAndSetConstants( m_perFramePSBuffer, m_perFramePSData, PIXEL_SHADER , Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
	}

	// Update variable store
	m_backgroundCubeMapVar = m_backgroundCubeMapRes;
	
	// Gather geometry batches
	m_activePrimaryRenderBatches = m_primaryRenderBatches;
	m_activeTransparentBatchStore = m_transparentBatchStore;
	m_cameraToWorldMatrix = Tr2Renderer::GetViewTransform();

	m_shSolver.Clear();
	GatherPrepassForwardBatches( m_visibilityResults );
	if( m_enableSHSolver )
	{
		m_shSolver.Solve( m_lights, renderContext );
	}

	// Render geometry
	renderContext.SetReadOnlyDepth( true );
	if( m_visualizeMethod == VM_OCCLUSION )
	{
		RenderGeometry( NULL, renderContext );
		ITr2ShaderState * shader = m_visualizerEffects[VM_OCCLUSION]->GetShaderStateInterface();
		if( shader )
		{
			shader->ApplyAllStateForPass( 0, renderContext );
			m_visualizerEffects[VM_OCCLUSION]->ApplyMaterialDataForPass( 0, renderContext );

			uint32_t oldCullState;
			uint32_t oldZEnableState;
			renderContext.GetRenderState( RS_CULLMODE, &oldCullState);
			renderContext.GetRenderState( RS_ZENABLE, &oldZEnableState);

			ON_BLOCK_EXIT( [&]{ renderContext.SetRenderState( RS_CULLMODE, oldCullState ); } );
			ON_BLOCK_EXIT( [&]{ renderContext.SetRenderState( RS_ZENABLE, oldZEnableState ); } );

			renderContext.SetRenderState( RS_ZENABLE, FALSE );
			renderContext.SetRenderState( RS_CULLMODE, D3DCULL_NONE );
			renderContext.SetRenderState( RS_FILLMODE, Tr2RenderContextEnum::FM_WIREFRAME );

			for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
			{
				( *it )->RenderOcclusionGeometry();
			}

			renderContext.SetRenderState( RS_FILLMODE, Tr2RenderContextEnum::FM_SOLID );
		}
	}
	else
	{
		RenderGeometry( m_visualizerEffects[m_visualizeMethod == VM_LIGHT_PRE_PASS_LIGHT_OVERDRAW ? VM_LIGHT_PRE_PASS_LIGHTING : m_visualizeMethod], renderContext );
	}
	renderContext.m_esm.UnsetAllTextures();
	renderContext.SetReadOnlyDepth( false );

	// Clear batches
	m_primaryRenderBatches->Clear();
	m_transparentBatchStore->Clear();

	// Clear the SH and unset the variables
	m_shSolver.Clear();

	// debug info
	RenderDebugInfo( renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Performs render flare pass. Gathers flare batches from light sources and renders them.
//   Ignores mirrors and skips itself when any visualization is active.
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::RenderFlarePass( Tr2RenderContext& renderContext )
{
	// Disable flare pass when any visualization is active
	if( m_visualizeMethod != VM_NONE )
	{
		return;
	}

	CCP_STATS_ZONE( __FUNCTION__ );

	D3DPERF_EVENT( L"Tr2InteriorScene::RenderFlarePass" );

	// set per-frame data
	PopulatePerFramePSData( m_perFramePSData );
	PopulatePerFrameVSData( m_perFrameVSData );

	{
		D3DPERF_EVENT( L"Set per-frame shader constants" );
		FillAndSetConstants( m_perFrameVSBuffer, m_perFrameVSData, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
		FillAndSetConstants( m_perFramePSBuffer, m_perFramePSData, PIXEL_SHADER , Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
	}

	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	TriRenderBatchAccumulator<Tr2IntKeyGenerator> lightBatches( allocator );

	// Gather light batches
	m_cameraToWorldMatrix = Tr2Renderer::GetViewTransform();
	m_activePrimaryRenderBatches = &lightBatches;
	m_activeTransparentBatchStore = m_transparentBatchStore;
	GatherFlareBatches( m_visibilityResults );
	lightBatches.Finalize();

	renderContext.m_esm.BeginManagedRendering();

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );

	renderContext.SetReadOnlyDepth( true );
	renderContext.m_esm.RenderBatches( &lightBatches );
	renderContext.m_esm.UnsetAllTextures();
	renderContext.SetReadOnlyDepth( false );
	lightBatches.Clear();
	m_transparentBatchStore->Clear();

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
	renderContext.m_esm.EndManagedRendering();
}

void Tr2InteriorScene::EndRender( Tr2RenderContext& renderContext )
{
	// Now update the visibility of all cells
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		if( ( *it )->GetVisibility() )
		{
			( *it )->DetermineVisibility( 0 );
		}
	}

	// Clear lights
	m_activeLightSet.Clear();

	if( m_apexScene )
	{
		m_apexScene->PostRender( m_lastUpdateTime, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}
}

void Tr2InteriorScene::RenderFullForward( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	D3DPERF_EVENT( L"Tr2InteriorScene::RenderFullForward" );

	// Clear the SH and unset the variables
	m_shSolver.Clear();

	// Update variable store
	m_backgroundCubeMapVar = m_backgroundCubeMapRes;
	
	// set per-frame data
	PopulatePerFramePSData( m_perFramePSData );
	PopulatePerFrameVSData( m_perFrameVSData );

	{
		D3DPERF_EVENT( L"Set per-frame shader constants" );
		FillAndSetConstants( m_perFrameVSBuffer, m_perFrameVSData, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
		FillAndSetConstants( m_perFramePSBuffer, m_perFramePSData, PIXEL_SHADER , Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
	}

	// Gather geometry batches
	m_activePrimaryRenderBatches = m_primaryRenderBatches;
	m_activeTransparentBatchStore = m_transparentBatchStore;
	m_cameraToWorldMatrix = Tr2Renderer::GetViewTransform();
	GatherFullForwardBatches( m_visibilityResults );

	if( m_enableSHSolver )
	{
		m_shSolver.Solve( m_lights, renderContext );
	}

	// Render geometry
	RenderGeometry( m_visualizerEffects[m_visualizeMethod], renderContext );

	// Clear batches
	m_primaryRenderBatches->Clear();
	m_transparentBatchStore->Clear();

	// Clear the SH and unset the variables
	m_shSolver.Clear();
}

// --------------------------------------------------------------------------------------
// Description
//   This function queues up a background cubemap batch in the supplied accumulator.
// Arguments:
//   batches - The accumulator
// See Also:
//   OnQueryBeginForwardPass, OnStencilMaskForwardPass
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::PrepareBackgroundCubemapBatch( ITriRenderBatchAccumulator* batches )
{
	// Verify that we actually want to render the background effect, that we have a background effect & that the accumulator is good
	if( m_renderBackgroundCubeMap && m_backgroundEffect && batches )
	{
		// Allocate a batch
		Tr2InteriorBackgroundCubemapBatch* batch =
			batches->Allocate<Tr2InteriorBackgroundCubemapBatch>();

		// Allocate a per-object data for the background cubemap
		Tr2LitPerObjectData* perObjectData =
			batches->Allocate<Tr2LitPerObjectData>();

		// If the batch & per-object data allocations succeeded
		if( batch && perObjectData )
		{
			// Pixel shader per-object buffer
			Tr2InteriorPerObjectPSData perObjectPSBuffer;
			memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );
			// Set the mirror-to-world matrix
			perObjectPSBuffer.mirrorToWorldMatrix = m_mirrorToWorldMatrix;

			// Copy buffer into the per-object data
			perObjectData->CopyToPSFloatBuffer( perObjectPSBuffer );

			batch->SetShaderMaterial( m_backgroundEffect );
			batch->SetPerObjectData( perObjectData );
			batch->SetRenderingMode( Tr2EffectStateManager::RM_ANY );
			batches->SetUserData(
				ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_BEGIN )
				);

			batches->Commit( batch );
		}
	}
}

void Tr2InteriorScene::RenderGeometry( ITr2ShaderMaterial* overrideEffect, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// From now on its managed!
	renderContext.m_esm.BeginManagedRendering();

	// Render primary batches - includes opaques, decals, transparents & special batches
	{
		D3DPERF_EVENT( L"Primary render batches" );
		m_activePrimaryRenderBatches->Finalize();
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
		renderContext.m_esm.RenderBatchesWithOverride(
			m_activePrimaryRenderBatches,
			m_visualizerOverride ? m_visualizerOverride : overrideEffect,
			m_visualizerOverrideApplyPS ? Tr2EffectStateManager::OM_APPLY_PS : Tr2EffectStateManager::OM_DO_NOTHING );
	}

	// Collect statistics
	m_unsortedRenderBatchCount = (unsigned int)m_primaryRenderBatches->GetBatchCount();

	// Restore original viewport and per-frame data
	{
		D3DPERF_EVENT( L"Cleanup State" );

		unsigned int width, height;
		Tr2Renderer::GetBackBufferDimensions( width, height );

		renderContext.SetScissorRect( 0, 0, width, height );
		renderContext.SetRenderState( RS_SCISSORTESTENABLE, FALSE );
		renderContext.m_esm.SetInvertedCullMode( false );
		renderContext.SetRenderState( RS_CLIPPLANEENABLE, 0x0 );
		renderContext.SetRenderState( RS_STENCILENABLE, FALSE );
	}

	// Note: certain post-scene-render events (such as post-render callbacks) expect
	// to be in Alpha state.  This isn't guaranteed now, since Alpha state only gets set
	// during normal scene rendering if alpha batches are actually rendered.
	// This forces Alpha state, so that subsequent draw calls are in the correct
	// state environment.
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
	renderContext.m_esm.EndManagedRendering();
}

void Tr2InteriorScene::ClearVisibilityResults( void )
{
	if( m_visibilityResults )
	{
		m_visibilityResults->Clear();
	}
}

// -------------------------------------------------------------
// Description:
//   Helper function to render parts of a given box faces that
//   are inside another box.
// Arguments:
//   minBounds1 - Min bounds of box which faces we need to render
//   maxBounds1 - Max bounds of box which faces we need to render
//   minBounds2 - Min bounds of box to test intersection with
//   minBounds3 - Max bounds of box to test intersection with
//   transform - Transform matrix from 2nd box coordinate system
//               to 1st box CS
// -------------------------------------------------------------
static void RenderBoxInsideBox( const Vector3& minBounds1, const Vector3& maxBounds1,
								const Vector3& minBounds2, const Vector3& maxBounds2, const Matrix &transform )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	static const unsigned int color = 0x44ff3333;

	const Vector3 sides[6][4] = {
		{
			Vector3( minBounds1.x, minBounds1.y, minBounds1.z ),
			Vector3( minBounds1.x, maxBounds1.y, minBounds1.z ),
			Vector3( minBounds1.x, maxBounds1.y, maxBounds1.z ),
			Vector3( minBounds1.x, minBounds1.y, maxBounds1.z ),
		},
		{
			Vector3( maxBounds1.x, minBounds1.y, minBounds1.z ),
			Vector3( maxBounds1.x, minBounds1.y, maxBounds1.z ),
			Vector3( maxBounds1.x, maxBounds1.y, maxBounds1.z ),
			Vector3( maxBounds1.x, maxBounds1.y, minBounds1.z ),
		},
		{
			Vector3( minBounds1.x, minBounds1.y, minBounds1.z ),
			Vector3( maxBounds1.x, minBounds1.y, minBounds1.z ),
			Vector3( maxBounds1.x, minBounds1.y, maxBounds1.z ),
			Vector3( minBounds1.x, minBounds1.y, maxBounds1.z ),
		},
		{
			Vector3( minBounds1.x, maxBounds1.y, minBounds1.z ),
			Vector3( minBounds1.x, maxBounds1.y, maxBounds1.z ),
			Vector3( maxBounds1.x, maxBounds1.y, maxBounds1.z ),
			Vector3( maxBounds1.x, maxBounds1.y, minBounds1.z ),
		},
		{
			Vector3( minBounds1.x, minBounds1.y, minBounds1.z ),
			Vector3( maxBounds1.x, minBounds1.y, minBounds1.z ),
			Vector3( maxBounds1.x, maxBounds1.y, minBounds1.z ),
			Vector3( minBounds1.x, maxBounds1.y, minBounds1.z ),
		},
		{
			Vector3( minBounds1.x, minBounds1.y, maxBounds1.z ),
			Vector3( minBounds1.x, maxBounds1.y, maxBounds1.z ),
			Vector3( maxBounds1.x, maxBounds1.y, maxBounds1.z ),
			Vector3( maxBounds1.x, minBounds1.y, maxBounds1.z ),
		},
	};

	D3DXPLANE planes[6] = {
		D3DXPLANE( -1, 0, 0, minBounds2.x ),
		D3DXPLANE( 1, 0, 0, -maxBounds2.x ),
		D3DXPLANE( 0, -1, 0, minBounds2.y ),
		D3DXPLANE( 0, 1, 0, -maxBounds2.y ),
		D3DXPLANE( 0, 0, -1, minBounds2.z ),
		D3DXPLANE( 0, 0, 1, -maxBounds2.z ),
	};

	Matrix planeTransform;
	D3DXMatrixInverse( &planeTransform, NULL, &transform );
	D3DXMatrixTranspose( &planeTransform, &planeTransform );
	D3DXPlaneTransformArray( planes, sizeof( D3DXPLANE ), planes, sizeof( D3DXPLANE ), &planeTransform, 6 );

	for( int side = 0; side < 6; ++side )
	{
  		TriDebugResourceHelper::VertexPosColor polygon1[12];
		TriDebugResourceHelper::VertexPosColor polygon2[12];
		TriDebugResourceHelper::VertexPosColor *inPolygon = polygon1;
		TriDebugResourceHelper::VertexPosColor *outPolygon = polygon2;
		for( int i = 0; i < 4; ++i )
		{
			inPolygon[i].m_pos = sides[side][i];
			inPolygon[i].m_color = color;
		}
		int inCount = 4;
		int outCount = 0;

		for( int plane = 0; plane < 6; ++plane )
		{
			for( int edge = 0; edge < inCount; ++edge )
			{
				const Vector3& vertex1 = inPolygon[edge].m_pos;
				const Vector3& vertex2 = inPolygon[( edge + 1 ) % inCount].m_pos;
				float v0 = D3DXPlaneDotCoord( &planes[plane], &vertex1 );
				float v1 = D3DXPlaneDotCoord( &planes[plane], &vertex2 );
				if( v0 <= 0 )
				{
					outPolygon[outCount++] = inPolygon[edge];
					CCP_ASSERT( outCount < 12 );
				}
				if( v0 * v1 < 0 )
				{
					Vector3 result;
					D3DXPlaneIntersectLine( &result, &planes[plane], &vertex1, &vertex2 );
					outPolygon[outCount].m_pos = result;
					outPolygon[outCount++].m_color = inPolygon[edge].m_color;
					CCP_ASSERT( outCount < 12 );
				}
			}
			std::swap( inPolygon, outPolygon );
			std::swap( inCount, outCount );
			outCount = 0;
		}

		if( inCount > 2 )
		{
			struct EffectCallback: public IRenderCallback
			{
				TriDebugResourceHelper::VertexPosColor *polygon;
				unsigned int count;

				void SubmitGeometry( Tr2RenderContext& renderContext )
				{
					uint32_t stride = sizeof( TriDebugResourceHelper::VertexPosColor );

					renderContext.SetTopology( TOP_TRIANGLE_FAN );	//note doesn't exist anymore in DX11... only for debug rendering so can live with it for now.
					renderContext.DrawPrimitiveUP( count - 2, polygon, stride );
				}
			} callback;

			callback.polygon = inPolygon;
			callback.count = unsigned int( inCount );

			g_debugResourceHelper.GetEffect()->Render( &callback, renderContext );
		}
	}
}

// -------------------------------------------------------------
// Description:
//   Helper function to render volume of intersection of two
//   boxes. Used for debug rendering of cell intersection.
// Arguments:
//   minBounds1 - Min bounds of box which faces we need to render
//   maxBounds1 - Max bounds of box which faces we need to render
//   transform1 - Transform matrix from 1st box CS to world CS
//   minBounds2 - Min bounds of box to test intersection with
//   minBounds3 - Max bounds of box to test intersection with
//   transform2 - Transform matrix from 2nd box CS to world CS
// -------------------------------------------------------------
static void RenderBoxIntersection( const Vector3& minBounds1, const Vector3& maxBounds1, const Matrix &transform1,
								   const Vector3& minBounds2, const Vector3& maxBounds2, const Matrix &transform2 )
{
	Matrix toBox1, toBox2;
	D3DXMatrixInverse( &toBox1, NULL, &transform1 );
	toBox1 = transform2 * toBox1;
	D3DXMatrixInverse( &toBox2, NULL, &transform2 );
	toBox2 = transform1 * toBox2;

	USE_MAIN_THREAD_RENDER_CONTEXT();

	renderContext.m_esm.ApplyVertexDeclaration( g_debugResourceHelper.GetVertexPosColorDecl() );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );

	Tr2Renderer::SetWorldTransform( transform1 );
	RenderBoxInsideBox( minBounds1, maxBounds1, minBounds2, maxBounds2, toBox1 );

	Tr2Renderer::SetWorldTransform( transform2 );
	RenderBoxInsideBox( minBounds2, maxBounds2, minBounds1, maxBounds1, toBox2 );
}

void Tr2InteriorScene::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	// lines
	if( m_renderDebugInfo && m_debugLines )
	{
		// cells
		for( PTr2InteriorCellVector::const_iterator it = m_cells.begin(); it != m_cells.end(); ++it )
		{
			( *it )->RenderDebugInfo( m_debugLines );
		}

		// Portals
		for( PTr2InteriorPhysicalPortalVector::const_iterator it = m_portals.begin(); it != m_portals.end(); ++it )
		{
			( *it )->RenderDebugInfo( m_debugLines );
		}

		for( PITr2InteriorDynamicVector::const_iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
		{
			if( const Tr2InteriorPlaceable* placable = dynamic_cast<const Tr2InteriorPlaceable*>( *it ) )
			{
				placable->RenderDebugInfo( m_debugLines );
			}
			else if( const Tr2InteriorParticleObject* particle = dynamic_cast<const Tr2InteriorParticleObject*>( *it ) )
			{
				particle->RenderDebugInfo( m_debugLines );
			}
		}

		m_debugLines->Render( renderContext );
		m_debugLines->Clear();

		// Render cell intersections
		for( PTr2InteriorCellVector::const_iterator it = m_cells.begin(); it != m_cells.end(); ++it )
		{
			Vector3 minBounds1, maxBounds1;
			( *it )->GetBoundingBox( minBounds1, maxBounds1 );
			for( PTr2InteriorCellVector::const_iterator jt = it + 1; jt != m_cells.end(); ++jt )
			{
				Vector3 minBounds2, maxBounds2;
				( *jt )->GetBoundingBox( minBounds2, maxBounds2 );

				RenderBoxIntersection( minBounds1, maxBounds1, ( *it )->GetWorldTransform(),
									   minBounds2, maxBounds2, ( *jt )->GetWorldTransform() );
			}
		}
	}

	// Light volumes are rendered regardless of renderDebugInfo flag
	for( auto it = m_lights.cbegin(); it != m_lights.cend(); ++it )
	{
		( *it )->RenderDebugInfo( renderContext );
	}

	if( m_apexScene )
	{
		m_apexScene->RenderDebugInfo( renderContext );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Re-renders shadows on different GPUs on SLI architecture.
// Return Value:
//   Number of shadow maps re-rendered.
// --------------------------------------------------------------------------------------
unsigned Tr2InteriorScene::ReRenderShadowMaps( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( "ReRenderShadowMaps" );

	if( m_numSLIGroups <= 1 )
	{
		return 0;
	}

	for( std::map<ShadowReRenderInfo, unsigned>::iterator it = m_shadowReRenderInfo.begin(); it != m_shadowReRenderInfo.end(); )
	{
		if( !it->first.lightSource || !it->first.lightSource->GetShadowAtlasTexture( it->first.shadowMapIndex ) )
		{
			std::map<ShadowReRenderInfo, unsigned>::iterator erase = it;
			++it;
			m_shadowReRenderInfo.erase( erase );
		}
		else
		{
			++it;
		}
	}

	if( m_shadowReRenderInfo.empty() )
	{
		return 0;
	}

	Tr2PerFrameVSData shadowPerFrameVS;
	Tr2PerFrameShadowPSData shadowPerFramePS;
	memset( &shadowPerFrameVS, 0, sizeof( shadowPerFrameVS ) );
	memset( &shadowPerFramePS, 0, sizeof( shadowPerFramePS ) );

	Tr2Renderer::PushViewport();
	ON_BLOCK_EXIT( &Tr2Renderer::PopViewport );

	Tr2PushPopDS pushPopDS( m_shadowMapDepthBuffer, renderContext );
	Tr2PushPopRT pushPopRT( renderContext );

	Tr2Renderer::PushProjection();
	ON_BLOCK_EXIT( &Tr2Renderer::PopProjection );

	Tr2Renderer::PushViewTransform();
	ON_BLOCK_EXIT( &Tr2Renderer::PopViewTransform );

	m_activePrimaryRenderBatches  = m_shadowBatches;
	m_activeTransparentBatchStore = NULL;
	m_gatherShadowCasterBatches = true;
	m_visibilityQueryType = SHADOW_QUERY;

	unsigned shadowsUpdated = 0;

	for( std::map<ShadowReRenderInfo, unsigned>::iterator it = m_shadowReRenderInfo.begin(); it != m_shadowReRenderInfo.end(); )
	{
		CCP_STATS_ZONE( "ShadowMapUpdate" );
		Tr2AtlasTexture* texture = it->first.lightSource->GetShadowAtlasTexture( it->first.shadowMapIndex );

		if( !texture->GetRenderTarget() )
		{
			continue;
		}

		Tr2Renderer::SetRenderTarget( 0, m_shadowMapTemporaryRTs[0], renderContext );

		CTriViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = texture->GetWidth();
		viewport.height = texture->GetHeight();
		Tr2Renderer::SetViewport( viewport );

		it->first.lightSource->BeginShadowUpdate( it->first.shadowMapIndex, &shadowPerFrameVS, &shadowPerFramePS, renderContext );

		// Prepare for Umbra visibility query
		m_cameraToWorldMatrix = it->first.lightSource->GetViewMatrix( it->first.shadowMapIndex );

		bool oldDisplayDynamics = m_displayDynamics;
		m_displayDynamics = it->first.lightSource->GetShadowCasterTypes() & ITr2InteriorLight::ST_DYNAMICS_ONLY;
		m_displayStatics = it->first.lightSource->GetShadowCasterTypes() & ITr2InteriorLight::ST_STATICS_ONLY;

		m_shadowCamera.SetScreenSize( texture->GetWidth(), texture->GetHeight() );
		m_shadowCamera.SetViewParameters( it->first.lightSource->GetInvViewMatrix( it->first.shadowMapIndex ),
			it->first.lightSource->GetProjectionMatrix( it->first.shadowMapIndex ) );
		m_visibilityQueryType = SHADOW_QUERY;
		m_lightGeneratingShadows = it->first.lightSource;
		ResolveVisibility( m_shadowCamera );

		m_displayDynamics = oldDisplayDynamics;
		m_displayStatics = true;

		// Finalize batches
		m_shadowBatches->Finalize();

		// Set new perframe data
		{
			D3DPERF_EVENT( L"Set per-frame shader constants" );
			FillAndSetConstants( m_perFrameShadowVSBuffer, shadowPerFrameVS, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
			FillAndSetConstants( m_perFrameShadowPSBuffer, shadowPerFramePS, PIXEL_SHADER , Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
		}

		// Render opaques and decals
		{
			D3DPERF_EVENT( L"Render opaques" );
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
			renderContext.m_esm.RenderBatches( m_shadowBatches );
		}

		it->first.lightSource->EndShadowUpdate( it->first.shadowMapIndex );

		m_shadowBatches->Clear();

		// Blur the shadow
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

		CTriViewport viewport0;
		viewport0.x = texture->GetX();
		viewport0.y = texture->GetY();
		viewport0.width = texture->GetWidth();
		viewport0.height = texture->GetHeight();

		// horizontal filtering
		Vector2 tl( 0.0f, 0.0f );
		Vector2 br = Vector2( float( texture->GetWidth() ), float( texture->GetHeight() ) ) / INTERIOR_SHADOW_MAP_MAX_RESOLUTION;
		float offset = 0.5f / INTERIOR_SHADOW_MAP_MAX_RESOLUTION;

		m_shadowFilterAreaVar = Vector4( tl.x + offset, tl.y + offset, br.x - offset, br.y - offset );
		m_shadowFilterVar = Vector4( 1.f / float( INTERIOR_SHADOW_MAP_MAX_RESOLUTION ), 0.f, 0.f, 0.f );
		Tr2Renderer::SetRenderTarget( 0, m_shadowMapTemporaryRTs[1], renderContext );
		Tr2Renderer::SetViewport( viewport );
		Tr2Renderer::DrawTexture( m_shadowFilter, m_shadowMapTemporaryRTs[0].GetTexture(), tl, br );

		// vertical filtering
		m_shadowFilterAreaVar = Vector4( tl.x + offset, tl.y + offset, br.x - offset, br.y - offset );
		m_shadowFilterVar = Vector4( 0.f, 1.f / float( INTERIOR_SHADOW_MAP_MAX_RESOLUTION ), 0.f, 0.f );
		Tr2Renderer::SetRenderTarget( 0, *texture->GetRenderTarget(), renderContext );
		Tr2Renderer::PushDepthStencilBuffer( nullDS, renderContext );
		Tr2Renderer::SetViewport( viewport0 );
		Tr2Renderer::DrawTexture( m_shadowFilter, m_shadowMapTemporaryRTs[1].GetTexture(), tl, br );
		Tr2Renderer::PopDepthStencilBuffer( renderContext );

		m_shadowFilterAreaVar = Vector4( 0.0f, 0.0f, 1.f, 1.f );

		if( ++it->second >= m_numSLIGroups )
		{
			std::map<ShadowReRenderInfo, unsigned>::iterator erase = it;
			++it;
			m_shadowReRenderInfo.erase( erase );
		}
		else
		{
			++it;
		}

		shadowsUpdated++;
		if( shadowsUpdated >= m_shadowsUpdatesPerFrame )
		{
			break;
		}
	}

	m_lightGeneratingShadows = NULL;
	m_visibilityQueryType = PRIMARY_QUERY;
	m_gatherShadowCasterBatches = false;

	return shadowsUpdated;
}

void Tr2InteriorScene::RenderShadowMaps( Tr2RenderContext& renderContext )
{
	D3DPERF_EVENT( L"Tr2InteriorScene::RenderShadowMaps" );
	CCP_STATS_ZONE( "RenderShadowMaps" );

	renderContext.m_esm.BeginManagedRendering();
	ON_BLOCK_EXIT( [&]{ renderContext.m_esm.EndManagedRendering(); } );

	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	if( !allocator )
	{
		return;
	}

	if( !m_shadowMapTemporaryRTs[0].IsValid() || !m_shadowMapTemporaryRTs[1].IsValid() )
	{
		return;
	}

	if( m_shadowsUpdatesPerFrame > 0 && Tr2Renderer::IsResourceCreationAllowed() )
	{
		unsigned updateCount = ReRenderShadowMaps( renderContext );
		if( updateCount >= m_shadowsUpdatesPerFrame )
		{
			return;
		}

		Vector3 focalPosition = m_useShadowFocalPosition ? m_shadowFocalPosition : Tr2Renderer::GetViewPosition();
		// put all lightsources in vector with sorting values
		std::vector<ITr2InteriorLight::LightSourceItem> sortedLightSources;
		std::vector<ITr2InteriorLight::LightSourceItem> sortedLightLODSources;
		for( PITr2InteriorLightVector::const_iterator it = m_lights.begin(); it != m_lights.end(); ++it )
		{
			if( m_enableLightCulling && ( m_visibleLights.find( *it ) == m_visibleLights.end() ) )
			{
				continue;
			}

			( *it )->CacheShadowMapResolution();
			for( unsigned i = 0; i < ( *it )->GetRequiredShadowMapCount(); ++i )
			{
				unsigned width, height;
				( *it )->GetRequiredShadowMapResolution( i, !m_useShadowLOD, width, height );
				if( width == 0 || height == 0 )
				{
					( *it )->SetShadowAtlasTexture( i, NULL );
					( *it )->MarkShadowDirty( i, false );
					( *it )->SetFramesSinceShadowUpdate( i, 0 );
				}
				else if( ( *it )->IsShadowDirty( i ) )
				{
					unsigned frames = ( *it )->GetFramesSinceShadowUpdate( i );
					( *it )->SetFramesSinceShadowUpdate( i, frames + 1 );
					ITr2InteriorLight::LightSourceItem item;
					item.lightSource = ( *it );
					item.importance = ( *it )->GetCurrentShadowImportance( i, focalPosition ) * frames;
					item.shadowMapIndex = i;
					sortedLightSources.push_back( item );
				}
				else if( !( *it )->IsShadowEmpty( i ) )
				{
					Tr2AtlasTexture* texture = ( *it )->GetShadowAtlasTexture( i );
					unsigned width, height;
					( *it )->GetRequiredShadowMapResolution( i, !m_useShadowLOD, width, height );
					if( width > 0 && height > 0 )
					{
						if( texture == NULL || width != texture->GetWidth() || height != texture->GetHeight() )
						{
							// Give a chance for shadow update to shadow maps that require shadow resolution
							// change (although with lower priority than dirty shadows).
							static const float CHANGE_RESOLUTION_IMPORTANCE_FACTOR = 0.1f;

							unsigned frames = ( *it )->GetFramesSinceShadowUpdate( i );
							( *it )->SetFramesSinceShadowUpdate( i, frames + 1 );
							ITr2InteriorLight::LightSourceItem item;
							item.lightSource = ( *it );
							item.importance = ( *it )->GetCurrentShadowImportance( i, focalPosition ) *
								frames * CHANGE_RESOLUTION_IMPORTANCE_FACTOR;
							item.shadowMapIndex = i;
							sortedLightLODSources.push_back( item );
						}
					}
					else
					{
						( *it )->SetFramesSinceShadowUpdate( i, 0 );
					}
				}
				else
				{
					( *it )->SetFramesSinceShadowUpdate( i, 0 );
				}
			}
		}

		if( !sortedLightSources.empty() || !sortedLightLODSources.empty() )
		{
			CCP_STATS_ADD( wodInteriorSceneShadowsNeedUpdating, sortedLightSources.size() + sortedLightLODSources.size() );

			// sort!
			std::sort( sortedLightSources.begin(), sortedLightSources.end() );
			std::sort( sortedLightLODSources.begin(), sortedLightLODSources.end() );

			Tr2PerFrameVSData shadowPerFrameVS;
			Tr2PerFrameShadowPSData shadowPerFramePS;
			memset( &shadowPerFrameVS, 0, sizeof( shadowPerFrameVS ) );
			memset( &shadowPerFramePS, 0, sizeof( shadowPerFramePS ) );

			Tr2Renderer::PushViewport();
			ON_BLOCK_EXIT( &Tr2Renderer::PopViewport );

			Tr2PushPopDS pushPopDS( renderContext );
			Tr2PushPopRT pushPopRT( renderContext );

			Tr2Renderer::PushProjection();
			ON_BLOCK_EXIT( &Tr2Renderer::PopProjection );

			Tr2Renderer::PushViewTransform();
			ON_BLOCK_EXIT( &Tr2Renderer::PopViewTransform );

			m_activePrimaryRenderBatches  = m_shadowBatches;
			m_activeTransparentBatchStore = NULL;
			m_gatherShadowCasterBatches = true;
			m_visibilityQueryType = SHADOW_QUERY;
			unsigned lodSwitches = std::min( m_shadowsLODSwitchesPerFrame, (unsigned int)sortedLightLODSources.size() );

			for( std::vector<ITr2InteriorLight::LightSourceItem>::iterator it = sortedLightSources.begin(); it != sortedLightSources.end(); ++it )
			{
				// Give a chance for LOD switches (no more than m_shadowsLODSwitchesPerFrame at a time)
				if( updateCount + lodSwitches >= m_shadowsUpdatesPerFrame && !sortedLightLODSources.empty() )
				{
					if( it->importance < sortedLightLODSources.front().importance )
					{
						break;
					}
				}

				if( UpdateShadowMap( *it, allocator, shadowPerFrameVS, shadowPerFramePS, renderContext ) )
				{
					updateCount++;
					if( updateCount >= m_shadowsUpdatesPerFrame )
					{
						break;
					}
				}
			}

			for( std::vector<ITr2InteriorLight::LightSourceItem>::iterator it = sortedLightLODSources.begin(); it != sortedLightLODSources.end(); ++it )
			{
				if( UpdateShadowMap( *it, allocator, shadowPerFrameVS, shadowPerFramePS, renderContext ) )
				{
					updateCount++;
					if( updateCount >= m_shadowsUpdatesPerFrame )
					{
						break;
					}
				}
			}

			m_lightGeneratingShadows = NULL;
			m_visibilityQueryType = PRIMARY_QUERY;
			m_gatherShadowCasterBatches = false;
		}
	}
	else
	{
		for( PITr2InteriorLightVector::const_iterator it = m_lights.begin(); it != m_lights.end(); ++it )
		{
			for( unsigned int i = 0; i < ( *it )->GetRequiredShadowMapCount(); ++i )
			{
				( *it )->SetShadowAtlasTexture( i, NULL );
				( *it )->MarkShadowDirty( i, true );
			}
		}
	}
	for( size_t i = 0; i < m_shadowAtlases.size(); ++i )
	{
		if( m_shadowAtlases[i]->GetTexturesInAtlasCount() == 0 )
		{
			m_shadowAtlases.Remove( i-- );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates a single shadow from a light source.
// Arguments:
//   item - Light source shadow information
//   allocator - Render batch allocator
//   shadowMapTemporarySurface - Temporary DirectX surfaces used for rendering a shadow
//   shadowPerFrameVS - Per-frame VS data
//   shadowPerFramePS - Per-frame VS data
// Return Value:
//   true If shadow was successfully rendered
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2InteriorScene::UpdateShadowMap(
									   ITr2InteriorLight::LightSourceItem& item,
									   TriPoolAllocator* allocator,
									   Tr2PerFrameVSData shadowPerFrameVS,
									   Tr2PerFrameShadowPSData shadowPerFramePS,
									   Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( "PerLightSource" );

	Tr2AtlasTexture* texture = item.lightSource->GetShadowAtlasTexture( item.shadowMapIndex );
	if( texture != NULL )
	{
		unsigned width, height;
		item.lightSource->GetRequiredShadowMapResolution( item.shadowMapIndex, !m_useShadowLOD, width, height );
		if( width != texture->GetWidth() || height != texture->GetHeight() )
		{
			item.lightSource->SetShadowAtlasTexture( item.shadowMapIndex, NULL );
			texture = NULL;
		}
	}
	if( texture == NULL )
	{
		unsigned width, height;
		item.lightSource->GetRequiredShadowMapResolution( item.shadowMapIndex, !m_useShadowLOD, width, height );
		CCP_ASSERT( width < INTERIOR_SHADOW_ATLAS_RESOLUTION && height < INTERIOR_SHADOW_ATLAS_RESOLUTION );

		// First try to insert a new texture into the same atlas as any other texture of the same light
		unsigned count = item.lightSource->GetRequiredShadowMapCount();
		bool firstTexture = true;
		for( unsigned int i = 0; i < count; ++i )
		{
			if( item.lightSource->GetShadowAtlasTexture( i ) != NULL )
			{
				firstTexture = false;
				for( PTr2TextureAtlasVector::iterator atlas = m_shadowAtlases.begin(); atlas != m_shadowAtlases.end(); ++atlas )
				{
					if( ( *atlas )->GetRenderTarget() == item.lightSource->GetShadowAtlasTexture( i )->GetRenderTarget() )
					{
						( *atlas )->CreateTexture( width, height, Tr2TextureAtlas::ATT_DEFAULT, &texture );
						if( texture )
						{
							item.lightSource->SetShadowAtlasTexture( item.shadowMapIndex, texture );
							texture->Unlock();
						}
						break;
					}
				}
				break;
			}
		}

		if( texture == NULL )
		{
			if( firstTexture )
			{
				// This is the first texture for this light - insert it into any atlas
				for( PTr2TextureAtlasVector::iterator atlas = m_shadowAtlases.begin();
					atlas != m_shadowAtlases.end(); ++atlas )
				{
					( *atlas )->CreateTexture( width, height, Tr2TextureAtlas::ATT_DEFAULT, &texture );
					if( texture )
					{
						break;
					}
				}
				if( texture == NULL )
				{
					Tr2TextureAtlasPtr ta;
					ta.CreateInstance();
					ta->InitializeRenderTarget(
						PIXEL_FORMAT_R16G16_UNORM,
						INTERIOR_SHADOW_ATLAS_RESOLUTION,
						INTERIOR_SHADOW_ATLAS_RESOLUTION );
					ta->SetPaintEmptyAreas( false );
					ta->SetMargin( 0 );
					ta->SetCreateOutsiders( false );
					ta->SetMaxTextureArea(
						INTERIOR_SHADOW_ATLAS_RESOLUTION * INTERIOR_SHADOW_ATLAS_RESOLUTION );
					m_shadowAtlases.Append( ta );
					ta->CreateTexture( width, height, Tr2TextureAtlas::ATT_DEFAULT, &texture );
				}
				if( texture == NULL )
				{
					CCP_LOGERR( "Could not fit shadow map into any atlas texture" );
					return false;
				}
				else
				{
					item.lightSource->SetShadowAtlasTexture( item.shadowMapIndex, texture );
					texture->Unlock();
				}
			}
			else
			{
				// We already have textures for this light, so we need to allocate all of
				// them in some other atlas (that can fit them + new texture) and copy
				// texture contents into a new place
				Tr2AtlasTexturePtr *textures = new Tr2AtlasTexturePtr[count];
				for( unsigned int i = 0; i < count; ++i )
				{
					textures[i] = 0;
				}
				bool failed = true;
				for( PTr2TextureAtlasVector::iterator atlas = m_shadowAtlases.begin();
					failed && atlas != m_shadowAtlases.end(); ++atlas )
				{
					failed = false;
					for( unsigned int i = 0; i < count; ++i )
					{
						if( item.lightSource->GetShadowAtlasTexture( i ) || i == item.shadowMapIndex )
						{
							unsigned width, height;
							if( i == item.shadowMapIndex )
							{
								item.lightSource->GetRequiredShadowMapResolution(
									i,
									!m_useShadowLOD,
									width,
									height );
							}
							else
							{
								width = item.lightSource->GetShadowAtlasTexture( i )->GetWidth();
								height = item.lightSource->GetShadowAtlasTexture( i )->GetHeight();
							}
							CCP_ASSERT( width < INTERIOR_SHADOW_ATLAS_RESOLUTION && height < INTERIOR_SHADOW_ATLAS_RESOLUTION );
							textures[i] = nullptr;
							( *atlas )->CreateTexture( width, height, Tr2TextureAtlas::ATT_DEFAULT, &textures[i] );
							if( textures[i] == NULL )
							{
								failed = true;
								break;
							}
						}
					}
				}
				if( failed )
				{
					failed = false;
					Tr2TextureAtlasPtr ta;
					ta.CreateInstance();
					ta->InitializeRenderTarget(
						PIXEL_FORMAT_R16G16_UNORM,
						INTERIOR_SHADOW_ATLAS_RESOLUTION,
						INTERIOR_SHADOW_ATLAS_RESOLUTION );
					ta->SetPaintEmptyAreas( false );
					ta->SetMargin( 0 );
					ta->SetCreateOutsiders( false );
					ta->SetMaxTextureArea(
						INTERIOR_SHADOW_ATLAS_RESOLUTION * INTERIOR_SHADOW_ATLAS_RESOLUTION );
					m_shadowAtlases.Append( ta );
					for( unsigned int i = 0; i < count; ++i )
					{
						if( item.lightSource->GetShadowAtlasTexture( i ) || i == item.shadowMapIndex )
						{
							unsigned width, height;
							if( i == item.shadowMapIndex )
							{
								item.lightSource->GetRequiredShadowMapResolution(
									i,
									!m_useShadowLOD,
									width,
									height );
							}
							else
							{
								width = item.lightSource->GetShadowAtlasTexture( i )->GetWidth();
								height = item.lightSource->GetShadowAtlasTexture( i )->GetHeight();
							}
							CCP_ASSERT( width < INTERIOR_SHADOW_ATLAS_RESOLUTION &&
								height < INTERIOR_SHADOW_ATLAS_RESOLUTION );
							textures[i] = nullptr;
							m_shadowAtlases.back()->CreateTexture( width, height, Tr2TextureAtlas::ATT_DEFAULT, &textures[i] );
							CCP_ASSERT( textures[i] );
							if( textures[i] == NULL )
							{
								failed = true;
								break;
							}
						}
					}
				}
				if( failed )
				{
					CCP_LOGERR( "Could not fit all shadow maps of a light into any atlas texture" );
					delete []textures;
					return false;
				}
				else
				{
					// Copy textures into new locations
					bool copySuccess = true;
					bool setRTs = false;
					for( unsigned int i = 0; i < count; ++i )
					{
						if( textures[i] )
						{
							Tr2AtlasTexture *srcTexture = item.lightSource->GetShadowAtlasTexture( i );
							if( srcTexture )
							{
								if( !setRTs )
								{
									setRTs = true;
									Tr2Renderer::SetRenderTarget( 0, *textures[i]->GetRenderTarget(), renderContext );
									Tr2Renderer::SetDepthStencilBuffer( nullDS, renderContext );
								}
								CTriViewport viewport;
								viewport.x = textures[i]->GetX();
								viewport.y = textures[i]->GetY();
								viewport.width = textures[i]->GetWidth();
								viewport.height = textures[i]->GetHeight();
								Tr2Renderer::SetViewport( viewport );
								Vector4 window;
								srcTexture->GetTextureWindow( window );
								Tr2Renderer::DrawTexture(
									*srcTexture->GetTexture(),
									Vector2( window.x, window.y ),
									Vector2( window.x + window.z, window.y + window.w ) );
							}
						}
					}
					if( copySuccess )
					{
						for( unsigned int i = 0; i < count; ++i )
						{
							if( textures[i] )
							{
								Tr2AtlasTexture *srcTexture = item.lightSource->GetShadowAtlasTexture( i );
								item.lightSource->SetShadowAtlasTexture( i, textures[i] );
							}
						}
						texture = textures[item.shadowMapIndex];
						delete []textures;
					}
					else
					{
						delete []textures;
						return false;
					}
				}
			}
		}
	}

	CTriViewport viewport;

	{
		CCP_STATS_ZONE( "Setup" );

		if( !texture->GetRenderTarget() )
		{
			return false;
		}

		Tr2Renderer::SetRenderTarget( 0, m_shadowMapTemporaryRTs[0], renderContext );
		Tr2Renderer::SetDepthStencilBuffer( m_shadowMapDepthBuffer, renderContext );

		viewport.x = 0;
		viewport.y = 0;
		viewport.width = texture->GetWidth();
		viewport.height = texture->GetHeight();
		Tr2Renderer::SetViewport( viewport );

		item.lightSource->SetFramesSinceShadowUpdate( item.shadowMapIndex, 0 );
		item.lightSource->MarkShadowDirty( item.shadowMapIndex, false );

		item.lightSource->BeginShadowUpdate( item.shadowMapIndex, &shadowPerFrameVS, &shadowPerFramePS, renderContext );

		// Prepare for Umbra visibility query
		m_cameraToWorldMatrix = item.lightSource->GetViewMatrix( item.shadowMapIndex );

		bool oldDisplayDynamics = m_displayDynamics;
		m_displayDynamics = item.lightSource->GetShadowCasterTypes() & ITr2InteriorLight::ST_DYNAMICS_ONLY;
		m_displayStatics = item.lightSource->GetShadowCasterTypes() & ITr2InteriorLight::ST_STATICS_ONLY;

		m_shadowCamera.SetScreenSize( texture->GetWidth(), texture->GetHeight() );
		m_shadowCamera.SetViewParameters( item.lightSource->GetInvViewMatrix( item.shadowMapIndex ),
			item.lightSource->GetProjectionMatrix( item.shadowMapIndex ) );
		m_visibilityQueryType = SHADOW_QUERY;
		m_lightGeneratingShadows = item.lightSource;
		ResolveVisibility( m_shadowCamera );

		m_displayDynamics = oldDisplayDynamics;
		m_displayStatics = true;

		// Finalize batches
		m_shadowBatches->Finalize();
	}

	{
		CCP_STATS_ZONE( "Render" );

		bool hasRenderBatches = false;
		for( TriRenderBatch* batch = m_shadowBatches->GetFirstBatch(); batch != NULL; batch = batch->GetNext() )
		{
			ITr2ShaderState* shaderForThisBatch = batch->GetShaderStateInterface();

			if( shaderForThisBatch )
			{
				hasRenderBatches = true;
				break;
			}
		}
		item.lightSource->SetEmptyShadow( item.shadowMapIndex, !hasRenderBatches );
		if( !hasRenderBatches )
		{
			item.lightSource->SetShadowAtlasTexture( item.shadowMapIndex, NULL );
			item.lightSource->MarkShadowDirty( item.shadowMapIndex, false );
			item.lightSource->EndShadowUpdate( item.shadowMapIndex );
			m_shadowBatches->Clear();
			return false;
		}

		// Set new perframe data
		{
			D3DPERF_EVENT( L"Set per-frame shader constants" );
			FillAndSetConstants( m_perFrameShadowVSBuffer, shadowPerFrameVS, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
			FillAndSetConstants( m_perFrameShadowPSBuffer, shadowPerFramePS, PIXEL_SHADER , Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
		}

		// Render opaques and decals
		{
			D3DPERF_EVENT( L"Render opaques" );
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
			renderContext.m_esm.RenderBatches( m_shadowBatches );
		}

		item.lightSource->EndShadowUpdate( item.shadowMapIndex );

		m_shadowBatches->Clear();
	}

	// Blur the shadow
	{
		CCP_STATS_ZONE( "Blur" );

		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
		Tr2Renderer::SetDepthStencilBuffer( nullDS, renderContext );

		CTriViewport viewport0;
		viewport0.x = texture->GetX();
		viewport0.y = texture->GetY();
		viewport0.width = texture->GetWidth();
		viewport0.height = texture->GetHeight();

		// horizontal filtering
		Vector2 tl( 0.0f, 0.0f );
		Vector2 br = Vector2( float( texture->GetWidth() ), float( texture->GetHeight() ) ) / INTERIOR_SHADOW_MAP_MAX_RESOLUTION;
		float offset = 0.5f / INTERIOR_SHADOW_MAP_MAX_RESOLUTION;

		m_shadowFilterAreaVar = Vector4( tl.x + offset, tl.y + offset, br.x - offset, br.y - offset );
		m_shadowFilterVar = Vector4( 1.f / float( INTERIOR_SHADOW_MAP_MAX_RESOLUTION ), 0.f, 0.f, 0.f );
		Tr2Renderer::SetRenderTarget( 0, m_shadowMapTemporaryRTs[1], renderContext );
		Tr2Renderer::SetViewport( viewport );
		Tr2Renderer::DrawTexture( m_shadowFilter, m_shadowMapTemporaryRTs[0].GetTexture(), tl, br );

		// vertical filtering
		m_shadowFilterAreaVar = Vector4( tl.x + offset, tl.y + offset, br.x - offset, br.y - offset );
		m_shadowFilterVar = Vector4( 0.f, 1.f / float( INTERIOR_SHADOW_MAP_MAX_RESOLUTION ), 0.f, 0.f );
		Tr2Renderer::SetRenderTarget( 0, *texture->GetRenderTarget(), renderContext );
		Tr2Renderer::SetViewport( viewport0 );
		Tr2Renderer::DrawTexture( m_shadowFilter, m_shadowMapTemporaryRTs[1].GetTexture(), tl, br );
	}

	{
		CCP_STATS_ZONE( "Finalize" );

		m_shadowFilterAreaVar = Vector4( 0.0f, 0.0f, 1.f, 1.f );

		if( m_numSLIGroups > 1 )
		{
			ShadowReRenderInfo reRenderInfo;
			reRenderInfo.lightSource = item.lightSource;
			reRenderInfo.shadowMapIndex = item.shadowMapIndex;
			m_shadowReRenderInfo[reRenderInfo] = 1;
		}
		CCP_STATS_INC( wodInteriorSceneShadowsUpdated );
	}
	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::SetUmbraProperties( unsigned int properties )
{
	unsigned int width, height;
	width = Tr2Renderer::GetRenderTargetWidth();
	height = Tr2Renderer::GetRenderTargetHeight();

	m_sceneCamera.SetProperties( width, height, properties );
}

// ------------------------------------------------------------------------------------------------------
unsigned int Tr2InteriorScene::GetUmbraProperties()
{
    return m_sceneCamera.GetProperties();
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::SetVisualizeMethod( void )
{
	if( m_visualizeMethod == VM_EN_ALBEDO || m_visualizeMethod == VM_EN_TARGET_DETAIL )
	{
		m_displayDynamics = false;
	}
	else
	{
		m_displayDynamics = true;
	}

	// Pass down to cells
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->SetVisualizeMethod( m_visualizeMethod );
	}
}

//---------------------------------------------------------------------------------------
void Tr2InteriorScene::SetRenderBackgroundCubeMap( bool renderBackgroundCubeMap )
{
	m_renderBackgroundCubeMap = renderBackgroundCubeMap;
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds an interior light source to the scene.  An individual light source can only be
//   added to a scene once.  It is an error to add a NULL light source, and a log message
//   is printed saying so.
// Arguments:
//   lightSource - The light source to add to the interior scene (should not be NULL)
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::AddLightSource( ITr2InteriorLight* lightSource )
{
	// Bail out if the lightSource is NULL
	if( !lightSource )
	{
		CCP_LOGERR( "Attempt to add NULL light source to interior scene!" );
		return;
	}

	// See if this light is already in our list
	ssize_t pos = m_lights.FindKey( lightSource );
	if( pos == -1 )
	{
		m_lights.Insert( -1, lightSource );

		lightSource->EnableROI( m_enableROIs );
		lightSource->AddToScene();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes an interior light source from the scene.  The light source is also removed
//   from all cells containing it, and is removed from Umbra.  It is an error to remove a
//   NULL light source, and a log message is printed saying so.
// Arguments:
//   lightSource - The light source to remove from the interior scene (should not be NULL)
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::RemoveLightSource( ITr2InteriorLight* lightSource )
{
	// Bail out early if the light source is NULL
	if( !lightSource )
	{
		CCP_LOGERR( "Attempt to remove a NULL light source from interior scene!" );
		return;
	}

	// Find this light in our list
	ssize_t pos = m_lights.FindKey( lightSource );
	if( pos == -1 )
	{
		CCP_LOGERR("Tr2InteriorScenel::RemoveLightSource() - interiorLightSource not found in the scene!" );
		return;
	}

	// Remove this light source from the cells
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->RemoveLight( lightSource );
	}

	lightSource->RemoveFromScene();

	m_lights.Remove( pos );
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds a dynamic object to the scene.  If the dynamic object is not fully loaded, it
//   is added to a pending load queue for delayed add.  An individual dynamic object can
//   only be added to a scene once.  It is an error to add a NULL dynamic, and a log
//   message is printed saying so.
// Arguments:
//   dynamic - The dynamic to add to the interior scene (should not be NULL)
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::AddDynamic( ITr2InteriorDynamic* dynamic )
{
	// Bail out early if the dynamic is NULL
	if( !dynamic )
	{
		CCP_LOGERR(" Attempt to add NULL dynamic to interior scene!" );
		return;
	}

	// See if this dynamic is already in our list
	ssize_t pos = m_dynamics.FindKey( dynamic );
	if( pos == -1 )
	{
		CCP_ASSERT( !m_debugInsideSetLOD );

		if( !dynamic->AddToScene( m_apexScene ) && ( m_dynamicsPendingLoad.FindKey( dynamic ) == -1 ) )
		{
			m_dynamicsPendingLoad.Insert( -1, dynamic );
		}

		m_dynamics.Insert( -1, dynamic );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes a dynamic object from the scene.  The dynamic is also removed from all cells
//   containing it, and is removed from Umbra.  If the dynamic is in the pending load
//   queue, it is removed from the queue.  It is an error to remove a NULL dynamic, and
//   a log message is printed saying so.
// Arguments:
//   dynamic - The dynamic object to remove from the interior scene (should not be NULL)
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::RemoveDynamic( ITr2InteriorDynamic* dynamic )
{
	// Find this dynamic in our list
	ssize_t pos = m_dynamics.FindKey( dynamic );
	if( pos == -1 )
	{
		CCP_LOGERR("Tr2InteriorScene::RemoveDynamic() - interiorDynamic not found in the scene!" );
		return;
	}

	CCP_ASSERT( !m_debugInsideSetLOD );

	dynamic->RemoveFromScene();

	// Remove the dynamic from all the cells
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->RemoveDynamic( dynamic );
	}

	m_dynamics.Remove( pos );

	// See if this dynamic is in the pending load list
	pos = m_dynamicsPendingLoad.FindKey( dynamic );
	if( pos != -1 )
	{
		m_dynamicsPendingLoad.Remove( pos );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Moves the dynamic object in the dynamics list. This affects the order of updates .
// Arguments:
//   object - Dynamic object to move in the list (needs to be in the dynamics list)
//   insertAfter - Dynamic object to put the fist object after in the list (needs to be
//		in the dynamics list)
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::ReorderDynamic( ITr2InteriorDynamic* object, ITr2InteriorDynamic* insertAfter )
{
	ssize_t originalPos = m_dynamics.FindKey( object );
	if( originalPos == -1 )
	{
		CCP_LOGERR("Tr2InteriorScene::ReorderDynamic() - object not found in the scene!" );
		return;
	}
	ssize_t newPos = m_dynamics.FindKey( insertAfter );
	if( newPos == -1 )
	{
		CCP_LOGERR("Tr2InteriorScene::ReorderDynamic() - insertAfter not found in the scene!" );
		return;
	}

	CCP_ASSERT( !m_debugInsideSetLOD );
	m_dynamics.Move( originalPos, newPos );
}

// ------------------------------------------------------------------------------------------------------
// Description
//   This function marks all spotlight shadows as dirty to force their update during subsequent render
//	 calls.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::UpdateSpotlightShadows()
{
	for( PITr2InteriorLightVector::iterator it = m_lights.begin(); it != m_lights.end(); ++it )
	{
		for( unsigned int i = 0; i < ( *it )->GetRequiredShadowMapCount(); ++i )
		{
			( *it )->MarkShadowDirty( i, true );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
// Description
//   This function updates lights, dynamics and portals for dirty cells (cells that has been moved).
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::UpdateCells()
{
	CCP_STATS_ZONE( __FUNCTION__ );
	for( PTr2InteriorCellVector::iterator cellIt = m_cells.begin(); cellIt != m_cells.end(); ++cellIt )
	{
		Tr2InteriorCell* cell = *cellIt;

		// Rebuild cell bounding box and mark root portals for rebuild
		bool isCellDirty = cell->IsDirty();
		if( isCellDirty )
		{
			cell->RebuildBoundingBox();
			if( g_useRootCell && m_sceneUseRootCell )
			{
				UpdateRootPortalForCell( cell );
			}
		}

		if( isCellDirty && ( cell->IsUnbounded() || cell->IsBoundingBoxReady() ) )
		{
			for( PITr2InteriorLightVector::iterator lit = m_lights.begin(); lit != m_lights.end(); ++lit )
			{
				if( ( *lit )->TestCellIntersectionAndAdd( cell ) )
				{
					for( unsigned int i = 0; i < ( *lit )->GetRequiredShadowMapCount(); ++i )
					{
						( *lit )->MarkShadowDirty( i, true );
					}
				}
			}
			for( PITr2InteriorDynamicVector::iterator dit = m_dynamics.begin(); dit != m_dynamics.end(); ++dit )
			{
				if( ( *dit )->IsBoundingBoxReady() && ( *dit )->IsUmbraReady() )
				{
					( *dit )->TestCellIntersectionAndAdd( cell );
				}
			}
			for( PTr2InteriorPhysicalPortalVector::iterator it = m_portals.begin(); it != m_portals.end(); ++it )
			{
				( *it )->OnCellTransformChanged( cell );
			}

			std::map<Umbra::Cell*, std::pair<Umbra::Model*, Umbra::PhysicalPortal*> >::iterator it = m_rootPortals.find( cell->GetUmbraCell() );
			if( it != m_rootPortals.end() )
			{
				Umbra::Model* model = it->second.first;
				if( model )
				{
						model->release();
						model = NULL;
				}

				Umbra::PhysicalPortal* portal = it->second.second;
				if( portal )
				{
						portal->release();
						portal = NULL;
				}

				m_rootPortals.erase( it );
			}
			cell->ResetDirtyFlag();
		}
		cell->MarkShadowsDirtyForSkinnedObjects();
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called at the beginning of an Umbra query.  It begins by correcting
//   the handedness of the cameraToWorld matrix for Umbra's convention.  Then, if this is
//   a primary visibility query, it queues up a Tr2VisibilityEvent in the current result
//   set.  Otherwise, it immediately executes the event by calling DoQueryBegin.
// See Also:
//   OnQueryEnd, DoQueryBegin, DoQueryBeginPrePass
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnQueryBegin( void )
{
	// Umbra-ize the camera-to-world matrix
	XMVECTOR det;
	m_cameraToWorldMatrix = XMMatrixInverse( &det, m_cameraToWorldMatrix );
	if( Tr2Renderer::IsRightHanded() )
	{
		Vector3& zAxis = m_cameraToWorldMatrix.GetZ();
		zAxis = -zAxis;
	}

	// Setup visibility event
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::QUERY_BEGIN;

	// Main visibility query
	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		m_visibleLights.clear();
		// Add to the result set
		m_visibilityResults->AddVisibilityEvent( event );
	}
	// Query for shadows
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		// Do query begin
		DoQueryBegin( event, IMMEDIATE_GATHER );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called at the end of an Umbra query.  If this is a primary
//   visibility query, it queues up a Tr2VisibilityEvent in the current result set.
//   Otherwise, it immediately executes the event by calling DoQueryEnd.
// See Also:
//   OnQueryBegin, DoQueryEnd
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnQueryEnd( void )
{
	// Setup visibility event
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::QUERY_END;

	// Main visibility query
	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		// Add to the result set
		m_visibilityResults->AddVisibilityEvent( event );
	}
	// Query for shadows
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		// Do query end
		DoQueryEnd( event, IMMEDIATE_GATHER );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called when the visibility query enters a portal.  It checks the
//   portal user data, and if the user data is a mirror, it sets the placeable hosting
//   the mirror as the Tr2VisibilityEvent user data.  If this is a primary visibility
//   query, this function queues up the Tr2VisibilityEvent in the current result set.
//   Otherwise it executes the event immediately by calling DoPortalEnter.
// See Also:
//   OnPortalExit, OnPortalPreExit, DoPortalEnter, DoPortalEnterPrePass
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnPortalEnter( void )
{
	// Get the IRoot pointer from the instance user data
	const Umbra::Commander::Instance* instance = getInstance();
	Tr2InteriorMirror* mirror = reinterpret_cast<Tr2InteriorMirror*>(instance->getUserPointer());

	if( m_rootCell == NULL || m_currentCameraCell != m_rootCell || mirror != NULL )
	{
		auto visitedObject = new std::unordered_set<IRoot*>;
		if( !mirror )
		{
			visitedObject->insert( ( *m_visitedObjects.rbegin() )->begin(), ( *m_visitedObjects.rbegin() )->end() );
			
		}
		m_visitedObjects.push_back( visitedObject );
	}

	// Setup visibility event
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::PORTAL_ENTER;
	if( mirror )
	{
		event.m_userData = mirror->GetPlaceable()->GetRawRoot();
		event.m_mirrorIndex = mirror->GetMirrorIndex();
	}

	// Main visibility query
	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		// Add to the result set
		m_visibilityResults->AddVisibilityEvent( event );
	}
	// Query for shadows
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		// Do portal enter
		DoPortalEnter( event, IMMEDIATE_GATHER );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called when the visibility query exits a portal.  It checks the
//   portal user data, and if the user data is a mirror, it sets the placeable hosting
//   the mirror as the Tr2VisibilityEvent user data.  It also computes the current
//   objectToWorld matrix and sets it on the event.  If this is a primary visibility
//   query, this function queues up the Tr2VisibilityEvent in the current result set.
//   Otherwise it executes the event immediately by calling DoPortalExit.
// See Also:
//   OnPortalEnter, OnPortalPreExit, DoPortalExit, DoPortalExitPrePass
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnPortalExit( void )
{
	// Get the IRoot pointer from the instance user data
	const Umbra::Commander::Instance* instance = getInstance();
	Tr2InteriorMirror* mirror = reinterpret_cast<Tr2InteriorMirror*>(instance->getUserPointer());

	if( m_rootCell == NULL || m_currentCameraCell != m_rootCell || mirror != NULL )
	{
		// Check if we are exiting one of the "camera" portals (built around the camera
		// when it is located in the intersection of several cells). If we are we need
		// to copy all visited objects into the previous stack slot since the camera
		// portal can overlap with normal physical portal.
		Umbra::Object* portal = instance->getObject();
		if( std::find( m_cameraPortals.begin(), m_cameraPortals.end(), portal ) != m_cameraPortals.end() )
		{
			if( m_visitedObjects.size() > 1 )
			{
				auto previous = m_visitedObjects[m_visitedObjects.size() - 2];
				previous->insert( m_visitedObjects.back()->begin(), m_visitedObjects.back()->end() );					
				delete *m_visitedObjects.rbegin();
				m_visitedObjects.pop_back();
			}
			// If the "camera" portal is the first one visited - just leave
			// visited objects stack untouched.
		}
		else
		{
			delete *m_visitedObjects.rbegin();
			m_visitedObjects.pop_back();
		}
	}

	// IMPORTANT NOTE:
	// Umbra gives an object-to-camera matrix per visible instance.  This is the OpenGL
	// MODEL_VIEW matrix, not a matrix we can directly use in D3D.  We need the WorldMat
	// (object-to-world matrix), so we multiply Umbra's object-to-camera matrix by the
	// camera-to-world matrix (the inverse ViewMat) to get the matrix we need.
	Umbra::Matrix4x4 tempMat;
	instance->getObjectToCameraMatrix( tempMat );
	Matrix objectToCameraMatrix( &tempMat.m[0][0] );

	// Setup visibility event
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::PORTAL_EXIT;
	if( mirror )
	{
		event.m_userData = mirror->GetPlaceable()->GetRawRoot();
		event.m_mirrorIndex = mirror->GetMirrorIndex();
	}
	event.m_objectToWorldMatrix = objectToCameraMatrix * m_cameraToWorldMatrix;

	// Main visibility query
	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		// Add to the result set
		m_visibilityResults->AddVisibilityEvent( event );
	}
	// Query for shadows
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		// Do portal exit
		DoPortalExit( event, IMMEDIATE_GATHER );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called before the visibility query exits a portal.  It checks the
//   portal user data, and if the user data is a mirror, it sets the placeable hosting
//   the mirror as the Tr2VisibilityEvent user data.  If this is a primary visibility
//   query, this function queues up the Tr2VisibilityEvent in the current result set.
//   Otherwise it executes the event immediately by calling DoPortalPreExit.
// See Also:
//   OnPortalEnter, OnPortalExit, DoPortalPreExit, DoPortalPreExitPrePass
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnPortalPreExit( void )
{
	// Get the IRoot pointer from the instance user data
	const Umbra::Commander::Instance* instance = getInstance();
	Tr2InteriorMirror* mirror = reinterpret_cast<Tr2InteriorMirror*>(instance->getUserPointer());

	// Setup visibility event
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::PORTAL_PRE_EXIT;
	if( mirror )
	{
		event.m_userData = mirror->GetPlaceable()->GetRawRoot();
		event.m_mirrorIndex = mirror->GetMirrorIndex();
	}

	// Main visibility query
	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		// Add to the result set
		m_visibilityResults->AddVisibilityEvent( event );
	}
	// Query for shadows
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		// Do portal pre-exit
		DoPortalPreExit( event, IMMEDIATE_GATHER );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called when Umbra requests a change to the view parameters, when
//   entering or exiting a portal.  This function gets the mirroring flag, the new
//   scissor rectangle, and the user clip plane from Umbra.  It sets the relevant
//   parameters on the Tr2VisibilityEvent.  If this is a primary visibility query, this
//   function queues up the Tr2VisibilityEvent in the current result set.  Otherwise it
//   executes the event immediately by calling DoViewParametersChanged.
// See Also:
//   DoViewParametersChanged
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnViewParametersChanged( void )
{
	// Get the viewer object from Umbra, which will supply us with new view parameters
	const Umbra::Commander::Viewer* viewer = getViewer();

	// Setup visibility event
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::VIEW_PARAMETERS_CHANGED;
	event.m_isMirroredInLeftHandedSpace = viewer->isMirrored();

	if( m_rootCell != NULL && m_currentCameraCell == m_rootCell && viewer->getFrustumPlaneCount() <= 6 )
	{
		event.m_deferScissorRectangle = true;
	}
	else
	{
		int left, top, right, bottom;
		viewer->getScissor( left, top, right, bottom );
		event.m_scissorRect.left = left;
		event.m_scissorRect.top = top;
		event.m_scissorRect.right = right;
		event.m_scissorRect.bottom = bottom;
	}
	event.m_useClipPlane = false;
	if( viewer->getFrustumPlaneCount() > 6 )
	{
		Umbra::Vector4 plane;
		viewer->getFrustumPlane( 6, plane );

		event.m_clipPlane = XMPlaneTransform(
			XMVectorSet( plane.x, plane.y, -plane.z, plane.w ),
			XMMatrixTranspose( Tr2Renderer::GetInverseProjectionTransform() ) );

		event.m_useClipPlane = true;
	}

	// Main visibility query
	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		// Add to the result set
		m_visibilityResults->AddVisibilityEvent( event );
	}
	// Query for shadows
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		// Do view parameters changed
		DoViewParametersChanged( event, IMMEDIATE_GATHER );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called when Umbra encounters an instance of a visible object.  This
//   function checks the Umbra user data & verifies that it is an interior object.  Then
//   it computes the objectToWorld matrix and sets the parameters and user data on the
//   Tr2VisibilityEvent.  If this is a primary visibility query, this function queues up
//   the Tr2VisibilityEvent in the current result set.  Otherwise it  executes the event
//   immediately by calling DoInstanceVisible.
// See Also:
//   DoInstanceVisible
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnInstanceVisible( void )
{
	// Get the IRoot pointer from the instance user data
	const Umbra::Commander::Instance* instance = getInstance();

	IRoot* iroot = CONVERT_FROM_UMBRA_USER_DATA( instance->getUserPointer() );

	if ( !iroot )
	{
		return;
	}

	if( m_visibilityQueryType != PICKING_QUERY )
	{
		if( ! ( *m_visitedObjects.rbegin() )->insert( iroot ).second )
		{
			return;
		}
	}

	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		ITr2InteriorLight* light = dynamic_cast<ITr2InteriorLight*>( iroot );
		if( light )
		{
			m_visibleLights.insert( light );
		}
	}

	// IMPORTANT NOTE:
	// Umbra gives an object-to-camera matrix per visible instance.  This is the OpenGL
	// MODEL_VIEW matrix, not a matrix we can directly use in D3D.  We need the WorldMat
	// (object-to-world matrix), so we multiply Umbra's object-to-camera matrix by the
	// camera-to-world matrix (the inverse ViewMat) to get the matrix we need.
	Umbra::Matrix4x4 tempMat;
	instance->getObjectToCameraMatrix( tempMat );
	Matrix objectToCameraMatrix( &tempMat.m[0][0] );

	// Setup visibility event
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::INSTANCE_VISIBLE;
	event.m_userData = iroot;
	event.m_objectToWorldMatrix = objectToCameraMatrix * m_cameraToWorldMatrix;

	// Main visibility query
	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		// Add to the result set
		m_visibilityResults->AddVisibilityEvent( event );
	}
	// Query for shadows
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		// Do instance visible
		DoInstanceVisible( event, IMMEDIATE_GATHER );
	}
	// Picking query
	else if( m_visibilityQueryType == PICKING_QUERY )
	{
		ITr2Renderable* renderable = dynamic_cast<ITr2Renderable*>( iroot );
		if( renderable )
		{
			m_visibleObjects.push_back( renderable );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called when Umbra activates a light's region of influence.  It
//   checks to see if the Umbra user data is an interior light source, then computes
//   the light's objectToWorld matrix.  It sets the appropriate parameters and user data
//   on the Tr2VisibilityEvent.   If this is a primary visibility query, this function
//   queues up the Tr2VisibilityEvent in the current result set.  Otherwise it executes
//   the event immediately by calling DoRegionOfInfluenceActive.
// See Also:
//   OnRegionOfInfluenceInactive, DoRegionOfInfluenceActive
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnRegionOfInfluenceActive( void )
{
	// Get the IRoot pointer from the instance user data
	const Umbra::Commander::Instance* instance = getInstance();
	ITr2InteriorLight* lightSource = dynamic_cast<ITr2InteriorLight*>( CONVERT_FROM_UMBRA_USER_DATA( instance->getUserPointer() ) );

	// IMPORTANT NOTE:
	// Umbra gives an object-to-camera matrix per visible instance.  This is the OpenGL
	// MODEL_VIEW matrix, not a matrix we can directly use in D3D.  We need the WorldMat
	// (object-to-world matrix), so we multiply Umbra's object-to-camera matrix by the
	// camera-to-world matrix (the inverse ViewMat) to get the matrix we need.
	Umbra::Matrix4x4 tempMat;
	instance->getObjectToCameraMatrix( tempMat );
	Matrix objectToCameraMatrix( &tempMat.m[0][0] );

	// Setup visibility event
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::REGION_OF_INFLUENCE_ACTIVE;
	event.m_userData = lightSource;
	event.m_objectToWorldMatrix = objectToCameraMatrix * m_cameraToWorldMatrix;

	// Main visibility query
	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		// Add to the result set
		m_visibilityResults->AddVisibilityEvent( event );
	}
	// Query for shadows
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		// Do region of influence active
		DoRegionOfInfluenceActive( event, IMMEDIATE_GATHER );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called when Umbra deactivates a light's region of influence.  It
//   checks to see if the Umbra user data is an interior light source.  It sets the
//   appropriate parameters and user data on the Tr2VisibilityEvent.   If this is a
//   primary visibility query, this function queues up the Tr2VisibilityEvent in the
//   current result set.  Otherwise it executes the event immediately by calling
//   DoRegionOfInfluenceInactive.
// See Also:
//   OnRegionOfInfluenceActive, DoRegionOfInfluenceInctive
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnRegionOfInfluenceInactive( void )
{
	// Get the IRoot pointer from the instance user data
	const Umbra::Commander::Instance* instance = getInstance();
	ITr2InteriorLight* lightSource = dynamic_cast<ITr2InteriorLight*>( CONVERT_FROM_UMBRA_USER_DATA( instance->getUserPointer() ) );

	// Setup visibility event
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::REGION_OF_INFLUENCE_INACTIVE;
	event.m_userData = lightSource;

	// Main visibility query
	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		// Add to the result set
		m_visibilityResults->AddVisibilityEvent( event );
	}
	// Query for shadows
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		// Do region of influence inactive
		DoRegionOfInfluenceInactive( event, IMMEDIATE_GATHER );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called when Umbra needs to draw a stencil buffer mask to isolate
//   the surface of a portal.  It checks that the Umbra user data is a mirror, then
//   computes the objectToWorld matrix.  It gets the stencilWrite and stencilTest values
//   from Umbra, then sets the appropriate parameters and user data on the
//   Tr2VisibilityEvent.  If this is a primary visibility query, this function queues up
//   the Tr2VisibilityEvent in the current result set.  Otherwise it executes the event
//   immediately by calling DoStencilMask.
// See Also:
//   OnStencilMask
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnStencilMask( void )
{
	// Get the IRoot pointer from the instance user data
	const Umbra::Commander::Instance* instance = getInstance();
	Tr2InteriorMirror* mirror = reinterpret_cast<Tr2InteriorMirror*>(instance->getUserPointer());

	// IMPORTANT NOTE:
	// Umbra gives an object-to-camera matrix per visible instance.  This is the OpenGL
	// MODEL_VIEW matrix, not a matrix we can directly use in D3D.  We need the WorldMat
	// (object-to-world matrix), so we multiply Umbra's object-to-camera matrix by the
	// camera-to-world matrix (the inverse ViewMat) to get the matrix we need.
	Umbra::Matrix4x4 tempMat;
	instance->getObjectToCameraMatrix( tempMat );
	Matrix objectToCameraMatrix( &tempMat.m[0][0] );

	// Setup visibility event
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::STENCIL_MASK;
	if( mirror )
	{
		event.m_userData = mirror->GetPlaceable()->GetRawRoot();
		event.m_mirrorIndex = mirror->GetMirrorIndex();
	}
	event.m_objectToWorldMatrix = objectToCameraMatrix * m_cameraToWorldMatrix;
	getStencilValues( event.m_stencilTest, event.m_stencilWrite );

	// Main visibility query
	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		// Add to the result set
		m_visibilityResults->AddVisibilityEvent( event );
	}
	// Query for shadows
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		// Do stencil mask
		DoStencilMask( event, IMMEDIATE_GATHER );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called when Umbra enters a new cell.  This callback executes early
//   enough in the visibility query that changes can be made to the cell's contents.
//   This function is used to add background proxy dynamics to the current cell, ensuring
//   that proxies are visible from all cells.  As this function only affects the internal
//   behavior of the remainder of the current query, it does not generate a visibility
//   for the result set.
// ---------------------------------------------------------------------------------------
void Tr2InteriorScene::OnCellImmediateReport( void )
{
	Umbra::Cell* cell = getCell();

	for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin();
		 it != m_dynamics.end(); ++it )
	{
		ITr2InteriorDynamic* dynamic = *it;

		if( dynamic->IsBackgroundProxy() )
		{
			dynamic->AddToCellAsBackgroundProxy( cell );
		}
	}
}

void Tr2InteriorScene::OnDrawLine3D( void )
{
	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		if( m_debugLines )
		{
			Umbra::Vector3 a;
			Umbra::Vector3 b;
			Umbra::Vector4 color;
			getLine3D( a, b, color );

			Color drawColor( color.x, color.y, color.z, color.w );

			m_debugLines->Add( Vector3( a.x, a.y, a.z ), drawColor, Vector3( b.x, b.y, b.z ), drawColor );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function executes a query begin event in a prepass batch gathering operation.
//   It initializes a bunch of state variables needed for tracking the query through the
//   scene.  Unlike during DoQueryBeginForwardPass, it does not initialize the
//   transparency stack because pre-pass, light-pass, and shadows ignore transparency.
//   It also skips the background cubemap, since the cubemap should only draw in the
//   forward pass.
// Arguments:
//   event		- The query begin event to execute.
//   gatherType - The type of batches we are gathering
// See Also:
//   OnQueryBegin
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoQueryBegin( const Tr2VisibilityEvent& event,
									 BatchGatherType gatherType )
{
	CCP_ASSERT( event.m_eventType == Tr2VisibilityEvent::QUERY_BEGIN );

	// Not initially mirrored
	m_isMirroredInLeftHandedSpace = false;
	m_mirrorDepth = 0;

	// Clear mirror matrix
	m_mirrorToWorldMatrix = Tr2Renderer::GetIdentityTransform();
	m_mirrorToWorldMatrixStack.clear();
	m_mirrorToWorldMatrixStack.push_back( m_mirrorToWorldMatrix );

	// Flag the first view parameter change so we can ignore it
	m_firstViewParameterChange = true;

	// Clear visible objects
	m_visibleObjects.clear();

	// Clear lights
	m_activeLightSet.Clear();

	// Clear the portal entry flag and object group counter
	m_enteringPortal = false;
	m_currentObjectGroup = 0;

	// Clear stencil stack
	m_stencilStack.clear();

	// Clear the transparency stack
	m_transparencyStack.clear();
	m_transparencyStack.push_back( std::make_pair( 0, 0 ) );

	// If we're gathering forward batches, then we need a background cubemap batch here
	if( gatherType == PREPASS_FORWARD_GATHER || gatherType == FULL_FORWARD_GATHER )
	{
		PrepareBackgroundCubemapBatch( m_activePrimaryRenderBatches );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function executes a query end event in a normal batch gathering operation.  If
//   there are outstanding transparent batches for the camera cell, it first queues up
//   a batch to disable the stencil test.  Then it adds the remaining transparent batches
//   to the primary accumulator in back-to-front order.
// Arguments:
//   event		- The query end event to execute.
//   gatherType - The type of batches we are gathering
// See Also:
//   OnQueryEnd
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoQueryEnd( const Tr2VisibilityEvent& event,
								   BatchGatherType gatherType )
{
	CCP_ASSERT( event.m_eventType == Tr2VisibilityEvent::QUERY_END );

	if( m_activePrimaryRenderBatches && m_activeTransparentBatchStore && !m_transparencyStack.empty() )
	{
		// Verify that we have transparent batches to render.  This will be true
		// if batchRange.first < batchRange.second, meaning there is a non-empty
		// set of transparent batches to render.
		std::pair<int, int> batchRange = m_transparencyStack.back();
		if( batchRange.first < batchRange.second )
		{
			// If we are gathering batches for the forward pass, we need to disable the
			// stencil test before drawing the final set of transparent batches
			if( gatherType == PREPASS_FORWARD_GATHER || gatherType == FULL_FORWARD_GATHER )
			{
				Tr2InteriorStencilMaskBatch* stencilBatch =
					m_activePrimaryRenderBatches->Allocate<Tr2InteriorStencilMaskBatch>();

				if( stencilBatch )
				{
					stencilBatch->SetShaderMaterial( NULL );
					stencilBatch->SetPerObjectData( NULL );
					stencilBatch->SetGeometryResource( NULL );
					stencilBatch->SetMeshParameters( 0, 0, 0 );

					// This flag disables the stencil test
					stencilBatch->SetDisableStencil( true );

					stencilBatch->SetRenderingMode( Tr2EffectStateManager::RM_ANY );
					m_activePrimaryRenderBatches->SetUserData(
						ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_BEGIN ) );

					m_activePrimaryRenderBatches->Commit( stencilBatch );
				}
			}

			// Skip transparency gather during prepass
			if( gatherType != PREPASS_GATHER )
			{
				// Collect the transparent batches from the specified batch range
				for( int i = batchRange.second - 1; i >= batchRange.first; --i )
				{
					// Get the correct rendering mode
					Tr2EffectStateManager::RenderingMode mode = gatherType == LIGHT_GATHER ?
						Tr2EffectStateManager::RM_LIGHT : Tr2EffectStateManager::RM_ALPHA;

					// Set the rendering mode
					m_activePrimaryRenderBatches->SetRenderingMode( mode );

					m_activePrimaryRenderBatches->SetUserData(
						ConstructKey( m_currentObjectGroup, gatherType == LIGHT_GATHER ? WODINTBATCHGROUP_OPAQUE : WODINTBATCHGROUP_BLEND ) );
					// Transfer the transparent batch to the primary batch accumulator
					m_activeTransparentBatchStore->TransferBatchToOtherAccumulator(
						m_activePrimaryRenderBatches, i );
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function executes a portal enter event in a normal batch gathering operation.
//   It increments the object group and sets the portal enter flag.  If there is an
//   active transparent batch store, it pushes the transparency stack.  Finally, if the
//   event user data is a mirror, it computes the new mirrorToWorld matrix and adds it
//   to the stack.
// Arguments:
//   event		- The portal enter event to execute.
//   gatherType - The type of batches to gather.
// See Also:
//   OnPortalEnter, DoPortalEnterPrePass
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoPortalEnter( const Tr2VisibilityEvent& event,
									  BatchGatherType gatherType )
{
	CCP_ASSERT( event.m_eventType == Tr2VisibilityEvent::PORTAL_ENTER );

	// Increment the object group id & flag that we're entering a portal
	m_enteringPortal = true;
	++m_currentObjectGroup;

	// Push the transparency stack
	// Note that the top of the stack is the active transparent batch range,
	// so any batches added in the cell we're entering will increment the
	// end of the range (second value in the pair)
	if( m_activeTransparentBatchStore )
	{
		// Get the current transparent batch count
		int startBatch = (int)m_activeTransparentBatchStore->GetBatchCount();
		// Note that the start & end batches are the same at this point, as we haven't
		// accumulated any transparent batches for the new cell on the other side of
		// the portal
		m_transparencyStack.push_back( std::make_pair( startBatch, startBatch ) );
	}

	if( event.m_mirrorIndex != -1 )
	{
		// Get a handle to the placeable
		IRoot* iroot = event.m_userData;
		Tr2InteriorPlaceable* placeable = dynamic_cast<Tr2InteriorPlaceable*>( iroot );

		if( placeable )
		{
			Tr2InteriorMirror* mirror = placeable->GetMirror( event.m_mirrorIndex );

			if( mirror )
			{
				// We're going into a mirror, increment the mirror depth
				++m_mirrorDepth;

				// Get the inverse transform and warp matrices from the mirror
				XMVECTOR det;
				Matrix transformBack(
					XMMatrixInverse( &det, mirror->GetTransformMatrix() ) );
				Matrix warpMatrixBack(
					XMMatrixInverse( &det, mirror->GetWarpMatrixBack() ) );

				// Compute the mirror matrix
				Matrix mirrorMatrix(
					XMMatrixMultiply(
						XMMatrixMultiply( transformBack, warpMatrixBack ),
						XMMatrixMultiply( mirror->GetWarpMatrixFront(), mirror->GetTransformMatrix() ) ) );

				// Need the inverse-transpose for fixing up vectors in mirror space
				mirrorMatrix = XMMatrixTranspose(
					XMMatrixInverse( &det, mirrorMatrix ) );

				// Store the current transform in the stack
				m_mirrorToWorldMatrixStack.push_back( mirrorMatrix );

				// Update the accumulated mirror-to-world matrix
				m_mirrorToWorldMatrix = Tr2Renderer::GetIdentityTransform();
				for( std::vector<Matrix>::iterator it = m_mirrorToWorldMatrixStack.begin();
					it != m_mirrorToWorldMatrixStack.end(); ++it )
				{
					m_mirrorToWorldMatrix *= ( *it );
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function executes a portal exit event in a normal batch gathering operation.
//   It increments the object group.  If the event user data is a mirror, it pops the
//   mirrorToWorld matrix stack and queues up a stencil mask batch to decrement the
//   stencil buffer and restore depth to the plane of the mirror.
// Arguments:
//   event		- The portal exit event to execute.
//   gatherType - The type of batches to gather
// See Also:
//   OnPortalExit, DoPortalExit
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoPortalExit( const Tr2VisibilityEvent& event,
									 BatchGatherType gatherType )
{
	CCP_ASSERT( event.m_eventType == Tr2VisibilityEvent::PORTAL_EXIT );

	// Early exit if there are no active primary batches
	if( !m_activePrimaryRenderBatches )
	{
		return;
	}

	if( event.m_mirrorIndex != -1 )
	{
		// Get a handle to the placeable
		IRoot* iroot = event.m_userData;
		Tr2InteriorPlaceable* placeable = dynamic_cast<Tr2InteriorPlaceable*>( iroot );

		if( placeable )
		{
			Tr2InteriorMirror* mirror = placeable->GetMirror( event.m_mirrorIndex );

			if( mirror )
			{
				// Coming out of a mirror, decrement the mirror depth
				--m_mirrorDepth;

				m_mirrorToWorldMatrixStack.pop_back();

				// Update the accumulated mirror-to-world matrix
				m_mirrorToWorldMatrix = Tr2Renderer::GetIdentityTransform();
				for( std::vector<Matrix>::iterator it = m_mirrorToWorldMatrixStack.begin();
					it != m_mirrorToWorldMatrixStack.end(); ++it )
				{
					m_mirrorToWorldMatrix *= ( *it );
				}

				if( gatherType == PREPASS_FORWARD_GATHER || gatherType == FULL_FORWARD_GATHER )
				{
					// Get the per-object data for the opaque batches
					Tr2PerObjectData* perObjectOpaque =
						placeable->GetPerObjectDataWithPerInstanceLighting(
							m_activePrimaryRenderBatches,
							&m_activeLightSet,
							event.m_objectToWorldMatrix,
							m_mirrorToWorldMatrix
						);

					m_activePrimaryRenderBatches->SetRenderingMode(
						Tr2EffectStateManager::RM_ANY );
					m_activePrimaryRenderBatches->SetUserData(
						ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_END ) );

					// Setup stencil batch parameters
					WodStencilBatchParams params =
					{
						mirror->GetMeshIndex(),
						mirror->GetAreaIndex(),
						m_stencilStack.back().second,
						m_stencilStack.back().first,
						D3DSTENCILOP_DECR,
						false, // This flag indicates that we're coming out of a mirror,
						// so we don't need to clear depth
						true // We do render mirror material on forward pass
					};

					// Set the stencil parameters
					placeable->SetStencilParameters( params );

					m_activePrimaryRenderBatches->SetRenderingMode( Tr2EffectStateManager::RM_ALPHA );
					// Get the stencil batches
					placeable->GetBatches( m_activePrimaryRenderBatches,
						                   TRIBATCHTYPE_MIRROR,
										   perObjectOpaque );

					// Pop the stencil stack
					m_stencilStack.pop_back();
				}
				else if( gatherType == LIGHT_GATHER || gatherType == PREPASS_GATHER )
				{
					Tr2InteriorStencilMaskBatch* stencilBatch =
						m_activePrimaryRenderBatches->Allocate<Tr2InteriorStencilMaskBatch>();

					if( stencilBatch )
					{
						stencilBatch->SetShaderMaterial( NULL );
						stencilBatch->SetPerObjectData( NULL );
						stencilBatch->SetGeometryResource( NULL );
						stencilBatch->SetMeshParameters( 0, 0, 0 );

						// This flag disables the stencil test
						stencilBatch->SetDisableStencil( false );
						stencilBatch->SetStencilTest( m_stencilStack.back().second );

						stencilBatch->SetRenderingMode( Tr2EffectStateManager::RM_ANY );
						m_activePrimaryRenderBatches->SetUserData(
							ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_END ) );

						m_activePrimaryRenderBatches->Commit( stencilBatch );

						// Pop the stencil stack
						m_stencilStack.pop_back();
					}
				}
			}
		}
	}

	// Increment the current object group
	++m_currentObjectGroup;
}

// --------------------------------------------------------------------------------------
// Description
//   This function executes a portal pre-exit event in a normal batch gathering operation.
//   It queues up any outstanding transparent batches for the cell it is exiting and pops
//   the transparency stack.
// Arguments:
//   event		- The portal pre-exit event to execute.
//   gatherType - The type of batches to gather
// See Also:
//   OnPortalPreExit, DoPortalPreExitPrePass
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoPortalPreExit( const Tr2VisibilityEvent& event,
									    BatchGatherType gatherType )
{
	CCP_ASSERT( event.m_eventType == Tr2VisibilityEvent::PORTAL_PRE_EXIT );

	// Skip transparency gather during prepass
	if( gatherType != PREPASS_GATHER && m_activeTransparentBatchStore && m_activePrimaryRenderBatches)
	{
		// Coming out of a portal, accumulate transparent batches
		std::pair<int, int> batchRange = m_transparencyStack.back();
		if( batchRange.first < batchRange.second )
		{
			for( int i = batchRange.second - 1; i >= batchRange.first; --i )
			{
				// Get the correct rendering mode
				Tr2EffectStateManager::RenderingMode mode = gatherType == LIGHT_GATHER ?
					Tr2EffectStateManager::RM_LIGHT : Tr2EffectStateManager::RM_ALPHA;

				// Set the rendering mode
				m_activePrimaryRenderBatches->SetRenderingMode( mode );

				// Construct the sort key
				m_activePrimaryRenderBatches->SetUserData(
					ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_BLEND ) );
				// Transfer the transparent batch to the primary batch accumulator
				m_activeTransparentBatchStore->TransferBatchToOtherAccumulator(
					m_activePrimaryRenderBatches, i );
			}
		}

		// Pop the transparency stack
		m_transparencyStack.pop_back();
	}

	// Flag that we're no longer entering a portal
	m_enteringPortal = false;
}

// --------------------------------------------------------------------------------------
// Description
//   This function executes a view parameters changed event in a normal batch gathering
//   operation.  It queues up a clipping batch to set the cull mode, scissor rectangle,
//   and user clip plane.  This batch will appear at the beginning or end of the current
//   object group, depending on whether the query is entering or exiting a cell, as
//   determined by the preceding portal events.
// Arguments:
//   event - The view parameters changed event to execute.
//   gatherType - The type of batches to gather.
// See Also:
//   OnPortalViewParametersChanged
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoViewParametersChanged( const Tr2VisibilityEvent& event,
											    BatchGatherType gatherType )
{
	CCP_ASSERT( event.m_eventType == Tr2VisibilityEvent::VIEW_PARAMETERS_CHANGED );

	// Bail out if there is no active opaque batch accumulator
	if( !m_activePrimaryRenderBatches )
	{
		return;
	}

	if( gatherType == FLARE_GATHER || m_visibilityQueryType == SHADOW_QUERY )
	{
		return;
	}

	// Get the isMirrored state from the event
	m_isMirroredInLeftHandedSpace = event.m_isMirroredInLeftHandedSpace;

	// Get the mirrored state
	bool isMirrored = IsMirrored();

	// Insert the new clipping batch into the opaques list
	Tr2InteriorClippingBatch* batch =
		m_activePrimaryRenderBatches->Allocate<Tr2InteriorClippingBatch>();
	if( batch )
	{
		// Need to offset scissor rect by the actual viewport origin
		int x = Tr2Renderer::GetViewport().x;
		int y = Tr2Renderer::GetViewport().y;
		Rect scissorRect;
		if( event.m_deferScissorRectangle )
		{
			const TriViewport &viewport = Tr2Renderer::GetViewport();
			scissorRect.left = 0;
			scissorRect.top = 0;
			scissorRect.right = viewport.width;
			scissorRect.bottom = viewport.height;
		}
		else
		{
			scissorRect.left = event.m_scissorRect.left;
			scissorRect.top = event.m_scissorRect.top;
			scissorRect.right = event.m_scissorRect.right;
			scissorRect.bottom = event.m_scissorRect.bottom;
		}
		scissorRect.left += x;
		scissorRect.top += y;
		scissorRect.right += x;
		scissorRect.bottom += y;

		batch->SetPerFramePSData( m_perFramePSData );
		batch->SetScissorRect( scissorRect );
		batch->SetInvertedCullMode( isMirrored );
		batch->SetClipPlane( event.m_clipPlane );
		batch->UseClipPlane( event.m_useClipPlane );
		batch->SetRenderingMode( Tr2EffectStateManager::RM_ANY );
		if( m_enteringPortal )
		{
			m_activePrimaryRenderBatches->SetUserData(
				ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_BEGIN ) );
		}
		else
		{
			m_activePrimaryRenderBatches->SetUserData(
				ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_END ) );
		}

		m_activePrimaryRenderBatches->Commit( batch );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function executes an instance visible event in a normal batch gathering
//   operation.  It queues up opaque and decal batches from the renderable and any
//   attached renderables.  If the renderable has transparent batches, it adds those to
//   the current transparent batch store.
// Arguments:
//   event		- The instance visible event to execute.
//   gatherType - The type of batches to gather.
// See Also:
//   OnInstanceVisible, OnInstanceVisiblePrePass
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoInstanceVisible( const Tr2VisibilityEvent& event,
										  BatchGatherType gatherType )
{
	CCP_STATS_ZONE( "DoInstanceVisible" );

	CCP_ASSERT( event.m_eventType == Tr2VisibilityEvent::INSTANCE_VISIBLE );
	CCP_ASSERT( event.m_userData != NULL );

	bool opaqueOnly = false;
	if( m_visualizeMethod >= VM_EN_ONLY && m_visualizeMethod <= VM_EN_TARGET_CHARTS ||
		m_visualizeMethod >= VM_ALL_LIGHTING && m_visualizeMethod <= VM_LIGHT_PRE_PASS_SPECULAR_LIGHTING )
	{
		opaqueOnly = true;
	}

	// Bail out early if there are no active batch accumulators
	if( !m_activePrimaryRenderBatches && !m_activeTransparentBatchStore )
	{
		return;
	}

	// Get the IRoot userdata
	IRoot* iroot = event.m_userData;

	// Handle light-pass differently, since ITr2InteriorLights aren't ITr2Interiors
	if( gatherType == LIGHT_GATHER )
	{
		// Get a handle to the Wod interior light
		ITr2InteriorLight* light = dynamic_cast<ITr2InteriorLight*>( iroot );

		// Bail out if the user-data isn't a light source
		if( !light || !light->UseWithPrimaryLighting() || !m_activeTransparentBatchStore )
		{
			return;
		}

		m_activeTransparentBatchStore->SetRenderingMode( Tr2EffectStateManager::RM_LIGHT );
		// If we're in a mirrored state, we need to get per-instance information for the light
		if( m_mirrorDepth > 0 )
		{
			// Add the instanced light to the active light set
			light->GetInstancedBatches( m_activeTransparentBatchStore, m_mirrorToWorldMatrix );
		}
		else
		{
			// Add the non-instanced light to the active light set
			light->GetBatches( m_activeTransparentBatchStore );
		}

		// Update batch count to indicate the range of transparent batches gathered
		// for this cell
		m_transparencyStack.back().second =
			(unsigned int)m_activeTransparentBatchStore->GetBatchCount();
	}
	else
	{
		// Get a handle to the Wod interior object
		ITr2Interior* interior = dynamic_cast<ITr2Interior*>( iroot );
		// Cast the interior to a renderable
		ITr2Renderable* renderable = dynamic_cast<ITr2Renderable*>( interior );

		if( interior && renderable )
		{
			// Handle Bert's filter list
			if( m_useFilterList )
			{
				// this is fairly slow obviously, but the filter list is supposed to be extremely small,
				// and only used in semi-exotic scenario's where fps isn't the end of the world (eg. debug views)
				if( std::find( m_filterList.begin(), m_filterList.end(), interior ) == m_filterList.end() )
				{
					return;
				}
			}

			// Store the renderable in the visible objects list
			m_visibleObjects.push_back( renderable );

			if( m_visibilityQueryType == SHADOW_QUERY )
			{
				ITr2InteriorDynamic* dynamic = dynamic_cast<ITr2InteriorDynamic*>( interior );

				if( dynamic )
				{
					if( !m_lightGeneratingShadows->IsDynamicContributingToShadows( dynamic ) )
					{
						return;
					}
				}
				else
				{
					// Skip static objects when rendering to a shadow map for certain lights
					if( !m_displayStatics )
					{
						Tr2InteriorStatic* staticObject = dynamic_cast<Tr2InteriorStatic*>( interior );
						if( staticObject )
						{
							return;
						}
					}
				}
			}
			else
			{
				if( !m_displayDynamics )
				{
					ITr2InteriorDynamic* dynamic = dynamic_cast<ITr2InteriorDynamic*>( interior );
					if( dynamic )
					{
						return;
					}
				}
			}

			// Set the visibility flag, needed for determining cell visibility for
			// Enlighten updates
			interior->SetVisibility( true );

			// Set the number of visible lights
			interior->SetVisibleLightCount( m_activeLightSet.GetNumOfActiveLights() );
			interior->SetVisibleLightSet( m_activeLightSet );

			if( m_enableSHSolver )
			{
				interior->SetSHLightingSolver( &m_shSolver );
			}
			else
			{
				interior->SetSHLightingSolver( NULL );
			}

			// Set the mirror depth
			interior->SetMirrorDepth( m_mirrorDepth );

			// Get the per-object data for opaque batches
			Tr2PerObjectData* perObjectOpaque;

			if( gatherType == PREPASS_FORWARD_GATHER || gatherType == FULL_FORWARD_GATHER )
			{
				perObjectOpaque =
					interior->GetPerObjectDataWithPerInstanceLighting(
						m_activePrimaryRenderBatches ?
							m_activePrimaryRenderBatches : m_activeTransparentBatchStore,
						&m_activeLightSet,
						event.m_objectToWorldMatrix,
						m_mirrorToWorldMatrix
					);
			}
			else
			{
				perObjectOpaque =
					interior->GetPerObjectDataWithPerInstanceLighting(
						m_activePrimaryRenderBatches ?
							m_activePrimaryRenderBatches : m_activeTransparentBatchStore,
						NULL,
						event.m_objectToWorldMatrix,
						m_mirrorToWorldMatrix
					);
			}

			if( m_activePrimaryRenderBatches )
			{
				// Get the opaque batches
				m_activePrimaryRenderBatches->SetRenderingMode(
					Tr2EffectStateManager::RM_OPAQUE );

				// Shadow query
				if( m_visibilityQueryType == SHADOW_QUERY )
				{
					m_activePrimaryRenderBatches->SetUserData( ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_OPAQUE ) );
					renderable->GetBatches( m_activePrimaryRenderBatches,
						TRIBATCHTYPE_DEPTH,
						perObjectOpaque );
				}
				else
				{
					// Prepass gather
					if( gatherType == PREPASS_GATHER )
					{
						m_activePrimaryRenderBatches->SetUserData(
							ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_OPAQUE ) );
						renderable->GetBatches( m_activePrimaryRenderBatches,
							TRIBATCHTYPE_DEPTHNORMAL,
							perObjectOpaque );

						//	Decal normals only output to the Normal buffer, not the Depth buffer
						m_activePrimaryRenderBatches->SetRenderingMode(
							Tr2EffectStateManager::RM_DECAL_NO_DEPTH );
						m_activePrimaryRenderBatches->SetUserData(
							ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_DECAL ) );
						renderable->GetBatches( m_activePrimaryRenderBatches,
							TRIBATCHTYPE_DECALNORMAL,
							perObjectOpaque );
					}
					// Prepass forward gather
					else if( gatherType == PREPASS_FORWARD_GATHER )
					{
						// Get the opaque batches
						m_activePrimaryRenderBatches->SetUserData(
							ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_OPAQUE ) );
						renderable->GetBatches( m_activePrimaryRenderBatches,
							TRIBATCHTYPE_OPAQUE_PREPASS,
							perObjectOpaque );

						// Get the decal batches
						m_activePrimaryRenderBatches->SetRenderingMode(
							Tr2EffectStateManager::RM_DECAL );
						m_activePrimaryRenderBatches->SetUserData(
							ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_DECAL ) );
						renderable->GetBatches( m_activePrimaryRenderBatches,
							TRIBATCHTYPE_DECAL_PREPASS,
							perObjectOpaque );
					}
					else if( gatherType == FLARE_GATHER )
					{
						// Get the opaque batches
						if( !opaqueOnly )
						{
							m_activePrimaryRenderBatches->SetUserData(
								ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_OPAQUE ) );
							renderable->GetBatches( m_activePrimaryRenderBatches,
								TRIBATCHTYPE_FLARE,
								perObjectOpaque );
						}
					}
					// Full forward gather
					else
					{
						// Get the opaque batches
						m_activePrimaryRenderBatches->SetUserData(
							ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_OPAQUE ) );
						renderable->GetBatches( m_activePrimaryRenderBatches,
							TRIBATCHTYPE_OPAQUE,
							perObjectOpaque );

						// Get the decal batches
						m_activePrimaryRenderBatches->SetRenderingMode(
							Tr2EffectStateManager::RM_DECAL );
						m_activePrimaryRenderBatches->SetUserData(
							ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_DECAL ) );
						renderable->GetBatches( m_activePrimaryRenderBatches,
							TRIBATCHTYPE_DECAL,
							perObjectOpaque );
					}
				}

				// Now look at the attachments
				const ITr2RenderableVector* attachments = renderable->GetAttachedRenderables();
				if( attachments )
				{
					// Loop over the attached renderables
					for( ITr2RenderableVector::const_iterator it = attachments->begin();
						it != attachments->end(); ++it )
					{
						ITr2Renderable* attachment = *it;
						ITr2Interior* attachedInterior = dynamic_cast<ITr2Interior*>( attachment );

						// If the attached object is an interior, we can gather batches from it
						if( attachedInterior )
						{
							m_visibleObjects.push_back( attachment );

							// Get per-object data
							Tr2PerObjectData* perObjectOpaqueAttachment;
							if( gatherType == PREPASS_FORWARD_GATHER || gatherType == FULL_FORWARD_GATHER )
							{
								perObjectOpaqueAttachment =
									attachedInterior->GetPerObjectDataWithPerInstanceLighting(
									m_activePrimaryRenderBatches,
									&m_activeLightSet,
									event.m_objectToWorldMatrix,
									m_mirrorToWorldMatrix
									);
							}
							else
							{
								perObjectOpaqueAttachment =
									attachedInterior->GetPerObjectDataWithPerInstanceLighting(
									m_activePrimaryRenderBatches,
									NULL,
									event.m_objectToWorldMatrix,
									m_mirrorToWorldMatrix
									);
							}

							if( gatherType == PREPASS_GATHER )
							{
								m_activePrimaryRenderBatches->SetRenderingMode(
									Tr2EffectStateManager::RM_OPAQUE );
								m_activePrimaryRenderBatches->SetUserData(
									ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_OPAQUE ) );
								attachment->GetBatches( m_activePrimaryRenderBatches,
									TRIBATCHTYPE_DEPTHNORMAL,
									perObjectOpaqueAttachment );
							}
							else
							{
								// Set opaque render state
								m_activePrimaryRenderBatches->SetRenderingMode(
									Tr2EffectStateManager::RM_OPAQUE );

								// Shadow batches
								if( m_visibilityQueryType == SHADOW_QUERY )
								{
									m_activePrimaryRenderBatches->SetUserData( 0 );
									attachment->GetBatches( m_activePrimaryRenderBatches,
										TRIBATCHTYPE_DEPTH,
										perObjectOpaqueAttachment );
								}
								// Prepass forward batches
								else if( gatherType == PREPASS_FORWARD_GATHER )
								{
									// Get the opaque batches
									m_activePrimaryRenderBatches->SetUserData(
										ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_OPAQUE ) );
									attachment->GetBatches( m_activePrimaryRenderBatches,
										TRIBATCHTYPE_OPAQUE_PREPASS,
										perObjectOpaqueAttachment );

									// Get the decal batches
									m_activePrimaryRenderBatches->SetRenderingMode(
										Tr2EffectStateManager::RM_DECAL );
									m_activePrimaryRenderBatches->SetUserData(
										ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_DECAL ) );
									attachment->GetBatches( m_activePrimaryRenderBatches,
										TRIBATCHTYPE_DECAL_PREPASS,
										perObjectOpaqueAttachment );
								}
								// Full forward batches
								else
								{
									// Get the opaque batches
									m_activePrimaryRenderBatches->SetUserData(
										ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_OPAQUE ) );
									attachment->GetBatches( m_activePrimaryRenderBatches,
										TRIBATCHTYPE_OPAQUE,
										perObjectOpaqueAttachment );

									// Get the decal batches
									m_activePrimaryRenderBatches->SetRenderingMode(
										Tr2EffectStateManager::RM_DECAL );
									m_activePrimaryRenderBatches->SetUserData(
										ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_DECAL ) );
									attachment->GetBatches( m_activePrimaryRenderBatches,
										TRIBATCHTYPE_DECAL,
										perObjectOpaqueAttachment );
								}
							}
						}
					}
				}
			}

			// Gather transparent batches
			if( !opaqueOnly && m_activeTransparentBatchStore && m_visibilityQueryType != SHADOW_QUERY && gatherType != FLARE_GATHER )
			{
				if( renderable->HasTransparentBatches() )
				{
					m_activeTransparentBatchStore->SetRenderingMode(
						Tr2EffectStateManager::RM_ALPHA );
					m_activeTransparentBatchStore->SetUserData(
						ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_BLEND ) );
					renderable->GetBatches( m_activeTransparentBatchStore,
						TRIBATCHTYPE_TRANSPARENT,
						perObjectOpaque );
				}

				// Update batch count to indicate the range of transparent batches gathered
				// for this cell
				m_transparencyStack.back().second =
					(unsigned int)m_activeTransparentBatchStore->GetBatchCount();

				// Now look at the attachments
				const ITr2RenderableVector* attachments = renderable->GetAttachedRenderables();
				if( attachments )
				{
					// Loop over the attachments
					for( ITr2RenderableVector::const_iterator it = attachments->begin();
						it != attachments->end(); ++it )
					{
						ITr2Renderable* attachment = *it;
						ITr2Interior* attachedInterior = dynamic_cast<ITr2Interior*>( attachment );

						// If the attachment is an interior, we can gather batches from it
						if( attachedInterior )
						{
							// Get per-object data
							Tr2PerObjectData* perObjectOpaqueAttachment;
							if( gatherType == PREPASS_FORWARD_GATHER || gatherType == FULL_FORWARD_GATHER )
							{
								perObjectOpaqueAttachment =
									attachedInterior->GetPerObjectDataWithPerInstanceLighting(
									m_activeTransparentBatchStore,
									&m_activeLightSet,
									event.m_objectToWorldMatrix,
									m_mirrorToWorldMatrix
									);
							}
							else
							{
								perObjectOpaqueAttachment =
									attachedInterior->GetPerObjectDataWithPerInstanceLighting(
									m_activeTransparentBatchStore,
									NULL,
									event.m_objectToWorldMatrix,
									m_mirrorToWorldMatrix
									);
							}

							m_activeTransparentBatchStore->SetRenderingMode(
								Tr2EffectStateManager::RM_ALPHA );
							m_activeTransparentBatchStore->SetUserData(
								ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_BLEND ) );
							attachment->GetBatches( m_activeTransparentBatchStore,
								TRIBATCHTYPE_TRANSPARENT,
								perObjectOpaqueAttachment );
							// Update batch count to indicate the range of transparent batches
							// gathered for this cell
							m_transparencyStack.back().second =
								(unsigned int)m_activeTransparentBatchStore->GetBatchCount();
						}
					}
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function executes a region of influence active event in a normal batch
//   gathering operation.  It determines whether the light is mirrored and adds an
//   instanced light to the active light set (if mirrored) or a non-instanced light (if
//   not mirrored).  It also the tracks the total set of all visible lights in the scene,
//   but is not aware of instancing.
// Arguments:
//   event		- The region of influence active event to execute.
//   gatherType - The type of batches to gather.
// See Also:
//   OnRegionOfInfluenceActive, DoRegionOfInfluenceInactive
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoRegionOfInfluenceActive( const Tr2VisibilityEvent& event,
												  BatchGatherType gatherType )
{
	CCP_ASSERT( event.m_eventType == Tr2VisibilityEvent::REGION_OF_INFLUENCE_ACTIVE );
	CCP_ASSERT( event.m_userData != NULL );

	// Get a handle to the light source
	IRoot* iroot = event.m_userData;
	ITr2InteriorLight* lightSource = static_cast<ITr2InteriorLight*>( iroot );

	// Bail out if the user-data isn't a light source
	if( !lightSource )
	{
		return;
	}

	// If we're in a mirrored state, we need to get per-instance information for the light
	if( m_mirrorDepth > 0 )
	{
		// Add the instanced light to the active light set
		m_activeLightSet.AddInstancedLight( lightSource, Tr2Renderer::GetViewPosition(),
			m_mirrorToWorldMatrix );
	}
	else
	{
		// Add the non-instanced light to the active light set
		m_activeLightSet.AddLight( lightSource, Tr2Renderer::GetViewPosition() );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function executes a region of influence inactive event in a normal batch
//   gathering operation.  It removes the light from the active light set.
// Arguments:
//   event		- The region of influence inactive event to execute.
//   gatherType - The type of batches to gather.
// See Also:
//   OnRegionOfInfluenceInactive, DoRegionOfInfluenceActive
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoRegionOfInfluenceInactive( const Tr2VisibilityEvent& event,
												    BatchGatherType gatherType )
{
	CCP_ASSERT( event.m_eventType == Tr2VisibilityEvent::REGION_OF_INFLUENCE_INACTIVE );
	CCP_ASSERT( event.m_userData != NULL );

	// Get a handle to the light source
	IRoot* iroot = event.m_userData;
	ITr2InteriorLight* lightSource = static_cast<ITr2InteriorLight*>( iroot );

	// Bail out if the user-data isn't a light source
	if( !lightSource )
	{
		return;
	}

	// Remove the light from the active light set
	m_activeLightSet.RemoveLight( lightSource );
}

// --------------------------------------------------------------------------------------
// Description
//   This function executes a stencil mask event in a normal batch gathering operation.
//   If the visibility query is entering a mirror, it queues up a stencil mask batch
//   to wipe the depth and increment the stencil buffer for the mirrored pixels.  If the
//   query is exiting a mirror, it does not queue up a batch (because this is handled in
//   the DoPortalExit function).  Also, on mirror entry, this function queues up a
//   background cubemap batch to draw the background in a mirror.
// Arguments:
//   event - The stencil mask event to execute.
//   gatherType - The type of batches to gather.
// See Also:
//   OnStencilMask, DoPortalExit
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoStencilMask( const Tr2VisibilityEvent& event,
									  BatchGatherType gatherType )
{
	CCP_ASSERT( event.m_eventType == Tr2VisibilityEvent::STENCIL_MASK );

	// Early exit if there are no active primary batches
	if( !m_activePrimaryRenderBatches )
	{
		return;
	}

	if( event.m_mirrorIndex != -1 )
	{
		// Get a handle to the placeable
		IRoot* iroot = event.m_userData;
		Tr2InteriorPlaceable* placeable = dynamic_cast<Tr2InteriorPlaceable*>( iroot );

		if( placeable )
		{
			Tr2InteriorMirror* mirror = placeable->GetMirror( event.m_mirrorIndex );

			if( mirror )
			{
				if( event.m_stencilWrite > event.m_stencilTest )
				{
					m_stencilStack.push_back( std::make_pair( event.m_stencilWrite,
						event.m_stencilTest ) );

					if( gatherType == PREPASS_GATHER || gatherType == FULL_FORWARD_GATHER )
					{
						// Get the per-object data for the opaque batches
						Tr2PerObjectData* perObjectOpaque =
							placeable->GetPerObjectDataWithPerInstanceLighting(
								m_activePrimaryRenderBatches,
								&m_activeLightSet,
								event.m_objectToWorldMatrix,
								m_mirrorToWorldMatrix
							);

						m_activePrimaryRenderBatches->SetRenderingMode(
							Tr2EffectStateManager::RM_ANY );
						m_activePrimaryRenderBatches->SetUserData(
							ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_BEGIN ) );

						// Setup stencil batch parameters
						WodStencilBatchParams params =
						{
							mirror->GetMeshIndex(),
							mirror->GetAreaIndex(),
							event.m_stencilWrite,
							event.m_stencilTest,
							D3DSTENCILOP_INCR,
							true, // This flag indicates that we're going into a mirror,
								 // so we need to clear depth
							gatherType == FULL_FORWARD_GATHER // We render mirror material if in forward pass
						};

						// Set the stencil parameters on the placeable
						placeable->SetStencilParameters( params );
						if( gatherType == FULL_FORWARD_GATHER )
						{
							m_activePrimaryRenderBatches->SetRenderingMode( Tr2EffectStateManager::RM_ALPHA );
						}
						// Get the mirror batches
						placeable->GetBatches( m_activePrimaryRenderBatches,
							                   TRIBATCHTYPE_MIRROR,
											   perObjectOpaque );
					}
					else if( gatherType == LIGHT_GATHER || gatherType == PREPASS_FORWARD_GATHER  )
					{
						Tr2InteriorStencilMaskBatch* stencilBatch =
							m_activePrimaryRenderBatches->Allocate<Tr2InteriorStencilMaskBatch>();

						if( stencilBatch )
						{
							stencilBatch->SetShaderMaterial( NULL );
							stencilBatch->SetPerObjectData( NULL );
							stencilBatch->SetGeometryResource( NULL );
							stencilBatch->SetMeshParameters( 0, 0, 0 );

							// This flag disables the stencil test
							stencilBatch->SetDisableStencil( false );
							stencilBatch->SetStencilTest( event.m_stencilWrite );

							stencilBatch->SetRenderingMode( Tr2EffectStateManager::RM_ANY );
							m_activePrimaryRenderBatches->SetUserData(
								ConstructKey( m_currentObjectGroup, WODINTBATCHGROUP_BEGIN ) );

							m_activePrimaryRenderBatches->Commit( stencilBatch );
						}
					}

					// If we're gathering batches for the forward pass, get a cubemap
					// batch for the background
					if( gatherType == PREPASS_FORWARD_GATHER || gatherType == FULL_FORWARD_GATHER )
					{
						PrepareBackgroundCubemapBatch( m_activePrimaryRenderBatches );
					}
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function gathers pre-pass batches.  It ignores lights, and in some cases calls
//   special pre-pass event handlers to process events while ignoring transparency.
// Arguments:
//   results - The results set object that supplies visibility events
// See Also:
//   GatherLightBatches, GatherForwardBatches
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::GatherPrePassBatches( Tr2VisibilityResults* results )
{
	if( results )
	{
		const std::vector<Tr2VisibilityEvent>& events = results->GetEvents();

		for( std::vector<Tr2VisibilityEvent>::const_iterator it = events.begin();
			it != events.end(); ++it )
		{
			const Tr2VisibilityEvent& event = *it;

			switch( event.m_eventType )
			{
			case Tr2VisibilityEvent::QUERY_BEGIN:
				DoQueryBegin( event, PREPASS_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_ENTER:
				DoPortalEnter( event, PREPASS_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_EXIT:
				DoPortalExit( event, PREPASS_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_PRE_EXIT:
				DoPortalPreExit( event, PREPASS_GATHER );
				break;
			case Tr2VisibilityEvent::VIEW_PARAMETERS_CHANGED:
				DoViewParametersChanged( event, PREPASS_GATHER );
				break;
			case Tr2VisibilityEvent::INSTANCE_VISIBLE:
				DoInstanceVisible( event, PREPASS_GATHER );
				break;
			case Tr2VisibilityEvent::STENCIL_MASK:
				DoStencilMask( event, PREPASS_GATHER );
				break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function gathers light batches.  It ignores everything except lights.
// Arguments:
//   results - The results set object that supplies visibility events
//   batches - The accumulator for light batches.
// See Also:
//   GatherPrepassBatches, GatherForwardBatches
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::GatherLightBatches( Tr2VisibilityResults* results )
{
	if( results )
	{
		const std::vector<Tr2VisibilityEvent>& events = results->GetEvents();

		for( std::vector<Tr2VisibilityEvent>::const_iterator it = events.begin();
			it != events.end(); ++it )
		{
			const Tr2VisibilityEvent& event = *it;

			switch( event.m_eventType )
			{
			case Tr2VisibilityEvent::QUERY_BEGIN:
				DoQueryBegin( event, LIGHT_GATHER );
				break;
			case Tr2VisibilityEvent::QUERY_END:
				DoQueryEnd( event, LIGHT_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_ENTER:
				DoPortalEnter( event, LIGHT_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_EXIT:
				DoPortalExit( event, LIGHT_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_PRE_EXIT:
				DoPortalPreExit( event, LIGHT_GATHER );
				break;
			case Tr2VisibilityEvent::VIEW_PARAMETERS_CHANGED:
				DoViewParametersChanged( event, LIGHT_GATHER );
				break;
			case Tr2VisibilityEvent::INSTANCE_VISIBLE:
				DoInstanceVisible( event, LIGHT_GATHER );
				break;
			case Tr2VisibilityEvent::STENCIL_MASK:
				DoStencilMask( event, LIGHT_GATHER );
				break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function gathers forward batches for prepass lighting.  It processes the full
//   set of visibility events, since they are all relevant to the forward pass.
// Arguments:
//   results - The results set object that supplies visibility events
// See Also:
//   GatherPrepassBatches, GatherLightBatches, GatherFullForwardBatches
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::GatherPrepassForwardBatches( Tr2VisibilityResults* results )
{
	if( results )
	{
		const std::vector<Tr2VisibilityEvent>& events = results->GetEvents();

		for( std::vector<Tr2VisibilityEvent>::const_iterator it = events.begin();
			it != events.end(); ++it )
		{
			const Tr2VisibilityEvent& event = *it;

			switch( event.m_eventType )
			{
			case Tr2VisibilityEvent::QUERY_BEGIN:
				DoQueryBegin( event, PREPASS_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::QUERY_END:
				DoQueryEnd( event, PREPASS_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_ENTER:
				DoPortalEnter( event, PREPASS_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_EXIT:
				DoPortalExit( event, PREPASS_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_PRE_EXIT:
				DoPortalPreExit( event, PREPASS_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::VIEW_PARAMETERS_CHANGED:
				DoViewParametersChanged( event, PREPASS_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::INSTANCE_VISIBLE:
				DoInstanceVisible( event, PREPASS_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::REGION_OF_INFLUENCE_ACTIVE:
				DoRegionOfInfluenceActive( event, PREPASS_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::REGION_OF_INFLUENCE_INACTIVE:
				DoRegionOfInfluenceInactive( event, PREPASS_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::STENCIL_MASK:
				DoStencilMask( event, PREPASS_FORWARD_GATHER );
				break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function gathers forward batches for a full forward lighting render.  It
//   processes the full set of visibility events, since they are all relevant to the
//   forward pass.
// Arguments:
//   results - The results set object that supplies visibility events
// See Also:
//   GatherPrepassBatches, GatherLightBatches, GatherPrepassForwardBatches
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::GatherFullForwardBatches( Tr2VisibilityResults* results )
{
	if( results )
	{
		const std::vector<Tr2VisibilityEvent>& events = results->GetEvents();

		for( std::vector<Tr2VisibilityEvent>::const_iterator it = events.begin();
			it != events.end(); ++it )
		{
			const Tr2VisibilityEvent& event = *it;

			switch( event.m_eventType )
			{
			case Tr2VisibilityEvent::QUERY_BEGIN:
				DoQueryBegin( event, FULL_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::QUERY_END:
				DoQueryEnd( event, FULL_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_ENTER:
				DoPortalEnter( event, FULL_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_EXIT:
				DoPortalExit( event, FULL_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_PRE_EXIT:
				DoPortalPreExit( event, FULL_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::VIEW_PARAMETERS_CHANGED:
				DoViewParametersChanged( event, FULL_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::INSTANCE_VISIBLE:
				DoInstanceVisible( event, FULL_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::REGION_OF_INFLUENCE_ACTIVE:
				DoRegionOfInfluenceActive( event, FULL_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::REGION_OF_INFLUENCE_INACTIVE:
				DoRegionOfInfluenceInactive( event, FULL_FORWARD_GATHER );
				break;
			case Tr2VisibilityEvent::STENCIL_MASK:
				DoStencilMask( event, FULL_FORWARD_GATHER );
				break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function gathers light batches for flare pass.  It ignores everything except
//   lights.
// Arguments:
//   results - The results set object that supplies visibility events
//   batches - The accumulator for light batches.
// See Also:
//   GatherLightBatches, GatherPrepassBatches, GatherForwardBatches
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::GatherFlareBatches( Tr2VisibilityResults* results )
{
	if( results )
	{
		const std::vector<Tr2VisibilityEvent>& events = results->GetEvents();

		for( std::vector<Tr2VisibilityEvent>::const_iterator it = events.begin();
			it != events.end(); ++it )
		{
			const Tr2VisibilityEvent& event = *it;

			switch( event.m_eventType )
			{
			case Tr2VisibilityEvent::QUERY_BEGIN:
				DoQueryBegin( event, FLARE_GATHER );
				break;
			case Tr2VisibilityEvent::QUERY_END:
				DoQueryEnd( event, FLARE_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_ENTER:
				DoPortalEnter( event, FLARE_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_EXIT:
				DoPortalExit( event, FLARE_GATHER );
				break;
			case Tr2VisibilityEvent::PORTAL_PRE_EXIT:
				DoPortalPreExit( event, FLARE_GATHER );
				break;
			case Tr2VisibilityEvent::VIEW_PARAMETERS_CHANGED:
				DoViewParametersChanged( event, FLARE_GATHER );
				break;
			case Tr2VisibilityEvent::INSTANCE_VISIBLE:
				DoInstanceVisible( event, FLARE_GATHER );
				break;
			case Tr2VisibilityEvent::STENCIL_MASK:
				DoStencilMask( event, FLARE_GATHER );
				break;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::CreateRootCell( void )
{
	// Create the root cell
	m_rootCell = Umbra::Cell::create();

	// Create the object and dummy-sphere model
	m_rootModel = ( Umbra::Model* )Umbra::SphereModel::create( AS_UMBRA_VECTOR3( Vector3( 0.0f, 0.0f, 0.0f ) ), 1.0f );
	m_rootObject = Umbra::Object::create( m_rootModel );

	// Set the object to 'UNBOUNDED' (this means the object is always visible in Umbra)
	m_rootObject->set( Umbra::Object::UNBOUNDED, true );
	m_rootObject->setCell( m_rootCell );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::ReleaseRootCell( void )
{
	if( m_rootCell == NULL )
	{
		return;
	}

	// Clean-up the root cell
	if( m_rootCell )
	{
		m_rootCell->release();
		m_rootCell = NULL;
	}

	// Clean-up the root model
	if( m_rootModel )
	{
		m_rootModel->release();
		m_rootModel = NULL;
	}

	// Clean-up the root object
	if( m_rootObject )
	{
		m_rootObject->release();
		m_rootObject = NULL;
	}

	// Mark all dynamics as dirty to remove those dynamics
	// that were added to the root cell
	for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
		( *it )->SetDirtyFlag( true );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::UpdateRootPortals( void )
{
	// Create portals for all cells
	for( PTr2InteriorCellVector::iterator cellIt = m_cells.begin(); cellIt != m_cells.end(); ++cellIt )
	{
		Vector3 minBounds, maxBounds;
		if( ( *cellIt )->GetBoundingBox( minBounds, maxBounds ) )
		{
			Umbra::Cell* ucell = ( *cellIt )->GetUmbraCell();

			std::map< Umbra::Cell*, std::pair<Umbra::Model*, Umbra::PhysicalPortal*> >::iterator it =
				m_rootPortals.find( ucell );
			if( it == m_rootPortals.end() )
			{
				Umbra::Model* model = ( Umbra::Model* )Umbra::OBBModel::create( AS_UMBRA_VECTOR3( minBounds ), AS_UMBRA_VECTOR3( maxBounds ) );
				model->set( Umbra::Model::BACKFACE_CULLABLE, true );

				Umbra::PhysicalPortal* portal = Umbra::PhysicalPortal::create( model, ucell );
				portal->setCell( m_rootCell );
				portal->setObjectToCellMatrix( AS_UMBRA_MATRIX( ( *cellIt )->GetWorldTransform() ) );
				portal->set( Umbra::Object::INFORM_PORTAL_ENTER, true );
				portal->set( Umbra::Object::INFORM_PORTAL_EXIT, true );
				portal->set( Umbra::Object::INFORM_PORTAL_PRE_EXIT, true );

				m_rootPortals[ucell] = std::make_pair( model, portal );
			}
		}
	}

	// Remove portals for deleted cells
	for( std::map<Umbra::Cell*, std::pair<Umbra::Model*, Umbra::PhysicalPortal*> >::iterator it = m_rootPortals.begin();
		it != m_rootPortals.end(); )
	{
		Umbra::Cell* ucell = it->first;

		bool found = false;
		for( PTr2InteriorCellVector::iterator cellIt = m_cells.begin(); cellIt != m_cells.end(); ++cellIt )
		{
			if( ucell == ( *cellIt )->GetUmbraCell() )
			{
				found = true;
				break;
			}
		}

		if( found )
		{
			++it;
		}
		else
		{
			Umbra::Model* model = it->second.first;
			if( model )
			{
				model->release();
				model = NULL;
			}

			Umbra::PhysicalPortal* portal = it->second.second;
			if( portal )
			{
				portal->release();
				portal = NULL;
			}

			it = m_rootPortals.erase( it );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates the root portal for a particular cell.  A log error message is printed if
//   the cell is NULL.
// Arguments:
//   cell - The cell to update (should not be NULL)
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::UpdateRootPortalForCell( Tr2InteriorCell* cell )
{
	// Bail out early if the cell is NULL
	if( !cell )
	{
		CCP_LOGERR( "Attempt to update root portal for NULL cell in interior scene!" );
		return;
	}

	Vector3 minBounds, maxBounds;
	if( cell->GetBoundingBox( minBounds, maxBounds ) )
	{
		Umbra::Cell* ucell = cell->GetUmbraCell();

		std::map< Umbra::Cell*, std::pair<Umbra::Model*, Umbra::PhysicalPortal*> >::iterator it =
			m_rootPortals.find( ucell );
		if( it == m_rootPortals.end() )
		{
			Umbra::Model* model = ( Umbra::Model* )Umbra::OBBModel::create(
				AS_UMBRA_VECTOR3( minBounds ), AS_UMBRA_VECTOR3( maxBounds ) );
			model->set( Umbra::Model::BACKFACE_CULLABLE, true );

			Umbra::PhysicalPortal* portal = Umbra::PhysicalPortal::create( model, ucell );
			portal->setCell( m_rootCell );
			portal->setObjectToCellMatrix( AS_UMBRA_MATRIX( cell->GetWorldTransform() ) );
			portal->set( Umbra::Object::INFORM_PORTAL_ENTER, true );
			portal->set( Umbra::Object::INFORM_PORTAL_EXIT, true );
			portal->set( Umbra::Object::INFORM_PORTAL_PRE_EXIT, true );

			m_rootPortals[ucell] = std::make_pair( model, portal );
		}
		else
		{
			Umbra::Model* model = ( *it ).second.first;
			if( model )
			{
				model->release();
			}

			Umbra::PhysicalPortal* portal = ( *it ).second.second;
			if( portal )
			{
				portal->release();
			}

			model = ( Umbra::Model* )Umbra::OBBModel::create(
				AS_UMBRA_VECTOR3( minBounds ), AS_UMBRA_VECTOR3( maxBounds ) );
			model->set( Umbra::Model::BACKFACE_CULLABLE, true );

			portal = Umbra::PhysicalPortal::create( model, ucell );
			portal->setCell( m_rootCell );
			portal->setObjectToCellMatrix( AS_UMBRA_MATRIX( cell->GetWorldTransform() ) );
			portal->set( Umbra::Object::INFORM_PORTAL_ENTER, true );
			portal->set( Umbra::Object::INFORM_PORTAL_EXIT, true );
			portal->set( Umbra::Object::INFORM_PORTAL_PRE_EXIT, true );

			( *it ).second.first = model;
			( *it ).second.second = portal;
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::ReleaseRootPortals( void )
{
	for( std::map<Umbra::Cell*, std::pair<Umbra::Model*, Umbra::PhysicalPortal*> >::iterator it = m_rootPortals.begin();
		it != m_rootPortals.end(); ++it )
	{
		Umbra::Model* model = it->second.first;
		if( model )
	{
			model->release();
			model = NULL;
	}

		Umbra::PhysicalPortal* portal = it->second.second;
		if( portal )
	{
			portal->release();
			portal = NULL;
	}
}

	m_rootPortals.clear();
}

// ------------------------------------------------------------------------------------------------------
Umbra::Cell* Tr2InteriorScene::GetCameraCell( void )
{
	if( g_useRootCell && m_sceneUseRootCell )
	{
		m_currentCameraCell = m_rootCell;
		return m_currentCameraCell;
	}

	// Get the camera position
	Vector3 cameraPos = Tr2Renderer::GetViewPosition();

	// Temporary list of candidate cells
	std::vector<Umbra::Cell*> candidateCells;
	bool cameraInCurrentCell = false;

	// Check if the camera position is inside any of the cells
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		Tr2InteriorCell* cell = ( *it );

		// If the camera position is inside the bounding box of the cell, return the Umbra cell
		if( cell->ContainsPoint( cameraPos ) )
		{
			Umbra::Cell* candidateCell = ( *it )->GetUmbraCell();
			if( candidateCell == m_currentCameraCell )
			{
				cameraInCurrentCell = true;
			}
			candidateCells.push_back( ( *it )->GetUmbraCell() );
		}
	}

	if( m_cameraPortalModel )
	{
		m_cameraPortalModel->release();
		m_cameraPortalModel = NULL;
	}
	for( std::vector<Umbra::PhysicalPortal*>::iterator it = m_cameraPortals.begin(); it != m_cameraPortals.end(); ++it )
	{
		( *it )->release();
	}
	m_cameraPortals.clear();
	CCP_STATS_RESET( wodInteriorSceneIntersectingCellPortals );

	if( candidateCells.size() > 1 )
	{
		Umbra::Cell* cameraCell;
		if( cameraInCurrentCell )
		{
			cameraCell = m_currentCameraCell;
		}
		else
		{
			cameraCell = candidateCells[0];
		}

		Vector3 minBounds( cameraPos.x - 0.1f, cameraPos.y - 0.1f, cameraPos.z - 0.1f );
		Vector3 maxBounds( cameraPos.x + 0.1f, cameraPos.y + 0.1f, cameraPos.z + 0.1f );
		m_cameraPortalModel = ( Umbra::Model* )Umbra::OBBModel::create( AS_UMBRA_VECTOR3( minBounds ), AS_UMBRA_VECTOR3( maxBounds ) );

		for( std::vector<Umbra::Cell*>::iterator it = candidateCells.begin(); it != candidateCells.end(); ++it )
		{
			if( *it != cameraCell )
			{
				Umbra::PhysicalPortal* portal = Umbra::PhysicalPortal::create( m_cameraPortalModel, *it );
				portal->setCell( cameraCell );
				Umbra::Matrix4x4 objectToCell;
				cameraCell->getWorldToCellMatrix( objectToCell );
				portal->setObjectToCellMatrix( objectToCell );
				portal->set( Umbra::Object::INFORM_PORTAL_ENTER, true );
				portal->set( Umbra::Object::INFORM_PORTAL_EXIT, true );
				portal->set( Umbra::Object::INFORM_PORTAL_PRE_EXIT, true );
				m_cameraPortals.push_back( portal );

				CCP_STATS_INC( wodInteriorSceneIntersectingCellPortals );
			}
		}
	}

	// If the camera is still in the current cell, just return that
	if( cameraInCurrentCell )
	{
		return m_currentCameraCell;
	}

	// If we got at least one candidate cell
	if( !candidateCells.empty() )
	{
		m_currentCameraCell = candidateCells[0];
		return m_currentCameraCell;
	}

	// If we made it here, then the camera is not inside any of the Tr2InteriorCells
	if( g_useRootCell && m_sceneUseRootCell )
	{
		m_currentCameraCell = m_rootCell;
	}
	else if( !m_cells.empty() )
	{
		// Find the closest cell
		Tr2InteriorCell* closestCell = NULL;
		float closestDistance;
		for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
		{
			Tr2InteriorCell* cell = ( *it );

			Matrix transformInv;
			D3DXMatrixInverse( &transformInv, NULL, &cell->GetWorldTransform() );
			Vector3 localPos;
			D3DXVec3TransformCoord( &localPos, &cameraPos, &transformInv );

			Vector3 min, max;
			cell->GetBoundingBox( min, max );

			Vector3 closest;
			closest.x = ( localPos.x < min.x ) ? min.x : ( localPos.x > max.x) ? max.x : localPos.x;
			closest.y = ( localPos.y < min.y ) ? min.y : ( localPos.y > max.y) ? max.y : localPos.y;
			closest.z = ( localPos.z < min.z ) ? min.z : ( localPos.z > max.z) ? max.z : localPos.z;

			closest -= localPos;

			float distance = D3DXVec3LengthSq( &closest );

			if( closestCell == NULL || distance < closestDistance )
			{
				closestCell = cell;
				closestDistance = distance;
			}
		}
		if( closestCell )
		{
			m_currentCameraCell = closestCell->GetUmbraCell();
		}
		else
		{
			m_currentCameraCell = NULL;
		}
	}
	else
	{
		m_currentCameraCell = NULL;
	}

	return m_currentCameraCell;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::UpdateLights( void )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	// Visit each light
	for( PITr2InteriorLightVector::iterator lit = m_lights.begin(); lit != m_lights.end(); ++lit )
	{
		// Check the dirty flag & update
		if( ( *lit )->IsDirty() )
		{
			// Are the cells fully loaded yet?
			bool cellsReady = true;
			// Test intersection with each cell
			for( Tr2InteriorCellVector::iterator cit = m_cells.begin(); cit != m_cells.end(); ++cit )
			{
				// If the bounding box is ready, do the intersection test
				if( ( *cit )->IsUnbounded() || ( *cit )->IsBoundingBoxReady() )
				{
					( *lit )->TestCellIntersectionAndAdd( *cit );
				}
				// Otherwise bail out
				else
				{
					cellsReady = false;
					break;
				}
			}

			// Clear the dirty flag on the light
			if( cellsReady )
			{
				( *lit )->SetDirtyFlag( false );
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::UpdateDynamics( void )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	// Create the root cell, if necessary
	if( ( g_useRootCell && m_sceneUseRootCell ) && !m_rootCell )
	{
		CreateRootCell();
	}

	for( PITr2InteriorDynamicVector::iterator dit = m_dynamics.begin(); dit != m_dynamics.end(); ++dit )
	{
		if( ( *dit )->IsDirty() && ( *dit )->IsBoundingBoxReady() && ( *dit )->IsUmbraReady() )
		{
			bool cellsReady = true;
			bool added = false;
			// Test intersection with each cell
			for( Tr2InteriorCellVector::iterator cit = m_cells.begin(); cit != m_cells.end(); ++cit )
			{
				// If the bounding box is ready, do the intersection test
				if( ( *cit )->IsUnbounded() || ( *cit )->IsBoundingBoxReady() )
				{
					if( ( *dit )->TestCellIntersectionAndAdd( *cit ) )
					{
						( *cit )->MarkShadowsDirtyForDynamic( *dit );
						added = true;
					}
				}
				// Otherwise bail out
				else
				{
					cellsReady = false;
					break;
				}
			}

			if( !added && m_rootCell != NULL )
			{
				( *dit )->AddToRootCell( m_rootCell );
			}

			// Clear the dirty flag on the dynamic
			if( cellsReady )
			{
				( *dit )->ClearDirty();
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorScene::IsMirrored( void ) const
{
	return( Tr2Renderer::IsRightHanded() && !m_isMirroredInLeftHandedSpace ) || ( !Tr2Renderer::IsRightHanded() && m_isMirroredInLeftHandedSpace );
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorScene::OnLightsListModified( long event, SSIZE_T key, SSIZE_T key2, IRoot* currvalue )
{
	if( ( event & BELIST_LOADING ) == 0  )
	{
		// Respond to an item removal event
		if( ( event & BELIST_EVENTMASK ) == BELIST_REMOVED )
		{
			if( currvalue )
			{
				// See if the removed item is a light
				ITr2InteriorLight* light = NULL;
				if( currvalue->QueryInterface( BlueInterfaceIID<ITr2InteriorLight>(), ( void** )&light ) )
				{
					// Now, if the light pointer is valid, we need to remove this light from all the cells
					for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
					{
						( *it )->RemoveLight( light );
					}

					// Remove the light from the scene
					light->RemoveFromScene();

					// QueryInterface Locks, so manually Unlock
					light->Unlock();
				}
			}
		}
		// Respond to an item insertion event
		else if( ( event & BELIST_EVENTMASK ) == BELIST_INSERTED )
		{
			if( currvalue )
			{
				// See if the inserted item is a light source
				ITr2InteriorLight* light = NULL;
				if( currvalue->QueryInterface( BlueInterfaceIID<ITr2InteriorLight>(), ( void** )&light ) )
				{
					light->EnableROI( m_enableROIs );
					light->AddToScene();

					// Need to unlock, since QueryInterface Locks
					light->Unlock();
				}
			}
		}
		// Respond to a list-cleared event
		else if( ( event & BELIST_EVENTMASK ) == BELIST_UNLOADSTART )
		{
			// Now, loop over all the lights and remove them from all the cells
			for( PITr2InteriorLightVector::iterator lit = m_lights.begin(); lit != m_lights.end(); ++lit )
			{
				for( PTr2InteriorCellVector::iterator cit = m_cells.begin(); cit != m_cells.end(); ++cit )
				{
					( *cit )->RemoveLight( *lit );
				}
			}
		}
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorScene::OnDynamicsListModified( long event, SSIZE_T key, SSIZE_T key2, IRoot* currvalue )
{
	if( ( event & BELIST_LOADING ) == 0  )
	{
		// Respond to an item removal event
		if( ( event & BELIST_EVENTMASK ) == BELIST_REMOVED )
		{
			if( currvalue )
			{
				// See if the removed item is a dynamic
				ITr2InteriorDynamic* dynamic = NULL;
				if( currvalue->QueryInterface( BlueInterfaceIID<ITr2InteriorDynamic>(), ( void** )&dynamic ) )
				{
					// Now, if the dynamic pointer is valid, we need to remove this dynamic from all the cells
					for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
					{
						( *it )->RemoveDynamic( dynamic );
					}

					// Remove from the scene
					dynamic->RemoveFromScene();

					// See if the dynamic is in the pending-load list
					ssize_t pos = m_dynamicsPendingLoad.FindKey( dynamic );
					if( pos != -1 )
					{
						m_dynamicsPendingLoad.Remove( pos );
					}

					// Need to unlock, since QueryInterface Locks
					dynamic->Unlock();
				}
			}
		}
		// Respond to an item insertion event
		else if( ( event & BELIST_EVENTMASK ) == BELIST_INSERTED )
		{
			if( currvalue )
			{
				// See if the inserted item is a dynamic
				ITr2InteriorDynamic* dynamic = NULL;
				if( currvalue->QueryInterface( BlueInterfaceIID<ITr2InteriorDynamic>(), ( void** )&dynamic ) )
				{
					if( !dynamic->AddToScene( m_apexScene ) && ( m_dynamicsPendingLoad.FindKey( dynamic ) == -1 ) )
					{
						m_dynamicsPendingLoad.Insert( -1, dynamic );
					}

					// Need to unlock, since QueryInterface Locks
					dynamic->Unlock();
				}
			}
		}
		// Respond to a list-cleared event
		else if( ( event & BELIST_EVENTMASK ) == BELIST_UNLOADSTART )
		{
			// Loop over all the dynamics and remove them from all the cells
			for( PITr2InteriorDynamicVector::iterator dit = m_dynamics.begin(); dit != m_dynamics.end(); ++dit )
			{
				for( PTr2InteriorCellVector::iterator cit = m_cells.begin(); cit != m_cells.end(); ++cit )
				{
					( *cit )->RemoveDynamic( *dit );
				}

				( *dit )->RemoveFromScene();
			}
		}
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::SetupTransformsForPicking( float fx, float fy, TriProjection* proj, TriView* view, TriViewport* viewport )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	Tr2Renderer::SetViewTransform( view->GetTransform() );
	proj->SetProjection();

	if( Tr2Renderer::GetCurrentProjectionType() == PT_ORTHOGONAL )
	{
		fx *= (Tr2Renderer::GetOrthoWidth()/2.0f);
		fy *= (Tr2Renderer::GetOrthoHeight()/2.0f);
		float metersPerPixel = (Tr2Renderer::GetOrthoWidth()/viewport->width)/2.0f;
		Tr2Renderer::SetOrthoProjection(fx-metersPerPixel,
			fx+metersPerPixel,
			fy-metersPerPixel,
			fy+metersPerPixel,
			Tr2Renderer::GetFrontClip(),
			Tr2Renderer::GetBackClip());
	}
	else
	{
		//
		// Projection is set up to scale the image such that the viewport is covered by one pixel.
		Vector2 scaling( float(viewport->width), float(viewport->height) );
		// translate the projection so that we center around the pick ray origin,
		// while remembering to scale this value as well:
		Vector2 translation;
		translation.x = -fx*scaling.x;
		translation.y = -fy*scaling.y;
		Tr2Renderer::AdjustProjection( scaling, translation );
	}

	Tr2PerFramePSData perFramePS;
	Tr2PerFrameVSData perFrameVS;
	PopulatePerFramePSData( perFramePS );
    PopulatePerFrameVSData( perFrameVS );

	FillAndSetConstants( m_perFrameVSBuffer, perFramePS, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
	FillAndSetConstants( m_perFramePSBuffer, perFrameVS, PIXEL_SHADER , Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
}

const std::vector<ITr2Renderable*>& Tr2InteriorScene::GetPickingObjectsToRender( const Vector3& dirWorld )
{
	if( m_visibleObjects.empty() )
	{
		return GetPickingObjectsToRender(
			dirWorld, Tr2Renderer::GetFieldOfView(), Tr2Renderer::GetAspectRatio() );
	}

	return m_visibleObjects;
}

const std::vector<ITr2Renderable*>& Tr2InteriorScene::GetPickingObjectsToRender( const Vector3& dirWorld, float fov, float aspect )
{
	// Setup the frustum
	TriFrustum frustum;
	frustum.m_zNear = Tr2Renderer::GetFrontClip();
	frustum.m_zFar = Tr2Renderer::GetBackClip();
	frustum.m_aspectRatio = aspect;
	frustum.m_fov = fov;

	// Setup view-inverse matrix
	const Vector3 eye = Tr2Renderer::GetViewPosition();
	XMVECTOR det;
	Matrix newViewInv(
		XMMatrixInverse( &det, XMMatrixLookAtLH( eye, ( eye - dirWorld ), Vector3( 0.0f, 1.0f, 0.0f ) ) )
		);

	// Set camera parameters
	unsigned int oldProperties = m_sceneCamera.GetProperties();
	m_sceneCamera.SetProperties( m_sceneCamera.GetWidth(), m_sceneCamera.GetHeight(),
		Umbra::Camera::VIEWFRUSTUM_CULLING );
	m_sceneCamera.SetViewParameters( newViewInv, &frustum );

	// Create the visibility stack
	auto visitedObject = new std::unordered_set<IRoot*>;
	m_visitedObjects.push_back( visitedObject );

	// Disable Mirrors for picking
	for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
		Tr2InteriorPlaceable* placeable = dynamic_cast<Tr2InteriorPlaceable*>( *it );
		if( placeable )
		{
			placeable->EnableMirrorPortals( false );
		}
	}

	// Issue the query
	m_visibleObjects.clear();
	m_visibilityQueryType = PICKING_QUERY;
	m_sceneCamera.ResolveVisibility( this, GetCameraCell(), 1 );

	// Renable mirrors
	for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
		Tr2InteriorPlaceable* placeable = dynamic_cast<Tr2InteriorPlaceable*>( *it );
		if( placeable )
		{
			placeable->EnableMirrorPortals( true );
		}
	}

	// Make sure that the objects allocated for the visibility stack are removed
	for( auto i = m_visitedObjects.begin(); i != m_visitedObjects.end(); ++i )
	{
		delete *i;
	}

	// Clear the stack
	m_visitedObjects.clear();

	// Restore old camera properties
	m_sceneCamera.SetProperties( m_sceneCamera.GetWidth(), m_sceneCamera.GetHeight(),
		oldProperties );

	return m_visibleObjects;
}

void Tr2InteriorScene::SetPerFrameDataForPicking( void )
{
	Tr2PerFramePSData perFramePS;
	Tr2PerFrameVSData perFrameVS;

	PopulatePerFramePSData( perFramePS );
	PopulatePerFrameVSData( perFrameVS );

	USE_MAIN_THREAD_RENDER_CONTEXT();	//TODO
    Tr2BindPerFramePSData( perFramePS, renderContext );
    Tr2BindPerFrameVSData( perFrameVS, renderContext );
}

__int64 Tr2InteriorScene::ConstructKey( unsigned int objectGroup, Tr2InteriorBatchGroup batchGroup )
{
	__int64 objGroup64 = ( __int64 )objectGroup << 48;

	__int64 batchGroup64 = ( __int64 )batchGroup << 32;

	return( objGroup64 | batchGroup64 );
}

void Tr2InteriorScene::PopulatePerFrameVSData( Tr2PerFrameVSData &data )
{
    Tr2PopulatePerFrameVSDataTransformations( data );

	// sun
	Vector3 vec;
	GetSunDirWorldHandle()->GetValue( vec );
	data.sunDirWorld.x = vec.x;
	data.sunDirWorld.y = vec.y;
	data.sunDirWorld.z = vec.z;
}

void Tr2InteriorScene::PopulatePerFramePSData( Tr2PerFramePSData &data )
{
    Tr2PopulatePerFramePSDataTransformations( data );

    // sun
	GetSunDiffuseColorHandle()->GetValue( data.sunDiffuseColor );
	GetSunSpecularColorHandle()->GetValue( data.sunSpecularColor );
	GetAmbientColorHandle()->GetValue( data.sceneAmbientColor );

	Vector3 vec;
	GetSunDirWorldHandle()->GetValue( vec );
	data.sunDirWorld.x = vec.x;
	data.sunDirWorld.y = vec.y;
	data.sunDirWorld.z = vec.z;
	data.cullDirection = 1.0f;
	data.shScale = m_shScale;

	data.sceneFogColor = m_fogColor;
	data.maxFogAmount = m_maxFogAmount;
	data.maxFogDistance = m_maxFogDistance;
	data.minFogDistance = m_minFogDistance;
}

ITriRenderBatchAccumulator* Tr2InteriorScene::GetOpaquePickingBatchAccumulator( void )
{
	return m_opaquePickingBatches;
}

ITriRenderBatchAccumulator* Tr2InteriorScene::GetPickingBatchAccumulator( void )
{
	return m_pickingBatches;
}

ITr2ShaderMaterial* Tr2InteriorScene::GetPickingEffect( PickComponents pass )
{
	return m_pickEffect;
}

// -------------------------------------------------------------
// Description:
//   Returns an array of passes that need to be rendered in order
//   to get all picking components.
// Arguments:
//   requestedComponents  - Components requested for picking operation
//                          (union of PickComponent).
//   passes - (out) Array components actually rendered during each
//            picking pass (union of PickComponent). Maximum number
//            of passes is MAX_PICK_PASSES.
// Return Value:
//   Number of passes required to query all requested picking
//   components.
// -------------------------------------------------------------
unsigned int Tr2InteriorScene::GetRequiredPasses( PickComponents requestedComponents, PickComponents* passes )
{
	const PickComponents all = PICK_OBJECT | PICK_AREA | PICK_POSITION | PICK_UV;
	if( ( requestedComponents & all ) == all )
	{
		passes[0] = PICK_OBJECT | PICK_AREA | PICK_POSITION;
		passes[1] = PICK_UV;
		return 2;
	}

	passes[0] = requestedComponents;
	return 1;
}

// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DecodeBufferPixel( const void* pBuffer, PickComponents pass, BufferResults& results ) const
{
	//	WodExteriorPicker::DecodeBufferPixel should match what's in this function
	//	This function specialization should go away when wodexterior and tr2interior scenes are merged, I hope
	//	In the meantime if you make changes here, make sure they end up in WodExteriorPicker::DecodeBufferPixel

	// helpers: get each channel
	float a = *( ( float* )pBuffer + 3 );
	float r = *( ( float* )pBuffer + 2 );
	float g = *( ( float* )pBuffer + 1 );
	float b = *( ( float* )pBuffer + 0 );
	// put it "together"
	results.objectId = unsigned short( b + 0.5f );
	results.objectId--;
	if( pass & PICK_UV )
	{
		results.uv.x = r;
		results.uv.y = a;
		if( pass & PICK_POSITION )
		{
			// put it "together"
			results.depth = g;
		}
		else
		{
			results.areaId = unsigned short( g + 0.5f );
		}
	}
	else
	{
		results.areaId = unsigned short( g + 0.5f );
		results.depth = r;
	}
}

void Tr2InteriorScene::SetBackgroundCubemapResPath()
{
	if( m_backgroundCubeMapRes )
	{
		m_backgroundCubeMapRes->RemoveNotifyTarget( this );
		m_backgroundCubeMapRes.Unlock();
	}

	BeResMan->GetResource( m_backgroundCubeMapPath.c_str(), "", m_backgroundCubeMapRes );

	if( m_backgroundCubeMapRes )
	{
		m_backgroundCubeMapRes->AddNotifyTarget( this );
	}
}

void Tr2InteriorScene::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_backgroundCubeMapRes )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		SetEnvironmentCubeMapToEnlighten( renderContext );
	}
}

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorScene::PreviewEnlighten( TriEnlightenProgressBar &progress )
{
	m_enlightenUpdateTaskManager.WaitToComplete();
	m_enlightenUpdateTaskManager.InvalidateResult();

	TriEnlightenProgressBar prog;
	return BuildEnlightenImpl( Tr2InteriorEnlightenSystemImpl::QUALITY_PREVIEW, progress );
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorScene::BuildEnlighten( TriEnlightenProgressBar &progress )
{
	m_enlightenUpdateTaskManager.WaitToComplete();
	m_enlightenUpdateTaskManager.InvalidateResult();

	return BuildEnlightenImpl( Tr2InteriorEnlightenSystemImpl::QUALITY_RELEASE, progress );
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorScene::BuildEnlightenImpl( Tr2InteriorEnlightenSystemImpl::Quality quality, TriEnlightenProgressBar& prog )
{
	CCP_LOG_CH( g_enlightenBuildChannel, "Enlighten build" );

	m_enlightenUpdateTaskManager.WaitToComplete();
	m_enlightenUpdateTaskManager.InvalidateResult();

	BeTimer timer;

	Enlighten::IPrecompute* pPrecompute = Enlighten::CreatePrecompute();
	ON_BLOCK_EXIT( &Enlighten::IPrecompute::Release, pPrecompute );// Can't use CComPtr

	if( g_outputEnlightenDebugBuildInfo )
	{
		_mkdir( "C:\\EnlightenDebug" );
		CCP_LOGWARN_CH( g_enlightenBuildChannel, "Dumping Enlighten Precompute state to C:\\EnlightenDebug" );
		pPrecompute->SetStateDumpFolder( L"C:\\EnlightenDebug" );
		pPrecompute->SetStateDump( pPrecompute->esdInputsOnly );
	}

	// Build packed systems
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		if( !( *it )->BuildPackedSystems( quality, pPrecompute, prog ) )
		{
			prog.Finished( false );
			return false;
		}
	}

	if( prog.ShouldCancel() )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Enlighten build canceled by user" );
		prog.Finished( false );
		return false;
	}

	// Gather packed systems from neighboring cells
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->GatherPackedSystems();
	}

	if( prog.ShouldCancel() )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Enlighten build canceled by user" );
		prog.Finished( false );
		return false;
	}

	// Build system clustering
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		if( !( *it )->BuildSystemPreClusters( pPrecompute, prog ) )
		{
			prog.Finished( false );
			return false;
		}
	}

	if( prog.ShouldCancel() )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Enlighten build canceled by user" );
		prog.Finished( false );
		return false;
	}

	// Gather system clustering from neighboring cells
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->GatherSystemPreClusters();
	}

	if( prog.ShouldCancel() )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Enlighten build canceled by user" );
		prog.Finished( false );
		return false;
	}

	// Build system clustering
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		if( !( *it )->BuildSystemClusters( pPrecompute, prog ) )
		{
			prog.Finished( false );
			return false;
		}
	}

	if( prog.ShouldCancel() )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Enlighten build canceled by user" );
		prog.Finished( false );
		return false;
	}

	// Gather system clustering from neighboring cells
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->GatherSystemClusters();
	}

	if( prog.ShouldCancel() )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Enlighten build canceled by user" );
		prog.Finished( false );
		return false;
	}

	// Build light probes
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->BuildLightProbes( pPrecompute, prog );
	}

	if( prog.ShouldCancel() )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Enlighten build canceled by user" );
		prog.Finished( false );
		return false;
	}

	// Finally, build enlighten solution
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->BuildEnlightenSolution( pPrecompute, prog );
	}

	// Cleanup data used during precompute
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->DeletePrecompData();
	}

	CCP_LOG_CH( g_enlightenBuildChannel, "Time taken: %fsec", timer.GetSeconds() );

	prog.Finished( true );

	return true;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorScene::BuildLightProbes()
{
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		if( !( *it )->CanBuildLightProbes() )
		{
			CCP_LOGERR( "Full Enlighten build is required" );
			return false;
		}
	}

	// CCP_LOG 'progress bar'
	TriEnlightenProgressBar prog;

	Enlighten::IPrecompute* pPrecompute = Enlighten::CreatePrecompute();
	ON_BLOCK_EXIT( &Enlighten::IPrecompute::Release, pPrecompute );// Can't use CComPtr

	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->BuildLightProbes( pPrecompute, prog );
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::SaveEnlighten()
{
	// Save enlighten solutions for all cells
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->SaveEnlightenSystems();
	}
}
#endif

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorScene::PopulateProbeVolumes()
{
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->PopulateProbeVolumes();
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorScene::RebuildSceneData( void )
{
	// Update internal data in all cells
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		( *it )->RebuildInternalData();
	}

	// Clear out any dynamics pending load
	m_dynamicsPendingLoad.Clear();

	// Visit all dynamics
	for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
		// Remove from the scene
		( *it )->RemoveFromScene();

		// Remove from cells
		for( PTr2InteriorCellVector::iterator cellIt = m_cells.begin(); cellIt != m_cells.end(); ++cellIt )
		{
			( *cellIt )->RemoveDynamic( *it );
		}

		// Attempt to re-add to the scene
		if( !( *it )->AddToScene( m_apexScene ) && ( m_dynamicsPendingLoad.FindKey( ( *it ) ) == -1 ) )
		{
			// Object not ready, add to pending load queue
			m_dynamicsPendingLoad.Insert( -1, ( *it ) );
		}
	}

	UpdateSHScaleFactor();
}

void Tr2InteriorScene::SetEnvironmentCubeMapToEnlighten( Tr2RenderContext &renderContext )
{
	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return;
	}

	// 6 faces, 2x2 each
	for( int f = 0; f < 6; ++f )
	{
		for( int p = 0; p < 4; ++p )
		{
			m_enlightenInputEnvironmentLightingCache[4*f+p] = Geo::VZero();
		}
	}

	// Update enlighten with the environment
	if( !m_backgroundCubeMapRes || !m_backgroundCubeMapRes->IsGood()  )
	{
		UpdateEnvironmentLightingFromCacheWithMultiplier();
		return;
	}

	if( m_backgroundCubeMapRes->GetType() != TEX_TYPE_CUBE )
	{
		CCP_LOGERR( "Background texture is not a cubemap, could not add it to enlighten environment lighting" );
		UpdateEnvironmentLightingFromCacheWithMultiplier();
		return;
	}

	const unsigned int cubemapWidth = m_backgroundCubeMapRes->GetWidth();
	const unsigned int cubemapHeight = m_backgroundCubeMapRes->GetHeight();

	CCP_ASSERT( cubemapWidth == cubemapHeight );

	// Need to get a 2x2 cubemap, so work out what level that is
	unsigned int levelNeeded = 0;

	// increment levelNeeded until it's equal to or larger than the width
	while( ( unsigned int )( 2 << levelNeeded ) < cubemapWidth )
	{
		++levelNeeded;
	}

	// This could fail if the cubemap does not use a power of 2 ( w x h )
	CCP_ASSERT( ( 2 << levelNeeded ) == cubemapWidth );

	if( m_backgroundCubeMapRes->GetMipLevelCount() <= levelNeeded )
	{
		CCP_LOGERR( "Could not create enlighten environment cubemap, not enough MIP levels?" );
		return;
	}

	XMVECTOR color[24];

	Tr2RenderTargetAL faceRT;
	Tr2TextureAL face;

	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN( faceRT.Create( 2, 2, 1, PIXEL_FORMAT_R32G32B32A32_FLOAT, renderContext ) );	
		CR_RETURN( face.Create2D( 4, 4, 1, m_backgroundCubeMapRes->GetTexture()->GetFormat(), 0, nullptr, renderContext ) );
	}

	renderContext.m_esm.BeginManagedRendering();
	ON_BLOCK_EXIT( [&]{ renderContext.m_esm.EndManagedRendering(); } );

	Tr2PushPopDS pushPopDS( nullDS, renderContext );

	for( unsigned i = 0; i < 6; ++i )
	{
		if( FAILED( face.CopySubresourceRegion( 
			Tr2TextureSubresource( CUBEMAP_FACE_POSITIVE_X ), 
			*m_backgroundCubeMapRes->GetTexture(), 
			Tr2TextureSubresource( CubemapFace( i ), levelNeeded - 1 ), 
			renderContext ) ) )
		{
			CCP_LOGERR( "Could not get environment cubemap face" );
			return;
		}

		Tr2Renderer::PushRenderTarget( faceRT, renderContext );
		renderContext.Clear( CLEARFLAGS_TARGET, 0, 0.f );
		Tr2Renderer::DrawTexture( face, Vector2( 0.f, 0.f ), Vector2( 1.f, 1.f ), Tr2Blitter::FILTER_LINEAR );
		Tr2Renderer::PopRenderTarget( renderContext );

		void* data;
		unsigned pitch;

		if( FAILED( faceRT.Lock( 0, nullptr, data, pitch, renderContext ) ) )
		{
			CCP_LOGERR( "Could not get environment cubemap face" );
			return;
		}

		for( unsigned j = 0; j < 2; ++j )
		{
			memcpy( color + 4 * i + j * 2, reinterpret_cast<char*>( data ) + pitch * j, sizeof( XMVECTOR ) * 2 );
		}

		if( FAILED( faceRT.Unlock( renderContext ) ) )
		{
			CCP_LOGERR( "Could not get environment cubemap face" );
			return;
		}
	}

	XMVECTOR power = XMVectorReplicate( 2.2f );
	for( unsigned i = 0; i < 24; ++i )
	{
		m_enlightenInputEnvironmentLightingCache[i] = XMVectorPow( color[i], power );
	}

	UpdateEnvironmentLightingFromCacheWithMultiplier();
}

void Tr2InteriorScene::UpdateEnvironmentLightingFromCacheWithMultiplier()
{
	Geo::GeoArray<Geo::v128> postMultipliedEnvironmentLighting( 24, Geo::VZero() );
	Geo::v128 multiplier = Geo::VBroadcast( m_enlightenEnvironmentMultiplicationFactor );

	for( int i = 0; i < 24;  ++i )
	{
		using namespace Geo;
		postMultipliedEnvironmentLighting[i] = multiplier * m_enlightenInputEnvironmentLightingCache[i];
	}

	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		(*it)->SetEnvironmentCube( postMultipliedEnvironmentLighting.GetArrayConst() );
	}
}

void Tr2InteriorScene::UpdateSHScaleFactor()
{
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		(*it)->SetSHScale( m_shScale );
	}
}

#endif
