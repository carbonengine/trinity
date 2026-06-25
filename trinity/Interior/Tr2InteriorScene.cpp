// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2InteriorScene.h"

#include "TriProjection.h"
#include "TriSettingsRegistrar.h"
#include "TriView.h"
#include "TriViewport.h"
#include "Tr2VisibilityResults.h"
#include "Tr2PerObjectData.h"
#include "Tr2AtlasTexture.h"
#include "Tr2PushPopDS.h"
#include "Tr2PushPopRT.h"

#include "Tr2InteriorPlaceable.h"
#include "Curves/TriCurveSet.h"
#include "Tr2TextureAtlas.h"
#include "Shader/Tr2Effect.h"
#include "TriFrustum.h"
#include "Resources/TriTextureRes.h"
#include "Tr2DebugRenderer.h"
#include "Tr2RenderTarget.h"
#include "Tr2InteriorLightSource.h"
#include "include/TriMath.h"
#include "Tr2DepthStencil.h"
#include "Tr2SkinnedObject.h"

using namespace Tr2RenderContextEnum;

BLUE_DEFINE_INTERFACE( ITr2InteriorCullable );
BLUE_DEFINE_INTERFACE( ITr2Interior );


namespace
{
const char* VISUALIZER_NAME[VM_COUNT] = {
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

Tr2InteriorScene::Tr2InteriorScene( IRoot* lockobj /*= NULL */ ) :
	PARENTLOCK( m_lights ),
	PARENTLOCK( m_dynamics ),
	PARENTLOCK( m_dynamicsPendingLoad ),
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_lightRenderTargets ),
	m_shadowSize( 1024 ),
	m_sunDirection( 0.0f, 0.0f, 1.0f ),
	m_sunDiffuseColor( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_sunSpecularColor( 0.8f, 0.8f, 0.8f, 1.0f ),
	m_ambientColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_lastUpdateTime( 0 ),
	m_visualizeMethod( VM_NONE ),
	m_pickBuffer( NULL, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT, 1 ),
	m_maxFogAmount( 0.0f ),
	m_maxFogDistance( 1000.0f ),
	m_minFogDistance( 0.0f ),
	m_shadowCount( 4 ),
	m_fogColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_renderBackgroundCubeMap( true ),
	m_sunDiffuseColorVar( "Sun.WodDiffuseColor", m_sunDiffuseColor ),
	m_sunSpecularColorVar( "Sun.SpecularColor", m_sunSpecularColor ),
	m_ambientColorVar( "Scene.AmbientColor", m_ambientColor ),
	m_cameraPosVar( "Camera.eyePosWorld", Vector3( 0.0f, 0.0f, 0.0f ) ),
	m_optimizeShadows( true ),
	m_renderShadows( true ),
	m_debugRenderShadowMaps( false ),
	m_shadowMap0Var( "SpotlightShadow0", (TriTextureRes*)NULL ),
	m_shadowMap1Var( "SpotlightShadow1", (TriTextureRes*)NULL ),
	m_shadowMap2Var( "SpotlightShadow2", (TriTextureRes*)NULL ),
	m_shadowMap3Var( "SpotlightShadow3", (TriTextureRes*)NULL )
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
	m_pickBuffer.SetClearColor( 0x0 );

	// Variable Handles
	Vector3 dir( XMVectorMultiply(
		XMVector3Normalize( m_sunDirection ),
		XMVectorReplicate( -1.0f ) ) );

	m_sunDirectionVar.Register( "Sun.DirWorld", dir );
	GlobalStore().RegisterVariable( "PickingComponents", Vector4() );

	m_dynamics.SetNotify( this );
	m_lights.SetNotify( this );

	PrepareResources();

	BeResMan->GetResource( "res:/texture/global/bluenoise32.png", "", m_noiseTexture );
	m_nDotLTextureHandle = GlobalStore().RegisterVariable( "ColorNdotLLookupMap", static_cast<ITr2TextureProvider*>( nullptr ) );

	BeResMan->GetResource( "res:/texture/global/noise.png", "", m_noiseTexture );
	m_noiseTextureHandle = GlobalStore().RegisterVariable( "NoiseMap", static_cast<ITr2TextureProvider*>( nullptr ) );

	BeResMan->GetResource( "res:/texture/global/white.dds", "", m_whiteTexture );
	m_shadowMap0Var = m_whiteTexture;
	m_shadowMap1Var = m_whiteTexture;
	m_shadowMap2Var = m_whiteTexture;
	m_shadowMap3Var = m_whiteTexture;
}

Tr2InteriorScene::~Tr2InteriorScene()
{
	CCP_DELETE( m_primaryRenderBatches );
	CCP_DELETE( m_pickingBatches );
	CCP_DELETE( m_opaquePickingBatches );

	m_pickBuffer.ReleaseResources( TRISTORAGE_ALL );

	m_shadowMap0Var = static_cast<ITr2TextureProvider*>( nullptr );
	m_shadowMap1Var = static_cast<ITr2TextureProvider*>( nullptr );
	m_shadowMap2Var = static_cast<ITr2TextureProvider*>( nullptr );
	m_shadowMap3Var = static_cast<ITr2TextureProvider*>( nullptr );
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

	if( IsMatch( value, m_shadowSize ) )
	{
		SetupShadowMaps();
	}

	if( IsMatch( value, m_shadowCount ) )
	{
		SetupShadowMaps();
	}

	return true;
}

// ---------------------------------------------------------------
void Tr2InteriorScene::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* currvalue, const IList* theList )
{
	if( theList == &m_lights )
	{
		SetupShadowMaps();
	}

	else if( theList == &m_dynamics )
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
}

void Tr2InteriorScene::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		m_perFramePSBuffer = Tr2ConstantBufferAL();
		m_perFrameVSBuffer = Tr2ConstantBufferAL();
	}
}

bool Tr2InteriorScene::OnPrepareResources()
{
	return true;
}

void Tr2InteriorScene::MaximizeShadowMapUsage( TriProjection& projection, const TriView* view, TriFrustum* frustum )
{
	Tr2SkinnedObject* character = nullptr;

	for( auto dynamicIt = m_dynamics.begin(); dynamicIt != m_dynamics.end(); dynamicIt++ )
	{
		ITr2InteriorDynamic* dynamic = ( *dynamicIt );
		Tr2SkinnedObjectPtr foundCharacter = BlueCastPtr( dynamic );

		if( foundCharacter )
		{
			if( character )
			{
				return;
			}

			character = foundCharacter;
		}
	}

	if( character )
	{
		Vector3 x, y, z, center, sizes;
		character->GetClippedWorldBoundingObb( character->GetSkinningTransform(), x, y, z, center, sizes, frustum );

		auto GetPoint = [&]( int i ) -> Vector4 {
			float scale;
			Vector3 point = center;
			if( ( i & 1 ) != 0 )
				scale = sizes[0];
			else
				scale = -sizes[0];
			point += scale * x;
			if( ( i & 2 ) != 0 )
				scale = sizes[1];
			else
				scale = -sizes[1];
			point += scale * y;
			if( ( i & 4 ) != 0 )
				scale = sizes[2];
			else
				scale = -sizes[2];
			point += scale * z;
			return Vector4( point.x, point.y, point.z, 1.0 );
		};

		float maxX = -1.0;
		float maxY = -1.0;
		float minX = 1.0;
		float minY = 1.0;

		Matrix VP = XMMatrixMultiply( view->GetTransform(), projection.GetTransform() );

		auto Constrain = []( float value ) -> float {
			value = min( 1.0f, value );
			value = max( -1.0f, value );
			return value;
		};

		Vector4 clip;
		for( int i = 0; i < 8; ++i )
		{
			clip = GetPoint( i ) * VP;
			if( clip[3] < 0.0001 )
			{
				continue;
			}
			Vector3 p = Vector3( clip.x / clip.w, clip.y / clip.w, clip.z / clip.w );
			minX = min( minX, p.x );
			minY = min( minY, p.y );
			maxX = max( maxX, p.x );
			maxY = max( maxY, p.y );
		}

		minX = Constrain( minX );
		maxX = Constrain( maxX );
		minY = Constrain( minY );
		maxY = Constrain( maxY );

		if( minX >= maxX || minY >= maxY )
		{
			return;
		}

		float scaleX = 2.0f / ( maxX - minX );
		float scaleY = 2.0f / ( maxY - minY );

		float biasX = ( minX + maxX ) / ( minX - maxX );
		float biasY = ( minY + maxY ) / ( minY - maxY );


		Matrix custom = Matrix( scaleX, 0, 0, 0, 0, scaleY, 0, 0, 0, 0, 1, 0, biasX, biasY, 0, 1 );

		custom = projection.GetTransform() * custom;

		projection.CustomProjection( custom );
	}
}

void Tr2InteriorScene::SetVisualizationMode( int visualizationMode )
{
	m_visualizeMethod = VisualizeMethod( visualizationMode );
}

void Tr2InteriorScene::Update( Be::Time realTime, Be::Time simTime )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( simTime == m_lastUpdateTime )
		// NB: Multiple calls on the same 'frame' should be ignored,
		//     but are not strictly errors, according to Dan Speed.
		return;
	m_lastUpdateTime = simTime;

	Vector3 dir( XMVectorMultiply(
		XMVector3Normalize( m_sunDirection ),
		XMVectorReplicate( -1.0f ) ) );

	m_sunDirectionVar = dir;
	m_sunDiffuseColorVar = m_sunDiffuseColor;
	m_sunSpecularColorVar = m_sunSpecularColor;
	m_ambientColorVar = m_ambientColor;
	m_cameraPosVar = Tr2Renderer::GetViewPosition();

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
			 it != dynamicsToRemove.end();
			 ++it )
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
}

void Tr2InteriorScene::Render( Tr2RenderContext& renderContext )
{
	D3DPERF_EVENT( L"Tr2InteriorScene::Render" );

	// Do the full-forward render
	RenderFullForward( renderContext );

	// debug info
	RenderDebugInfo( renderContext );

	// Clear lights
	m_activeLightSet.Clear();
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
void Tr2InteriorScene::VisibilityQuery( Tr2VisibilityResults* results, Tr2RenderContext& renderContext )
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
		renderContext.m_esm.GetViewport() );

	//this is to assist in tracking down the crash that's been popping up in this function.
	//The bools are here just in case logging breaks during the crash
	const size_t dynamicsSize = m_dynamics.size();

	for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
		if( dynamicsSize != m_dynamics.size() )
		{
			CCP_LOGERR( "VisibilityQuery-m_dynamics changed size while we iterated over it! (before setlod)" );
		}
		if( !*it )
		{
			CCP_LOGERR( "VisibilityQuery-null pointer in m_dynamics" );
		}
		( *it )->SetLOD( &frustum );
		if( dynamicsSize != m_dynamics.size() )
		{
			CCP_LOGERR( "VisibilityQuery-m_dynamics changed size while we iterated over it! (after setlod)" );
		}
	}

	m_visibilityQueryType = PRIMARY_QUERY;

	ResolveVisibility( Tr2Renderer::GetViewTransform(), Tr2Renderer::GetProjectionTransform(), true );
}

void Tr2InteriorScene::ResolveVisibility( const Matrix& view, const Matrix& projection, size_t maxDepth )
{
	CTriViewport viewport;
	TriFrustum frustum;
	XMVECTOR det;
	Matrix viewInv( XMMatrixInverse( &det, view ) );
	frustum.DeriveFrustum( &view, (Vector3*)( &viewInv._41 ), &projection, viewport );

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

	renderContext.AddGpuMarker( __FUNCTION__ );

	// If we don't have a visibilityResults object (i.e. the VisibilityQuery renderstep
	// wasn't called), then create one and call VisibilityQuery now.
	if( !m_visibilityResults )
	{
		m_visibilityResults.CreateInstance();
	}

	// Update variable store
	m_backgroundCubeMapVar = m_backgroundCubeMapRes;

	// Render Shadows
	RenderShadows( renderContext );

	VisibilityQuery( m_visibilityResults, renderContext );

	// Gather geometry batches
	GatherFullForwardBatches( m_visibilityResults );

	{
		// We need to adjust sorting of transparent batches to match the legacy sorting used during Incarna times.
		// The order of transparent batches for each individual object/character needs to be the reverse of the
		// order they were commited.
		auto& batches = m_primaryRenderBatches->GetBatches();
		for( size_t start = 0; start < batches.size(); )
		{
			auto& batch1 = batches[start];
			if( batch1.m_renderingMode == Tr2EffectStateManager::RM_ALPHA )
			{
				size_t end = start + 1;
				while( end < batches.size() && batch1.m_depth == batches[end].m_depth )
				{
					++end;
				}
				std::reverse( &batch1, batches.data() + end );
				start = end;
			}
			else
			{
				++start;
			}
		}
	}

	// set per-frame data
	PopulatePerFramePSData( m_perFramePSData, renderContext );
	PopulatePerFrameVSData( m_perFrameVSData );

	{
		D3DPERF_EVENT( L"Set per-frame shader constants" );
		FillAndSetConstants( m_perFrameVSBuffer, m_perFrameVSData, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
		FillAndSetConstants( m_perFramePSBuffer, m_perFramePSData, PIXEL_SHADER, Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
	}

	if( m_renderBackgroundCubeMap && m_backgroundEffect )
	{
		Tr2Renderer::DrawCameraSpaceScreenQuad( renderContext, m_backgroundEffect->GetShaderStateInterface(), m_backgroundEffect );
	}

	// Render geometry
	RenderGeometry( nullptr, renderContext );

	// Clear batches
	m_primaryRenderBatches->Clear();
}

void Tr2InteriorScene::SetupShadowMaps()
{
	m_lightRenderTargets.Clear();

	for( int i = 0; i < (int)( m_lights.size() ) && i < m_shadowCount; i++ )
	{
		Tr2RenderTargetPtr rt;
		rt.CreateInstance();
		rt->SetName( "ShadowMap" );
		rt->Create( m_shadowSize, m_shadowSize, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 1, 0 );
		m_lightRenderTargets.Append( rt );
	}

	// Setup depth stencil texture
	m_lightDepthStencil = Tr2DepthStencilPtr();
	m_lightDepthStencil.CreateInstance();
	m_lightDepthStencil->Create( m_shadowSize, m_shadowSize, Tr2RenderContextEnum::DSFMT_D32, 0, 0 );
}

// --------------------------------------------------------------------------------------
// Description
//   This function renders shadow maps for each shadow casting light source.
// --------------------------------------------------------------------------------------
void Tr2InteriorScene::RenderShadows( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_renderShadows )
	{
		return;
	}

	if( !m_lightDepthStencil )
	{
		SetupShadowMaps();
	}

	m_noiseTextureHandle = GlobalStore().RegisterVariable( "NoiseMap", m_noiseTexture );

	Tr2Renderer::PushProjection();
	ON_BLOCK_EXIT( Tr2Renderer::PopProjection );
	Tr2Renderer::PushViewTransform();
	ON_BLOCK_EXIT( Tr2Renderer::PopViewTransform );

	renderContext.m_esm.PushViewport();
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.PopViewport(); } );
	renderContext.m_esm.PushRenderTarget();
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.PopRenderTarget(); } );
	renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.PopDepthStencilBuffer(); } );

	TriViewPtr lightView = CreateInstance<TriView>();
	TriProjectionPtr lightProjection = CreateInstance<TriProjection>();

	// This is the camera's frustum, used for calculating tight bounds for shadow maps
	TriFrustum cameraFrustum;
	cameraFrustum.DeriveFrustum(
		&Tr2Renderer::GetViewTransform(),
		&Tr2Renderer::GetViewPosition(),
		&Tr2Renderer::GetProjectionTransform(),
		renderContext.m_esm.GetViewport() );

	// Reset the testures to white
	m_shadowMap0Var = m_whiteTexture;
	m_shadowMap1Var = m_whiteTexture;
	m_shadowMap2Var = m_whiteTexture;
	m_shadowMap3Var = m_whiteTexture;

	for( int i = 0; i < (int)( m_lights.size() ) && i < m_shadowCount; ++i )
	{
		if( i >= (int)( m_lightRenderTargets.size() ) )
		{
			// render target might not be created yet.
			return;
		}

		Tr2InteriorLightSource* light = dynamic_cast<Tr2InteriorLightSource*>( m_lights[i] );

		if( light->m_coneAlphaOuter >= 90 )
		{
			continue;
		}

		float fov = light->m_coneAlphaOuter * TRI_PI / 90.0f;
		lightProjection->PerspectiveFov( fov, 1.0, 0.1, light->m_radius );
		lightView->SetLookAtPosition( light->m_position, light->m_position + light->m_coneDirection, Vector3( 0.0, 0.0, 1.0 ) );

		// Fit the light's projection matrix to the character's frustum area to maximize shadowmap usage.
		if( m_optimizeShadows )
		{
			MaximizeShadowMapUsage( *lightProjection, lightView, &cameraFrustum );
		}

		auto rt = m_lightRenderTargets[i];
		renderContext.m_esm.SetRenderTarget( 0, rt->GetRenderTarget() );
		renderContext.m_esm.SetDepthStencilBuffer( *m_lightDepthStencil );
		renderContext.Clear( CLEARFLAGS_TARGET | CLEARFLAGS_ZBUFFER, 0xffffffff, 1.f, 0, 0 );

		Tr2Renderer::SetProjectionTransform( lightProjection->GetTransform() );
		Tr2Renderer::SetViewTransform( lightView->GetTransform() );

		// Gather visibility results
		VisibilityQuery( m_visibilityResults, renderContext );
		// Gather geometry batches
		GatherFullForwardBatches( m_visibilityResults );

		// Set per frame data
		PopulatePerFramePSData( m_perFramePSData, renderContext );
		PopulatePerFrameVSData( m_perFrameVSData );

		// Plug in the light's radius
		m_perFramePSData.radius = light->m_radius;

		m_perFrameVSData.ViewMat = Transpose( lightView->GetTransform() );
		m_perFrameVSData.ProjectionMat = Transpose( lightProjection->GetTransform() );
		Matrix VP = XMMatrixMultiply( lightView->GetTransform(), lightProjection->GetTransform() );
		m_perFrameVSData.ViewProjectionMat = Transpose( VP );

		Matrix uvAdjust = Matrix(
			0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f );

		// Set the VP matrix on the light and [-1, 1] -> [0, 1]
		light->m_viewProjection = Transpose( VP * uvAdjust );

		{
			D3DPERF_EVENT( L"Set per-frame shader constants for Shadows" );
			FillAndSetConstants( m_perFrameVSBuffer, m_perFrameVSData, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
			FillAndSetConstants( m_perFramePSBuffer, m_perFramePSData, PIXEL_SHADER, Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
		}

		{
			D3DPERF_EVENT( L"Primary render batches for Shadows" );
			m_primaryRenderBatches->Finalize();
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
			renderContext.RenderBatches( m_primaryRenderBatches, BlueSharedString( "Shadow" ) );
		}

		if( i == 0 )
			m_shadowMap0Var = rt;
		if( i == 1 )
			m_shadowMap1Var = rt;
		if( i == 2 )
			m_shadowMap2Var = rt;
		if( i == 3 )
			m_shadowMap3Var = rt;

		// Clear batches
		m_primaryRenderBatches->Clear();
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
		renderContext.RenderBatchesWithOverride( m_primaryRenderBatches, overrideEffect );
	}

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
	renderContext.m_esm.EndManagedRendering();
}

void Tr2InteriorScene::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	if( !m_debugRenderer )
	{
		if( m_debugRenderShadowMaps )
		{
			// Need to create a debug renderer
			m_debugRenderer = Tr2DebugRendererPtr();
			m_debugRenderer.CreateInstance();
		}
		else
		{
			return;
		}
	}

	// Display the debug meshes for the lights
	Tr2PerFrameVSDataDebug debugPerFrameData;

	debugPerFrameData.ViewProjectionMat = XMMatrixTranspose(
		XMMatrixMultiply( Tr2Renderer::GetViewTransform(), Tr2Renderer::GetProjectionTransform() ) );

	// attention: need the transposed, but shader also needs column_major, so it is transpose(transpose(m)) == m
	debugPerFrameData.ViewInverseTransposeMat = Tr2Renderer::GetInverseViewTransform();

	FillAndSetConstants( m_perFrameVSBuffer, debugPerFrameData, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );

	m_debugRenderer->BeginRender();
	auto bkProjection = Tr2Renderer::GetProjectionRawTransform();

	Tr2Renderer::SetProjectionTransform( Tr2Renderer::GetProjectionTransform() );
	ON_BLOCK_EXIT( [&] { Tr2Renderer::SetProjectionTransform( bkProjection ); } );

	for( auto it = m_lights.begin(); it != m_lights.end(); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->RenderDebugInfo( *m_debugRenderer );
		}
	}
	for( auto it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->RenderDebugInfo( *m_debugRenderer );
		}
	}
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
	RenderDebugInfo( *m_debugRenderer );

	m_debugRenderer->EndRender( renderContext );
	Tr2Renderer::RenderDebugInfo( renderContext );
}

ITr2MultiPassScene::RenderPassResult Tr2InteriorScene::RenderPass( PassType pass, Tr2RenderContext& renderContext )
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

	SetupShadowMaps();
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
		CCP_LOGERR( "Tr2InteriorScenel::RemoveLightSource() - interiorLightSource not found in the scene!" );
		return;
	}

	m_lights.Remove( pos );

	SetupShadowMaps();
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
		CCP_LOGERR( " Attempt to add NULL dynamic to interior scene!" );
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
#define DO_GET_BATCHES( batches, mode, key, batchType )       \
	batches->SetRenderingMode( Tr2EffectStateManager::mode ); \
	batches->SetUserData( ConstructKey( 0, key ) );           \
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
			 it != events.end();
			 ++it )
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
		fx *= ( Tr2Renderer::GetOrthoWidth() / 2.0f );
		fy *= ( Tr2Renderer::GetOrthoHeight() / 2.0f );
		float metersPerPixel = ( Tr2Renderer::GetOrthoWidth() / viewport->width ) / 2.0f;
		Tr2Renderer::SetOrthoProjection( fx - metersPerPixel,
										 fx + metersPerPixel,
										 fy - metersPerPixel,
										 fy + metersPerPixel,
										 Tr2Renderer::GetFrontClip(),
										 Tr2Renderer::GetBackClip() );
	}
	else
	{
		//
		// Projection is set up to scale the image such that the viewport is covered by one pixel.
		Vector2 scaling( float( viewport->width ), float( viewport->height ) );
		// translate the projection so that we center around the pick ray origin,
		// while remembering to scale this value as well:
		Vector2 translation;
		translation.x = -fx * scaling.x;
		translation.y = -fy * scaling.y;
		Tr2Renderer::AdjustProjection( scaling, translation );
	}
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
	Matrix proj = PerspectiveFovMatrix( fov, aspect, frustum.m_zNear, frustum.m_zFar );

	// Issue the query
	m_visibleObjects.clear();
	m_visibilityQueryType = PICKING_QUERY;
	ResolveVisibility( newView, proj, false );

	return m_visibleObjects;
}

void Tr2InteriorScene::SetPerFrameDataForPicking( Tr2RenderContext& renderContext )
{
	Tr2PerFramePSData perFramePS;
	Tr2PerFrameVSData perFrameVS;

	PopulatePerFramePSData( perFramePS, renderContext );
	PopulatePerFrameVSData( perFrameVS );

	Tr2BindPerFramePSData( perFramePS, renderContext );
	Tr2BindPerFrameVSData( perFrameVS, renderContext );
}

int64_t Tr2InteriorScene::ConstructKey( unsigned int objectGroup, Tr2InteriorBatchGroup batchGroup )
{
	int64_t objGroup64 = (int64_t)objectGroup << 48;

	int64_t batchGroup64 = (int64_t)batchGroup << 32;

	return ( objGroup64 | batchGroup64 );
}

void Tr2InteriorScene::PopulatePerFrameVSData( Tr2PerFrameVSData& data )
{
	Tr2PopulatePerFrameVSDataTransformations( data );

	// sun
	Vector3 vec;
	GetSunDirWorldHandle()->GetValue( vec );
	data.sunDirWorld.x = vec.x;
	data.sunDirWorld.y = vec.y;
	data.sunDirWorld.z = vec.z;
}

void Tr2InteriorScene::PopulatePerFramePSData( Tr2PerFramePSData& data, Tr2RenderContext& renderContext )
{
	memset( &data, 0, sizeof( Tr2PerFramePSData ) );

	// column_major for shaders
	data.ViewProjectionMat = XMMatrixTranspose(
		XMMatrixMultiply(
			Tr2Renderer::GetViewTransform(),
			Tr2Renderer::GetProjectionTransform() ) );
	// attention: need the transposed, but shader also needs column_major, so it is transpose(transpose(m)) == m
	data.ViewInverseTransposeMat = Tr2Renderer::GetInverseViewTransform();

	unsigned int width = renderContext.m_esm.GetRenderTargetWidth();
	unsigned int height = renderContext.m_esm.GetRenderTargetHeight();

	data.viewPort.x = float( renderContext.m_esm.GetViewport().width ) / float( width );
	data.viewPort.y = float( renderContext.m_esm.GetViewport().height ) / float( height );
	data.viewPort.z = float( renderContext.m_esm.GetViewport().x ) / float( width ) + 0.5f / float( width );
	data.viewPort.w = float( renderContext.m_esm.GetViewport().y ) / float( height ) + 0.5f / float( height );

	XMVECTOR det;
	data.ViewProjInverse = XMMatrixTranspose(
		XMMatrixInverse( &det, XMMatrixMultiply( Tr2Renderer::GetViewTransform(), Tr2Renderer::GetProjectionTransform() ) ) );

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
	data.shadowCount = (float)m_shadowCount;
	data.invShadowSize = 1.0f / m_shadowSize;

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
	float a = *( (float*)pBuffer + 3 );
	float r = *( (float*)pBuffer + 2 );
	float g = *( (float*)pBuffer + 1 );
	float b = *( (float*)pBuffer + 0 );
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

void Tr2InteriorScene::UpdateSceneFromScript( Be::Time time )
{
	Update( time, time );
}

void Tr2InteriorScene::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Shadow Maps" );
}

void Tr2InteriorScene::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( GetRawRoot(), "Shadow Maps" ) || m_debugRenderShadowMaps )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		renderContext.m_esm.PushViewport();
		int i = 0;
		for( auto it = m_lightRenderTargets.begin(); it != m_lightRenderTargets.end(); ++it )
		{
			renderContext.m_esm.SetViewport( 200, 200, 10, 10 + i * 210, 0.0, 1.0 );
			auto tex = dynamic_cast<Tr2RenderTarget*>( *it )->GetTexture();
			Tr2Renderer::DrawTexture( renderContext, *tex );
			i++;
		}
		renderContext.m_esm.PopViewport();
	}
}
