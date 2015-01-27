#include "StdAfx.h"

#include "Tr2InteriorScene.h"

#include "TriProjection.h"
#include "TriSettingsRegistrar.h"
#include "TriView.h"
#include "TriViewport.h"
#if APEX_ENABLED
#include "Apex/Apex.h"
#include "Apex/Tr2ApexScene.h"
#endif
#include "Tr2VisibilityResults.h"
#include "Tr2InteriorMirror.h"
#include "Tr2ShaderMaterial.h"
#include "Tr2LitPerObjectData.h"
#include "Tr2AtlasTexture.h"
#include "Tr2InteriorParticleObject.h"
#include "Tr2PushPopDS.h"
#include "Tr2PushPopRT.h"

#include "include/ITr2DebugRenderer.h"

#include "TriLineSet.h"
#include "Tr2InteriorPlaceable.h"
#include "Tr2InteriorStatic.h"
#include "ITr2PhysicsUpdater.h"
#include "Curves/TriCurveSet.h"
#include "Tr2InteriorCell.h"
#include "Tr2TextureAtlas.h"
#include "Tr2Effect.h"
#include "TriFrustum.h"

using namespace Tr2RenderContextEnum;

extern ITr2DebugRendererPtr g_debugRenderer;

CCP_STATS_DECLARE( wodInteriorSceneShadowsNeedUpdating, "Trinity/Tr2InteriorScene/ShadowsNeedUpdating", true, CST_COUNTER_LOW, "Number of spotlight shadows that need updating" );
CCP_STATS_DECLARE( wodInteriorSceneShadowsUpdated, "Trinity/Tr2InteriorScene/ShadowsUpdated", true, CST_COUNTER_LOW, "Number of spotlight shadows updated" );
CCP_STATS_DECLARE( wodInteriorSceneIntersectingCellPortals, "Trinity/Tr2InteriorScene/IntersectingCellPortals", true, CST_COUNTER_LOW, "Number of temporary portals to fix intersecting cells" );

BLUE_DEFINE_INTERFACE( ITr2InteriorCullable );
BLUE_DEFINE_INTERFACE( ITr2Interior );

const unsigned int INTERIOR_SHADOW_MAP_FOM_SAMPLER = 9;
const unsigned int INTERIOR_SHADOW_MAP_MINMAX_SAMPLER = 9;	// recycle FOM

const unsigned int INTERIOR_SHADOW_MAP0_SAMPLER = 10;
const unsigned int INTERIOR_SHADOW_MAP1_SAMPLER = 11;

const unsigned int INTERIOR_SHADOW_MAP_MAX_RESOLUTION = 1024;
const unsigned int INTERIOR_SHADOW_ATLAS_RESOLUTION = 2048;

// Name of the High-level shader used for picking
static const char* s_pickingEffectName   = "Picking";

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
	PARENTLOCK( m_lights ),
	PARENTLOCK( m_dynamics ),
	PARENTLOCK( m_dynamicsPendingLoad ),
	PARENTLOCK( m_curveSets ),
	m_renderDebugInfo( false ),
	m_sunDirection( 0.0f, 0.0f, 1.0f ),
	m_sunDiffuseColor( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_sunSpecularColor( 0.8f, 0.8f, 0.8f, 1.0f ),
	m_ambientColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_lastUpdateTime( 0 ),
	m_apexLODResourceBudget( 100000.0f ),
	m_apexLODResourceBudgetConsumed( 0.0f ),
	m_visualizeMethod( VM_NONE ),
	m_pickBuffer( NULL, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT, 1 ),
	m_shadowsUpdatesPerFrame( 2 ),
	m_shadowsLODSwitchesPerFrame( 1 ),
	m_useShadowLOD( true ),	
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

	// Initialize accumulator handles to NULL - these can point to the above accumulators, depending on the scene query type
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

	m_displayDynamics = true;
	m_displayStatics = true;
	m_lightGeneratingShadows = NULL;

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

	// Initialize SH scale factor
	m_shScale = 1.0f;

	PrepareResources();

	m_shadowFilter.CreateInstance();
	m_shadowFilter->SetEffectPathName( "res:/Graphics/Effect/Managed/Interior/Shadows/ShadowFilter.fx" );

	BeResMan->GetResource( "res:/Texture/Global/NdotLLibrary.png", "", m_nDotLTexture );
	m_nDotLTextureHandle = GlobalStore().RegisterPlaceholderTextureVariable( "ColorNdotLLookupMap" );

#if APEX_ENABLED
    // only create apex scene if apex is initialised which at this point is denoted by having the sdk loaded and available
    if( g_Tr2Apex && g_Tr2Apex->GetApexSDK() )
	{
		m_apexScene.CreateInstance();
		m_apexScene->CreateScene();
	}
#endif
}

Tr2InteriorScene::~Tr2InteriorScene()
{
#if APEX_ENABLED
	if( m_apexScene )
	{
		m_apexScene->DeleteScene();
	}
#endif
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
		for( auto it = m_cells.begin(); it != m_cells.end(); ++it )
		{
			( *it )->SetVisualizeMethod( m_visualizeMethod );
		}
    }
    else if( IsMatch( value, m_backgroundCubeMapPath ) )
	{
		SetBackgroundCubemapResPath();
	}
	else if( IsMatch( value, m_shScale ) )
	{
		UpdateSHScaleFactor();
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
}

void Tr2InteriorScene::Update( Be::Time realTime, Be::Time simTime )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( simTime == m_lastUpdateTime)
		// NB: Multiple calls on the same 'frame' should be ignored,
		//     but are not strictly errors, according to Dan Speed.
		return;

#if APEX_ENABLED
    // create the apex scene when it is intialised, should probably be done through the same python event that inits the sdk
    // but that is incredibly byzantine at the moment
    if( !m_apexScene && g_Tr2Apex && g_Tr2Apex->GetApexSDK() )
    {
        m_apexScene.CreateInstance();
		m_apexScene->CreateScene();
    }

	if( m_apexScene )
	{
		m_apexScene->PreUpdate( simTime, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}
#endif
	m_lastUpdateTime = simTime;

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
		m_ragdollScene->PrePhysics( simTime );
	}

	{
		CCP_STATS_ZONE( "UpdateCurves" );
		for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
		{
			( *it )->Update( realTime, simTime );
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

	UpdateLights();
	UpdateDynamics();
	UpdateCells();
	UpdateSecondaryLighting();

	{
		CCP_STATS_ZONE( "CellUpdate" );
		// since everything in the scene is in a cell, this ::Update spreads to all renderables
		for( Tr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
		{
			( *it )->Update( simTime );
		}
	}

	{
		CCP_STATS_ZONE( "PrePhysicsUpdate" );
		// Do the pre-physics update on the dynamics
		// Note: this used to happen in the cell pre-physics update, but that can cause double animation ticks
		// when a skinned object is intersecting multiple cells (e.g. crossing a portal)
		for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
		{
			( *it )->PrePhysicsUpdate( simTime );
		}
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
			( *it )->PostPhysicsUpdate( simTime, m_apexScene );
		}
	}

	{
		CCP_STATS_ZONE( "Update" );
		for( PITr2InteriorLightVector::iterator it = m_lights.begin(); it != m_lights.end(); ++it )
		{
			( *it )->Update( simTime );
		}
	}
#if APEX_ENABLED
	if( m_apexScene )
	{
		m_apexScene->PostUpdate( simTime, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}
#endif
}


namespace
{
float GetCellDistance( Tr2InteriorCell* cell, const Vector3& point )
{
	Vector3 minBounds, maxBounds;
	if( !cell->GetBoundingBox( minBounds, maxBounds ) )
	{
		return 0;
	}

	float distance;
	distance = point.x - minBounds.x;
	distance = std::min( distance, point.y - minBounds.y );
	distance = std::min( distance, point.z - minBounds.z );
	distance = std::min( distance, maxBounds.x - point.x );
	distance = std::min( distance, maxBounds.y - point.y );
	distance = std::min( distance, maxBounds.z - point.z );
	return std::max( distance, 0.0f );
}
}

void Tr2InteriorScene::UpdateSecondaryLighting()
{
	for( auto di = m_dynamics.begin(); di != m_dynamics.end(); ++di )
	{
		Vector3 position;
		if( !( *di )->GetShProbePosition( position ) )
		{
			continue;
		}
		Matrix& red = ( *di )->GetRedLightProbeMatrix();
		Matrix& green = ( *di )->GetGreenLightProbeMatrix();
		Matrix& blue = ( *di )->GetBlueLightProbeMatrix();
		memset( &red, 0, sizeof( Matrix ) );
		memset( &green, 0, sizeof( Matrix ) );
		memset( &blue, 0, sizeof( Matrix ) );
		float totalDistance = 0;
		for( auto ci = m_cells.begin(); ci != m_cells.end(); ++ci )
		{
			if( !( *ci )->HasSHProbes() )
			{
				continue;
			}
			float distance = GetCellDistance( *ci, position );
			if( distance > 0 )
			{
				Matrix r, g, b;
				( *ci )->GetSHProbe( position, r, g, b );
				red += r * distance;
				green += g * distance;
				blue += b * distance;
				totalDistance += distance;
			}
		}
		if( totalDistance > 0 )
		{
			totalDistance = 1.f / totalDistance;
			red *= totalDistance;
			green *= totalDistance;
			blue *= totalDistance;
		}
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

	VisibilityQuery( m_visibilityResults );
#if APEX_ENABLED
	if( m_apexScene )
	{
		m_apexScene->PreRender( m_lastUpdateTime, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}
#endif
	// Only render the cubemaps here
	//RenderShadowMaps();

	// Do the full-forward render
	RenderFullForward( renderContext );

	// debug info
	RenderDebugInfo( renderContext );

	// Clear lights
	m_activeLightSet.Clear();
#if APEX_ENABLED
	if( m_apexScene )
	{
		m_apexScene->PostRender( m_lastUpdateTime, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}
#endif
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

	m_visibilityQueryType = PRIMARY_QUERY;

	ResolveVisibility( Tr2Renderer::GetViewTransform(), Tr2Renderer::GetProjectionTransform(), true );
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

namespace
{

XMVECTOR GetMirrorClipPlane( const Tr2InteriorMirror* mirror, const Matrix& view, const Matrix& projection )
{
	auto& m = mirror->GetWarpMatrixFront();
	XMVECTOR det;
	XMVECTOR clipPlane = XMVectorSet( -m.GetZ().x, -m.GetZ().y, -m.GetZ().z, D3DXVec3Dot( &m.GetTranslation(), &m.GetZ() ) );
	return XMPlaneTransform( 
		clipPlane, 
		XMMatrixTranspose( XMMatrixInverse( &det, mirror->GetTransformMatrix() * view * projection ) ) );
}

}

void Tr2InteriorScene::FollowMirror( const Tr2InteriorMirror* mirror, const Matrix& objectToWorld, const TriFrustum& frustum, const Matrix& view, size_t depth, size_t maxDepth )
{
	XMVECTOR det;
	Matrix mirrorMatrix2(
		XMMatrixMultiply(
			XMMatrixInverse( &det, XMMatrixMultiply( mirror->GetWarpMatrixBack(), mirror->GetTransformMatrix() ) ),
			XMMatrixMultiply( mirror->GetWarpMatrixFront(), mirror->GetTransformMatrix() ) ) );

	Matrix newView( XMMatrixInverse( &det, XMMatrixMultiply( XMMatrixInverse( &det, view ), mirrorMatrix2 ) ) );
	TriFrustum newFrustum;
	Vector3 camPos( XMVector3Transform( frustum.m_viewPos, mirrorMatrix2 ) );
	CTriViewport vp;
	newFrustum.DeriveFrustum( &newView, &camPos, &frustum.m_projectionMatrix, vp );

	Vector4 clipPlane( GetMirrorClipPlane( mirror, view, Tr2Renderer::GetProjectionRawTransform() ) );

	OnPortalEnter( mirror, objectToWorld, true, clipPlane );
	DoVisibilityQuery( newFrustum, newView, depth + 1, maxDepth, mirrorMatrix2 );
	OnPortalExit( mirror, objectToWorld, false, clipPlane );
}

void Tr2InteriorScene::DoVisibilityQuery( const TriFrustum& frustum, const Matrix& view, size_t depth, size_t maxDepth, const Matrix& mirrorMatrix )
{
	if( depth == 0 )
	{
		OnQueryBegin();
	}
	for( auto ct = m_cells.begin(); ct != m_cells.end(); ++ct )
	{
		for( auto it = ( *ct )->m_statics.begin(); it != ( *ct )->m_statics.end(); ++it )
		{
			Matrix objectToWorld;
			if( ( *it )->IsInFrustum( frustum, objectToWorld ) )
			{
				if( depth )
				{
					objectToWorld = objectToWorld * mirrorMatrix;
				}
				OnInstanceVisible( *it, objectToWorld );
			}
		}
	}
	struct Mirror
	{
		Tr2InteriorMirror* mirror;
		Matrix objectToWorld;
	};
	std::vector<Mirror> mirrors;

	for( auto it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
		Matrix objectToWorld;
		if( ( *it )->IsInFrustum( frustum, objectToWorld ) )
		{
			if( depth )
			{
				objectToWorld = objectToWorld * mirrorMatrix;
			}
			OnInstanceVisible( *it, objectToWorld );
			if( depth < maxDepth )
			{
				for( size_t i = 0; i < ( *it )->GetMirrorCount(); ++i )
				{
					Mirror mirror;
					mirror.mirror = ( *it )->GetMirror( i );
					mirror.objectToWorld = objectToWorld;
					mirrors.push_back( mirror );
				}
			}
		}
	}
	for( auto it = m_lights.begin(); it != m_lights.end(); ++it )
	{
		Matrix objectToWorld;
		if( ( *it )->IsInFrustum( frustum, objectToWorld ) )
		{
			if( depth )
			{
				objectToWorld = objectToWorld * mirrorMatrix;
			}
			OnInstanceVisible( *it, objectToWorld );
		}
	}
	for( auto it = mirrors.begin(); it != mirrors.end(); ++it )
	{
		FollowMirror( it->mirror, it->objectToWorld, frustum, view, depth, maxDepth );
	}
	if( depth == 0 )
	{
		OnQueryEnd();
	}
}

void Tr2InteriorScene::ResolveVisibility( const Matrix& view, const Matrix& projection, size_t maxDepth )
{
	CTriViewport viewport;
	TriFrustum frustum;
	XMVECTOR det;
	Matrix viewInv( XMMatrixInverse( &det, view ) );
	frustum.DeriveFrustum( &view, (Vector3*)(&viewInv._41), &projection, viewport );
	DoVisibilityQuery( frustum, view, 0, maxDepth, Tr2Renderer::GetIdentityTransform() );
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
    default:
        break;
	}

	return PASS_RESULT_OK;
}

void Tr2InteriorScene::BeginRender( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	D3DPERF_EVENT( L"BeginRender" );
#if APEX_ENABLED
	if( m_apexScene )
	{
		m_apexScene->PreRender( m_lastUpdateTime, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}
#endif
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
		renderContext.RenderBatchesWithOverride( &lightBatches, m_visualizerEffects[VM_LIGHT_PRE_PASS_LIGHT_OVERDRAW], Tr2RenderContext::OM_DO_NOTHING );
	}
	else
	{
		renderContext.RenderLightBatches( &lightBatches );
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

	m_shSolver.Clear();
	GatherPrepassForwardBatches( m_visibilityResults );
	if( m_enableSHSolver )
	{
		m_shSolver.Solve( m_lights, renderContext );
	}

	// Render geometry
	renderContext.SetReadOnlyDepth( true );
	RenderGeometry( m_visualizerEffects[m_visualizeMethod == VM_LIGHT_PRE_PASS_LIGHT_OVERDRAW ? VM_LIGHT_PRE_PASS_LIGHTING : m_visualizeMethod], renderContext );
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
	m_activePrimaryRenderBatches = &lightBatches;
	m_activeTransparentBatchStore = m_transparentBatchStore;
	GatherFlareBatches( m_visibilityResults );
	lightBatches.Finalize();

	renderContext.m_esm.BeginManagedRendering();

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );

	renderContext.SetReadOnlyDepth( true );
	renderContext.RenderBatches( &lightBatches );
	renderContext.m_esm.UnsetAllTextures();
	renderContext.SetReadOnlyDepth( false );
	lightBatches.Clear();
	m_transparentBatchStore->Clear();

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
	renderContext.m_esm.EndManagedRendering();
}

void Tr2InteriorScene::EndRender( Tr2RenderContext& renderContext )
{
	// Clear lights
	m_activeLightSet.Clear();
#if APEX_ENABLED
	if( m_apexScene )
	{
		m_apexScene->PostRender( m_lastUpdateTime, m_apexLODResourceBudget, m_apexLODResourceBudgetConsumed );
	}
#endif
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
		renderContext.RenderBatchesWithOverride(
			m_activePrimaryRenderBatches,
			overrideEffect,
			Tr2RenderContext::OM_DO_NOTHING );
	}

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
			callback.count = unsigned( inCount );

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

		for( PITr2InteriorDynamicVector::const_iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
		{
			if( const Tr2InteriorParticleObject* particle = dynamic_cast<const Tr2InteriorParticleObject*>( *it ) )
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

				RenderBoxIntersection( minBounds1, maxBounds1, Tr2Renderer::GetIdentityTransform(),
									   minBounds2, maxBounds2, Tr2Renderer::GetIdentityTransform() );
			}
		}
	}
#if APEX_ENABLED
	if( m_apexScene )
	{
		m_apexScene->RenderDebugInfo( renderContext );
	}
#endif
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

		bool oldDisplayDynamics = m_displayDynamics;
		m_displayDynamics = ( it->first.lightSource->GetShadowCasterTypes() & ITr2InteriorLight::ST_DYNAMICS_ONLY ) != 0;
		m_displayStatics = ( it->first.lightSource->GetShadowCasterTypes() & ITr2InteriorLight::ST_STATICS_ONLY ) != 0;

		m_visibilityQueryType = SHADOW_QUERY;
		m_lightGeneratingShadows = it->first.lightSource;
		ResolveVisibility( 
			it->first.lightSource->GetViewMatrix( it->first.shadowMapIndex ), 
			it->first.lightSource->GetProjectionMatrix( it->first.shadowMapIndex ), 
			false );

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
			renderContext.RenderBatches( m_shadowBatches );
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

		Vector3 focalPosition = Tr2Renderer::GetViewPosition();
		// put all lightsources in vector with sorting values
		std::vector<ITr2InteriorLight::LightSourceItem> sortedLightSources;
		std::vector<ITr2InteriorLight::LightSourceItem> sortedLightLODSources;
		for( PITr2InteriorLightVector::const_iterator it = m_lights.begin(); it != m_lights.end(); ++it )
		{
			if( m_visibleLights.find( *it ) == m_visibleLights.end() )
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
							texture->GetRawRoot()->Unlock();
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
					texture->GetRawRoot()->Unlock();
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

		bool oldDisplayDynamics = m_displayDynamics;
		m_displayDynamics = ( item.lightSource->GetShadowCasterTypes() & ITr2InteriorLight::ST_DYNAMICS_ONLY ) != 0;
		m_displayStatics = ( item.lightSource->GetShadowCasterTypes() & ITr2InteriorLight::ST_STATICS_ONLY ) != 0;

		m_visibilityQueryType = SHADOW_QUERY;
		m_lightGeneratingShadows = item.lightSource;
		ResolveVisibility( 
			item.lightSource->GetViewMatrix( item.shadowMapIndex ),
			item.lightSource->GetProjectionMatrix( item.shadowMapIndex ), 
			false );

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
			renderContext.RenderBatches( m_shadowBatches );
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

		lightSource->AddToScene();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes an interior light source from the scene.  The light source is also removed
//   from all cells containing it.  It is an error to remove a
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
//   containing it.  If the dynamic is in the pending load
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
				if( ( *dit )->IsBoundingBoxReady() )
				{
					( *dit )->TestCellIntersectionAndAdd( cell );
				}
			}
			cell->ResetDirtyFlag();
		}
		cell->MarkShadowsDirtyForSkinnedObjects();
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called at the beginning of visibility query.  
// See Also:
//   OnQueryEnd, DoQueryBegin, DoQueryBeginPrePass
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnQueryBegin( void )
{
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::QUERY_BEGIN;

	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		m_visibleLights.clear();
		m_visibilityResults->AddVisibilityEvent( event );
	}
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		DoQueryBegin( event, IMMEDIATE_GATHER );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   This function is called at the end of visibility query.  
// See Also:
//   OnQueryBegin, DoQueryEnd
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::OnQueryEnd( void )
{
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::QUERY_END;

	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		m_visibilityResults->AddVisibilityEvent( event );
	}
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		DoQueryEnd( event, IMMEDIATE_GATHER );
	}
}

void Tr2InteriorScene::OnInstanceVisible( ITr2InteriorCullable* cullable, const Matrix& objectToWorld )
{
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::INSTANCE_VISIBLE;
	event.m_userData = cullable;
	event.m_objectToWorldMatrix = objectToWorld;

	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		ITr2InteriorLight* light = dynamic_cast<ITr2InteriorLight*>( cullable );
		if( light )
		{
			m_visibleLights.insert( light );
		}
		m_visibilityResults->AddVisibilityEvent( event );
	}
	else if( m_visibilityQueryType == SHADOW_QUERY )
	{
		DoInstanceVisible( event, IMMEDIATE_GATHER );
	}
	else if( m_visibilityQueryType == PICKING_QUERY )
	{
		ITr2Renderable* renderable = dynamic_cast<ITr2Renderable*>( cullable );
		if( renderable )
		{
			m_visibleObjects.push_back( renderable );
		}
	}
}

void Tr2InteriorScene::OnPortalEnter( const Tr2InteriorMirror* mirror, const Matrix& objectToWorld, bool isMirrored, const Vector4& clipPlane )
{
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::PORTAL_ENTER;
	event.m_userData = mirror->GetPlaceable()->GetRawRoot();
	event.m_mirrorIndex = mirror->GetMirrorIndex();
	event.m_objectToWorldMatrix = objectToWorld;

	event.m_stencilTest = 0;
	event.m_stencilWrite = 1;

	event.m_isMirroredInLeftHandedSpace = !isMirrored;

	int left = 0, top = 0, right = Tr2Renderer::GetViewport().width, bottom = Tr2Renderer::GetViewport().height;
	event.m_scissorRect.left = left;
	event.m_scissorRect.top = top;
	event.m_scissorRect.right = right;
	event.m_scissorRect.bottom = bottom;
	event.m_useClipPlane = false;
	if( isMirrored )
	{
		event.m_clipPlane = clipPlane;
		event.m_useClipPlane = true;
	}

	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		m_visibilityResults->AddVisibilityEvent( event );
	}
}

void Tr2InteriorScene::OnPortalExit( const Tr2InteriorMirror* mirror, const Matrix& objectToWorld, bool isMirrored, const Vector4& clipPlane )
{
	Tr2VisibilityEvent event;
	event.m_eventType = Tr2VisibilityEvent::PORTAL_EXIT;
	event.m_userData = mirror->GetPlaceable()->GetRawRoot();
	event.m_mirrorIndex = mirror->GetMirrorIndex();
	event.m_objectToWorldMatrix = objectToWorld;

	event.m_isMirroredInLeftHandedSpace = !isMirrored;

	int left = 0, top = 0, right = Tr2Renderer::GetViewport().width, bottom = Tr2Renderer::GetViewport().height;
	event.m_scissorRect.left = left;
	event.m_scissorRect.top = top;
	event.m_scissorRect.right = right;
	event.m_scissorRect.bottom = bottom;
	event.m_useClipPlane = false;
	if( isMirrored )
	{
		event.m_clipPlane = clipPlane;
		event.m_useClipPlane = true;
	}

	if( m_visibilityQueryType == PRIMARY_QUERY )
	{
		m_visibilityResults->AddVisibilityEvent( event );
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

	// Clear mirror matrix
	m_mirrorToWorldMatrix = Tr2Renderer::GetIdentityTransform();
	m_mirrorToWorldMatrixStack.clear();
	m_mirrorToWorldMatrixStack.push_back( m_mirrorToWorldMatrix );

	// Clear lights
	m_activeLightSet.Clear();

	// Clear the portal entry flag and object group counter
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
		if( m_enableROIs )
		{
			for( auto it = m_lights.begin(); it != m_lights.end(); ++it )
			{
				m_activeLightSet.AddLight( *it, Vector3( 0.f, 0.f, 0.f ) );
			}
		}
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

	Tr2InteriorMirror* mirror = dynamic_cast<ITr2Interior*>( event.m_userData.p )->GetMirror( event.m_mirrorIndex );

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

	DoStencilMask( event, gatherType );
	DoViewParametersChanged( event, gatherType, WODINTBATCHGROUP_BEGIN );
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

	DoPortalPreExit( event, gatherType );
	DoViewParametersChanged( event, gatherType, WODINTBATCHGROUP_END );

	// Early exit if there are no active primary batches
	if( !m_activePrimaryRenderBatches )
	{
		return;
	}

	Tr2InteriorPlaceable* placeable = dynamic_cast<Tr2InteriorPlaceable*>( event.m_userData.p );
	Tr2InteriorMirror* mirror = placeable->GetMirror( event.m_mirrorIndex );

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
			STENCILOP_DECR,
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
											    BatchGatherType gatherType, Tr2InteriorBatchGroup batchGroup )
{
	// Bail out if there is no active opaque batch accumulator
	if( !m_activePrimaryRenderBatches )
	{
		return;
	}

	if( gatherType == FLARE_GATHER || m_visibilityQueryType == SHADOW_QUERY )
	{
		return;
	}

	// Get the mirrored state
	bool isMirrored = !event.m_isMirroredInLeftHandedSpace;

	// Insert the new clipping batch into the opaques list
	Tr2InteriorClippingBatch* batch =
		m_activePrimaryRenderBatches->Allocate<Tr2InteriorClippingBatch>();
	if( batch )
	{
		// Need to offset scissor rect by the actual viewport origin
		int x = Tr2Renderer::GetViewport().x;
		int y = Tr2Renderer::GetViewport().y;
		Rect scissorRect;
		scissorRect.left = event.m_scissorRect.left;
		scissorRect.top = event.m_scissorRect.top;
		scissorRect.right = event.m_scissorRect.right;
		scissorRect.bottom = event.m_scissorRect.bottom;

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
		m_activePrimaryRenderBatches->SetUserData(
			ConstructKey( m_currentObjectGroup, batchGroup ) );

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

	bool opaqueOnly = false;
	if( m_visualizeMethod == VM_EN_ONLY ||
		( m_visualizeMethod >= VM_ALL_LIGHTING && m_visualizeMethod <= VM_LIGHT_PRE_PASS_SPECULAR_LIGHTING ) )
	{
		opaqueOnly = true;
	}
	if( gatherType == FLARE_GATHER && ( opaqueOnly || m_mirrorToWorldMatrixStack.size() > 1 ) )
	{
		return;
	}

	// Handle light-pass differently, since ITr2InteriorLights aren't ITr2Interiors
	if( gatherType == LIGHT_GATHER )
	{
		ITr2InteriorLight* light = dynamic_cast<ITr2InteriorLight*>( event.m_userData.p );
		if( !light )
		{
			return;
		}

		m_activeTransparentBatchStore->SetRenderingMode( Tr2EffectStateManager::RM_LIGHT );
		light->GetBatches( m_activeTransparentBatchStore, m_mirrorToWorldMatrix );

		// Update batch count to indicate the range of transparent batches gathered
		// for this cell
		m_transparencyStack.back().second =
			(unsigned int)m_activeTransparentBatchStore->GetBatchCount();
	}
	else
	{
		ITr2Interior* interior = dynamic_cast<ITr2Interior*>( event.m_userData.p );
		ITr2Renderable* renderable = dynamic_cast<ITr2Renderable*>( event.m_userData.p );

		if( interior && renderable )
		{
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

			if( m_enableSHSolver )
			{
				interior->SetSHLightingSolver( &m_shSolver );
			}
			else
			{
				interior->SetSHLightingSolver( NULL );
			}

			// Get the per-object data for opaque batches
			Tr2PerObjectData* perObjectOpaque =
				interior->GetPerObjectDataWithPerInstanceLighting(
					m_activePrimaryRenderBatches ?
						m_activePrimaryRenderBatches : m_activeTransparentBatchStore,
					gatherType == FULL_FORWARD_GATHER ? &m_activeLightSet : nullptr,
					event.m_objectToWorldMatrix,
					m_mirrorToWorldMatrix
				);

			if( m_activePrimaryRenderBatches )
			{
#define DO_GET_BATCHES( batches, mode, key, batchType )						\
	batches->SetRenderingMode( Tr2EffectStateManager::mode );				\
	batches->SetUserData( ConstructKey( m_currentObjectGroup, key ) );		\
	renderable->GetBatches( batches, batchType, perObjectOpaque );

				if( m_visibilityQueryType == SHADOW_QUERY )
				{
					DO_GET_BATCHES( m_activePrimaryRenderBatches, RM_OPAQUE, WODINTBATCHGROUP_OPAQUE, TRIBATCHTYPE_DEPTH );
				}
				else
				{
					if( gatherType == PREPASS_GATHER )
					{
						DO_GET_BATCHES( m_activePrimaryRenderBatches, RM_OPAQUE, WODINTBATCHGROUP_OPAQUE, TRIBATCHTYPE_DEPTHNORMAL );
						DO_GET_BATCHES( m_activePrimaryRenderBatches, RM_DECAL_NO_DEPTH, WODINTBATCHGROUP_DECAL, TRIBATCHTYPE_DECALNORMAL );
					}
					else if( gatherType == PREPASS_FORWARD_GATHER )
					{
						DO_GET_BATCHES( m_activePrimaryRenderBatches, RM_OPAQUE, WODINTBATCHGROUP_OPAQUE, TRIBATCHTYPE_OPAQUE_PREPASS );
						DO_GET_BATCHES( m_activePrimaryRenderBatches, RM_DECAL, WODINTBATCHGROUP_DECAL, TRIBATCHTYPE_DECAL_PREPASS );
					}
					else if( gatherType == FLARE_GATHER )
					{
						DO_GET_BATCHES( m_activePrimaryRenderBatches, RM_OPAQUE, WODINTBATCHGROUP_OPAQUE, TRIBATCHTYPE_FLARE );
					}
					else
					{
						DO_GET_BATCHES( m_activePrimaryRenderBatches, RM_OPAQUE, WODINTBATCHGROUP_OPAQUE, TRIBATCHTYPE_OPAQUE );
						DO_GET_BATCHES( m_activePrimaryRenderBatches, RM_DECAL, WODINTBATCHGROUP_DECAL, TRIBATCHTYPE_DECAL );
					}
				}
			}

			// Gather transparent batches
			if( !opaqueOnly && m_activeTransparentBatchStore && m_visibilityQueryType != SHADOW_QUERY && gatherType != FLARE_GATHER )
			{
				DO_GET_BATCHES( m_activeTransparentBatchStore, RM_ALPHA, WODINTBATCHGROUP_BLEND, TRIBATCHTYPE_TRANSPARENT );

				// Update batch count to indicate the range of transparent batches gathered
				// for this cell
				m_transparencyStack.back().second =
					(unsigned int)m_activeTransparentBatchStore->GetBatchCount();
			}
		}
	}
#undef DO_GET_BATCHES
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
	Tr2InteriorPlaceable* placeable = dynamic_cast<Tr2InteriorPlaceable*>( event.m_userData.p );

	Tr2InteriorMirror* mirror = placeable->GetMirror( event.m_mirrorIndex );
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
			STENCILOP_INCR,
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
			case Tr2VisibilityEvent::INSTANCE_VISIBLE:
				DoInstanceVisible( event, PREPASS_GATHER );
				break;
            default:
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
			case Tr2VisibilityEvent::INSTANCE_VISIBLE:
				DoInstanceVisible( event, LIGHT_GATHER );
				break;
            default:
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
			case Tr2VisibilityEvent::INSTANCE_VISIBLE:
				DoInstanceVisible( event, PREPASS_FORWARD_GATHER );
				break;
            default:
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
			case Tr2VisibilityEvent::INSTANCE_VISIBLE:
				DoInstanceVisible( event, FULL_FORWARD_GATHER );
				break;
            default:
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
			case Tr2VisibilityEvent::INSTANCE_VISIBLE:
				DoInstanceVisible( event, FLARE_GATHER );
				break;
            default:
                break;
			}
		}
	}
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

	for( PITr2InteriorDynamicVector::iterator dit = m_dynamics.begin(); dit != m_dynamics.end(); ++dit )
	{
		if( ( *dit )->IsDirty() && ( *dit )->IsBoundingBoxReady() )
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

			// Clear the dirty flag on the dynamic
			if( cellsReady )
			{
				( *dit )->SetDirtyFlag( false );
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorScene::OnLightsListModified( long event, ssize_t key, ssize_t key2, IRoot* currvalue )
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
bool Tr2InteriorScene::OnDynamicsListModified( long event, ssize_t key, ssize_t key2, IRoot* currvalue )
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
	Matrix newView( XMMatrixLookAtLH( eye, ( eye - dirWorld ), Vector3( 0.0f, 1.0f, 0.0f ) ) );

	// Set camera parameters
	Matrix proj;
	D3DXMatrixPerspectiveFovRH( &proj, fov, aspect, frustum.m_zNear, frustum.m_zFar );

	// Issue the query
	m_visibleObjects.clear();
	m_visibilityQueryType = PICKING_QUERY;
	ResolveVisibility( newView, proj, false );

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

int64_t Tr2InteriorScene::ConstructKey( unsigned int objectGroup, Tr2InteriorBatchGroup batchGroup )
{
	int64_t objGroup64 = ( int64_t )objectGroup << 48;

	int64_t batchGroup64 = ( int64_t )batchGroup << 32;

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
	results.objectId = (unsigned short)( b + 0.5f );
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
			results.areaId = (unsigned short)( g + 0.5f );
		}
	}
	else
	{
		results.areaId = (unsigned short)( g + 0.5f );
		results.depth = r;
	}
}

void Tr2InteriorScene::SetBackgroundCubemapResPath()
{
	if( m_backgroundCubeMapRes )
	{
		m_backgroundCubeMapRes.Unlock();
	}

	BeResMan->GetResource( m_backgroundCubeMapPath.c_str(), "", m_backgroundCubeMapRes );
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

void Tr2InteriorScene::UpdateSHScaleFactor()
{
	for( PTr2InteriorCellVector::iterator it = m_cells.begin(); it != m_cells.end(); ++it )
	{
		(*it)->SetSHScale( m_shScale );
	}
}

