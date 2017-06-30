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
#include "Tr2LitPerObjectData.h"
#include "Tr2AtlasTexture.h"
#include "Tr2PushPopDS.h"
#include "Tr2PushPopRT.h"

#include "TriLineSet.h"
#include "Tr2InteriorPlaceable.h"
#include "ITr2PhysicsUpdater.h"
#include "Curves/TriCurveSet.h"
#include "Tr2TextureAtlas.h"
#include "Shader/Tr2Effect.h"
#include "TriFrustum.h"
#include "Resources/TriTextureRes.h"

using namespace Tr2RenderContextEnum;

BLUE_DEFINE_INTERFACE( ITr2InteriorCullable );
BLUE_DEFINE_INTERFACE( ITr2Interior );


static const char* s_pickingEffectName   = "res:/graphics/effect/managed/interior/system/picking.fx";


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
	m_maxFogAmount( 0.0f ),
	m_maxFogDistance( 1000.0f ),
	m_minFogDistance( 0.0f ),
	m_fogColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_renderBackgroundCubeMap( true ),
	m_sunDiffuseColorVar( "Sun.WodDiffuseColor", m_sunDiffuseColor )
	, m_sunSpecularColorVar( "Sun.SpecularColor", m_sunSpecularColor )	
	, m_ambientColorVar( "Scene.AmbientColor", m_ambientColor )
	, m_cameraPosVar( "Camera.eyePosWorld", Vector3( 0.0f, 0.0f, 0.0f ) )
{
	m_backgroundCubeMapVar.Register( "EnvMap1", m_backgroundCubeMapRes );

	GlobalStore().RegisterVariable( "LightPrePassMap", (TriTextureRes*)NULL );
	GlobalStore().RegisterVariable( "LightAccumulationMap", (TriTextureRes*)NULL );
	//GlobalStore().RegisterVariable( "LightAccumulationSpecularMap", m_lightSpecularRenderPassTexture );
	GlobalStore().RegisterVariable( "LightPrePassDepthMap", (TriTextureRes*)NULL );

	// Create render batch accumulators
	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	m_primaryRenderBatches = CCP_NEW( "Tr2InteriorScene/m_primaryRenderBatches" ) TriRenderBatchAccumulator<Tr2IntKeyGenerator>( allocator );
	m_opaquePickingBatches = CCP_NEW( "Tr2InteriorScene/m_opaquePickingBatches" ) TriRenderBatchAccumulator<>( allocator );
	m_pickingBatches = CCP_NEW( "Tr2InteriorScene/m_pickingBatches" ) TriRenderBatchAccumulator<>( allocator );

	// picking
	m_pickBuffer.PrepareResources();
	m_pickEffect.CreateInstance();
	m_pickEffect->SetEffectPathName( s_pickingEffectName );
	m_pickBuffer.SetClearColor( 0x0 );

	// Variable Handles
	Vector3 dir( XMVectorMultiply(
		XMVector3Normalize( m_sunDirection ),
		XMVectorReplicate( -1.0f ) ) );

	m_sunDirectionVar.Register( "Sun.DirWorld", dir );	
	GlobalStore().RegisterVariable( "PickingComponents", Vector4() );
	
	// create debug renderer
	m_debugLines.CreateInstance();

	m_dynamics.SetNotify( this );

	PrepareResources();

	BeResMan->GetResource( "res:/Texture/Global/NdotLLibrary.png", "", m_nDotLTexture );
	m_nDotLTextureHandle = GlobalStore().RegisterVariable( "ColorNdotLLookupMap", static_cast<ITr2TextureProvider*>( nullptr ) );

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
	CCP_DELETE( m_pickingBatches );
	CCP_DELETE( m_opaquePickingBatches );

	// release debug renderer
	if( m_debugLines )
	{
		m_debugLines = NULL;
	}

    m_pickBuffer.ReleaseResources( TRISTORAGE_ALL );
}

bool Tr2InteriorScene::Initialize()
{
	SetBackgroundCubemapResPath();
	return true;
}

bool Tr2InteriorScene::OnModified( Be::Var* value )
{
    if( IsMatch( value, m_backgroundCubeMapPath ) )
	{
		SetBackgroundCubemapResPath();
	}

    return true;
}

// ---------------------------------------------------------------
void Tr2InteriorScene::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* currvalue, const IList* theList )
{
	if( ( event & BELIST_LOADING ) == 0 )
	{
		// Respond to an item removal event
		if( ( event & BELIST_EVENTMASK ) == BELIST_REMOVED )
		{
			if( currvalue )
			{
				// See if the removed item is a dynamic
				ITr2InteriorDynamic* dynamic = NULL;
				if( currvalue->QueryInterface( BlueInterfaceIID<ITr2InteriorDynamic>(), (void**)&dynamic ) )
				{
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
				if( currvalue->QueryInterface( BlueInterfaceIID<ITr2InteriorDynamic>(), (void**)&dynamic ) )
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
	}
}

void Tr2InteriorScene::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		m_perFramePSBuffer.Destroy();
		m_perFrameVSBuffer.Destroy();
	}
}

bool Tr2InteriorScene::OnPrepareResources()
{
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

void Tr2InteriorScene::ResolveVisibility( const Matrix& view, const Matrix& projection, size_t maxDepth )
{
	CTriViewport viewport;
	TriFrustum frustum;
	XMVECTOR det;
	Matrix viewInv( XMMatrixInverse( &det, view ) );
	frustum.DeriveFrustum( &view, (Vector3*)(&viewInv._41), &projection, viewport );

	OnQueryBegin();

	for( auto it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
		Matrix objectToWorld;
		if( ( *it )->IsInFrustum( frustum, objectToWorld ) )
		{
			OnInstanceVisible( *it, objectToWorld );
		}
	}
	for( auto it = m_lights.begin(); it != m_lights.end(); ++it )
	{
		Matrix objectToWorld;
		if( ( *it )->IsInFrustum( frustum, objectToWorld ) )
		{
			OnInstanceVisible( *it, objectToWorld );
		}
	}
}

void Tr2InteriorScene::RenderFullForward( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	D3DPERF_EVENT( L"Tr2InteriorScene::RenderFullForward" );

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
	GatherFullForwardBatches( m_visibilityResults );

	// Render geometry
	RenderGeometry( nullptr, renderContext );

	// Clear batches
	m_primaryRenderBatches->Clear();
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
			perObjectPSBuffer.mirrorToWorldMatrix = Tr2Renderer::GetIdentityTransform();

			// Copy buffer into the per-object data
			perObjectData->CopyToPSFloatBuffer( perObjectPSBuffer );

			batch->SetShaderMaterial( m_backgroundEffect );
			batch->SetPerObjectData( perObjectData );
			batch->SetRenderingMode( Tr2EffectStateManager::RM_ANY );
			batches->SetUserData(
				ConstructKey( 0, WODINTBATCHGROUP_BEGIN )
				);

			batches->Commit( batch );
		}
	}
}

void Tr2InteriorScene::RenderGeometry( Tr2Material* overrideEffect, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// From now on its managed!
	renderContext.m_esm.BeginManagedRendering();

	// Render primary batches - includes opaques, decals, transparents & special batches
	{
		D3DPERF_EVENT( L"Primary render batches" );
		m_primaryRenderBatches->Finalize();
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
		renderContext.RenderBatchesWithOverride(
			m_primaryRenderBatches,
			overrideEffect,
			Tr2RenderContext::OM_DO_NOTHING );
	}

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
	renderContext.m_esm.EndManagedRendering();
}

void Tr2InteriorScene::RenderDebugInfo( Tr2RenderContext& renderContext )
{
#if APEX_ENABLED
	if( m_apexScene )
	{
		m_apexScene->RenderDebugInfo( renderContext );
	}
#endif
}

ITr2MultiPassScene::RenderPassResult Tr2InteriorScene::RenderPass( PassType pass, Tr2RenderContext & renderContext )
{
	switch( pass )
	{
	case RP_MAIN_RENDER:
		Render( renderContext );
		break;
	default:
		break;
	}

	return PASS_RESULT_OK;
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
		CCP_LOG( "Tr2InteriorScene::RemoveDynamic() - interiorDynamic not found in the scene!" );
		return;
	}

	dynamic->RemoveFromScene();

	m_dynamics.Remove( pos );

	// See if this dynamic is in the pending load list
	pos = m_dynamicsPendingLoad.FindKey( dynamic );
	if( pos != -1 )
	{
		m_dynamicsPendingLoad.Remove( pos );
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
		m_visibilityResults->AddVisibilityEvent( event );
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
		m_visibilityResults->AddVisibilityEvent( event );
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
// See Also:
//   OnQueryBegin
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoQueryBegin( const Tr2VisibilityEvent& event )
{
	CCP_ASSERT( event.m_eventType == Tr2VisibilityEvent::QUERY_BEGIN );

	// Clear lights
	m_activeLightSet.Clear();

	// If we're gathering forward batches, then we need a background cubemap batch here
	PrepareBackgroundCubemapBatch( m_primaryRenderBatches );
	for( auto it = m_lights.begin(); it != m_lights.end(); ++it )
	{
		m_activeLightSet.AddLight( *it, Vector3( 0.f, 0.f, 0.f ) );
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
// See Also:
//   OnInstanceVisible, OnInstanceVisiblePrePass
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::DoInstanceVisible( const Tr2VisibilityEvent& event )
{
	CCP_STATS_ZONE( "DoInstanceVisible" );

	{
		ITr2Interior* interior = dynamic_cast<ITr2Interior*>( event.m_userData.p );
		ITr2Renderable* renderable = dynamic_cast<ITr2Renderable*>( event.m_userData.p );

		if( interior && renderable )
		{
			// Get the per-object data for opaque batches
			Tr2PerObjectData* perObjectOpaque =
				interior->GetPerObjectDataWithPerInstanceLighting(
					m_primaryRenderBatches,
					&m_activeLightSet,
					event.m_objectToWorldMatrix );

			if( m_primaryRenderBatches )
			{
#define DO_GET_BATCHES( batches, mode, key, batchType )						\
	batches->SetRenderingMode( Tr2EffectStateManager::mode );				\
	batches->SetUserData( ConstructKey( 0, key ) );							\
	renderable->GetBatches( batches, batchType, perObjectOpaque );

				DO_GET_BATCHES( m_primaryRenderBatches, RM_OPAQUE, WODINTBATCHGROUP_OPAQUE, TRIBATCHTYPE_OPAQUE );
				DO_GET_BATCHES( m_primaryRenderBatches, RM_DECAL, WODINTBATCHGROUP_DECAL, TRIBATCHTYPE_DECAL );
				DO_GET_BATCHES( m_primaryRenderBatches, RM_ALPHA, WODINTBATCHGROUP_BLEND, TRIBATCHTYPE_TRANSPARENT );
			}
		}
	}
#undef DO_GET_BATCHES
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
				DoQueryBegin( event );
				break;
			case Tr2VisibilityEvent::INSTANCE_VISIBLE:
				DoInstanceVisible( event );
				break;
            default:
                break;
			}
		}
	}
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
	data.shScale = 0;

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

Tr2Material* Tr2InteriorScene::GetPickingEffect( PickComponents pass )
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
	// Clear out any dynamics pending load
	m_dynamicsPendingLoad.Clear();

	// Visit all dynamics
	for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
		// Remove from the scene
		( *it )->RemoveFromScene();

		// Attempt to re-add to the scene
		if( !( *it )->AddToScene( m_apexScene ) && ( m_dynamicsPendingLoad.FindKey( ( *it ) ) == -1 ) )
		{
			// Object not ready, add to pending load queue
			m_dynamicsPendingLoad.Insert( -1, ( *it ) );
		}
	}
}
