#include "StdAfx.h"
#include "EveSpaceScene.h"
#include "IEveShadowCaster.h"
#include "TriDevice.h"
#include "Tr2Renderer.h"
#include "Tr2PushPopDS.h"
#include "Tr2PushPopRT.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2EffectStateManager.h"
#include "EveLensflare.h"
#include "Tr2VariableStore.h"
#include "TriSettingsRegistrar.h"
#include "TriShadowMap.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"
#include "TriFrustum.h"
#include "TriFrustumOrtho.h"
#include "TriProjection.h"
#include "TriView.h"
#include "EveCamera.h"
#include "Particle/Tr2ParticleSystem.h"
#include "Tr2RenderTarget.h"
#include "Tr2DepthStencil.h"
#include "EveTransform.h"
#include "TbbStub.h"
#include "Include/TriMath.h"
#include "EveDistanceField.h"
#include "Renderable/EveSceneStaticParticles.h"
#include "Shader/Utils/Tr2DataTextureManager.h"
#include "Shader/Tr2ShaderBuffer.h"
#include "Particle/Tr2GpuParticleSystem.h"
#include "Resources/TriTextureRes.h"
#include "Tr2ImpostorManager.h"
#include "Tr2DebugRenderer.h"
#include "EveEffectRoot2.h"

using namespace Tr2RenderContextEnum;

class TriPoolAllocator;

CCP_STATS_DECLARE( shadowsRendered, "Trinity/EveSpaceScene/shadowsRendered", true, CST_COUNTER_LOW, "How many times are shadows rendered per frame?" );
CCP_STATS_DECLARE( shLightingUpdateTime, "Trinity/EveSpaceScene/shLightingUpdateTime", true, CST_TIME, "Time took to update SH lighting for EveSpaceScene" );
CCP_STATS_DECLARE( gatherDynamicLights, "Trinity/EveSpaceScene/gatherDynamicLights", true, CST_TIME, "Time took to gather dynamic lights for EveSpaceScene" );
CCP_STATS_DECLARE( updateDynamicLightLists, "Trinity/EveSpaceScene/updateDynamicLights", true, CST_TIME, "Time took to gather dynamic lights for EveSpaceScene" );

bool g_eveIsSpaceObjectResourceUnloadingEnabled = true;
TRI_REGISTER_SETTING( "eveIsSpaceObjectResourceUnloadingEnabled", g_eveIsSpaceObjectResourceUnloadingEnabled );

float g_eveSpaceObjectResourceUnloadingTimeThreshold = 15.0f;
TRI_REGISTER_SETTING( "eveSpaceObjectResourceUnloadingTimeThreshold", g_eveSpaceObjectResourceUnloadingTimeThreshold );

// Object itself only renders if estimated pixel diameter is above
// this threshold. Note that attachments may still render, in particular
// turret firing effects, or light glows as they may be more noticeable from afar.
float g_eveSpaceSceneVisibilityThreshold = 5.0f;
TRI_REGISTER_SETTING( "eveSpaceSceneVisibilityThreshold", g_eveSpaceSceneVisibilityThreshold );

// Object itself renders with low detail geometry (if available) if estimated pixel 
// diameter is above this threshold. Note that attachments may still render, in particular
// turret firing effects, or light glows as they may be more noticeable from afar.
float g_eveSpaceSceneLowDetailThreshold = 100.0f;
float g_eveSpaceSceneMediumDetailThreshold = 400.0f;
float g_eveSpaceSceneHighDetailThreshold = 800.0f;
float g_eveSpaceSceneLODFactor = 1.0f;

TRI_REGISTER_SETTING( "eveSpaceSceneLowDetailThreshold", g_eveSpaceSceneLowDetailThreshold );
TRI_REGISTER_SETTING( "eveSpaceSceneMediumDetailThreshold", g_eveSpaceSceneMediumDetailThreshold );
TRI_REGISTER_SETTING( "eveSpaceSceneHighDetailThreshold", g_eveSpaceSceneHighDetailThreshold );
TRI_REGISTER_SETTING( "eveSpaceSceneLODFactor", g_eveSpaceSceneLODFactor );


// These variables determine how frequently curve sets are updated for objects with low and medium LODs.
float g_eveSpaceSceneLowUpdateRate = 1.0f;
float g_eveSpaceSceneMediumUpdateRate = 0.1f;

TRI_REGISTER_SETTING( "eveSpaceSceneLowUpdateRate", g_eveSpaceSceneLowUpdateRate );
TRI_REGISTER_SETTING( "eveSpaceSceneMediumUpdateRate", g_eveSpaceSceneMediumUpdateRate );

// This is the global gamma brightness
float g_eveSpaceSceneGammaBrightness = 1.f;
TRI_REGISTER_SETTING( "eveSpaceSceneGammaBrightness", g_eveSpaceSceneGammaBrightness );

// enabled impact effects
bool g_eveSpaceObjectImpactEffectEnabled = true;
TRI_REGISTER_SETTING( "eveSpaceObjectImpactEffectEnabled", g_eveSpaceObjectImpactEffectEnabled );


bool g_eveSpaceSceneDynamicLighting = false;
TRI_REGISTER_SETTING( "eveSpaceSceneDynamicLighting", g_eveSpaceSceneDynamicLighting );

namespace
{
	const char* VISUALIZER_EFFECT_PATH[EveSpaceScene::VM_COUNT] =
	{
		"",
		"res:/Graphics/Effect/Managed/Space/Visualizer/Texcoord0.fx",
		"res:/Graphics/Effect/Managed/Space/Visualizer/Texcoord1.fx",
		"res:/Graphics/Effect/Managed/Space/Visualizer/White.fx",
		"res:/Graphics/Effect/Managed/Space/Visualizer/Overdraw.fx",
		"res:/Graphics/Effect/Managed/Space/Visualizer/Wireframe.fx",
		"res:/Graphics/Effect/Managed/Space/Visualizer/LightCount.fx",
	};
}

struct ShadowReceiver
{
	float estimatedSize;
	IEveSpaceObject2* object;

	ShadowReceiver( float es, IEveSpaceObject2* obj ) : estimatedSize( es ), object( obj ) {}
	bool operator<( const ShadowReceiver& other ) const
	{
		return estimatedSize < other.estimatedSize;
	}
};

EveSpaceScene::EveSpaceScene( IRoot* lockobj ) :
	PARENTLOCK( m_backgroundObjects ),
	PARENTLOCK( m_planets ),
	PARENTLOCK( m_objects ),
	PARENTLOCK( m_uiObjects ),
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_lensflares ),
	PARENTLOCK( m_distanceFields ),
	PARENTLOCK( m_staticParticles ),
	PARENTLOCK( m_externalParameters ),
	m_display( true ),
	m_update( true ),
	m_enableShadows( true ),
	m_selfShadowOnly( false ),
	m_displayShadowMap( false ),
	m_displayShadowMapMipLevel( 0 ),
	m_shadowThreshold( 120.0f ),
	m_shadowFadeThreshold( 200.0f ),
	m_shadowReceiverMaxCount( 16 ),
	m_shadowCasterMaxCount( 16 ),
	m_visualizeMethod( VM_NONE ),
	m_perFrameDebug( 0.f ),
	m_pickBuffer( NULL,  Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 1 ),
	m_envMapRotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_backgroundRenderingEnabled( false ),
	m_debugShowShadowCasters( false ),
	m_enableShadowObb( true ),
	m_enableShadowDistanceTweak( true ),
	m_shadowCameraDistance( 50.0f ),
	m_updateContext( NULL ),
	m_envMapHandle( NULL ),
	m_envMap1Var( "EnvMap1", m_envMap1 ),
	m_envMap2Var( "EnvMap2", m_envMap2 ),
	m_reflectionMapVar( "ReflectionMap", m_envMap1 ),
	m_reflectionMaskMapVar( "ReflectionMaskMap", m_envMap2 ),
	m_depthMapVar( "DepthMap", (ITr2TextureProvider*)nullptr ),
	m_depthMapMsaaVar( "DepthMapMsaa", (ITr2TextureProvider*)nullptr ),
	m_envMapTransformVar( "EnvMapTransform", IdentityMatrix() ),
	m_reflectionMapTransformVar( "ReflectionMapTransform", IdentityMatrix() ),
	m_suncVecVar( "SunVec", Vector3( 0.0f, 0.0f, 1.0f )),
	m_shadowLightnessVar( "ShadowLightness", 0.0f ),
	m_nebulaIntensity( 1.f ),
	m_planetScale( 1e6 ),
	m_planetCameraScale( 1e6 ),
	m_taaPixelOffsetScale( 0.5f ),
	m_taaSamplingIndex( 0 ),
	m_taaPattern( TAA_NONE ),
	m_sunColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_sunColorWithDynamicLights( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_useSunColorWithDynamicLights( false ),
	m_nebulaBrightnessOverride( 0.f ),
	m_nebulaBrightnessOverrideVar( "NebulaBrightnessOverride", m_nebulaBrightnessOverride ),
	m_hasDepthPass( false ),
	m_msaaSamples( 0 )
{
	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	m_primaryBatches[TRIBATCHTYPE_OPAQUE] = CCP_NEW( "EveSpaceScene/m_batches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_primaryBatches[TRIBATCHTYPE_DECAL] = CCP_NEW( "EveSpaceScene/m_decalBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_primaryBatches[TRIBATCHTYPE_ADDITIVE] = CCP_NEW( "EveSpaceScene/m_additiveBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_primaryBatches[TRIBATCHTYPE_DISTORTION] = CCP_NEW( "EveSpaceScene/m_distortionBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_primaryBatches[TRIBATCHTYPE_TRANSPARENT] = CCP_NEW( "EveSpaceScene/m_sortedBatches" ) TriRenderBatchAccumulator<>( allocator );
	m_primaryBatches[TRIBATCHTYPE_DEPTH] = CCP_NEW( "EveSpaceScene/m_depthBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );

	m_secondaryBatches[TRIBATCHTYPE_OPAQUE] = CCP_NEW( "EveSpaceScene/m_batches2" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_secondaryBatches[TRIBATCHTYPE_DECAL] = CCP_NEW( "EveSpaceScene/m_decalBatches2" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_secondaryBatches[TRIBATCHTYPE_ADDITIVE] = CCP_NEW( "EveSpaceScene/m_additiveBatches2" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_secondaryBatches[TRIBATCHTYPE_DISTORTION] = CCP_NEW( "EveSpaceScene/m_distortionBatches2" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_secondaryBatches[TRIBATCHTYPE_TRANSPARENT] = CCP_NEW( "EveSpaceScene/m_sortedBatches2" ) TriRenderBatchAccumulator<>( allocator );
	m_secondaryBatches[TRIBATCHTYPE_DEPTH] = CCP_NEW( "EveSpaceScene/m_depthBatches2" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );

	m_shadowBatches = CCP_NEW( "EveSpaceScene/m_shadowBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );

	// global textures
	m_envMapHandle = GlobalStore().RegisterVariable( "EveSpaceSceneEnvMap", (TriTextureRes*)nullptr );
	
	// Picking batches
	m_pickingBatches = CCP_NEW( "EveSpaceScene/m_pickingBatches" ) TriRenderBatchAccumulator<>( allocator );
		
	m_sunData.DiffuseColor = Color( 1.0f, 1.0f, 1.0f, 1.0f );
	m_sunData.DirWorld = Vector3( 0.0f, -1.0f, 0.0f );
	m_sunData.unused_pad0 = 0.0;

	m_ambientColor = Color( 0.25f, 0.25f, 0.25f, 1.0f );
	m_fogColor = Color( 0.25f, 0.25f, 0.25f, 1.0f );
	m_fogEnd = m_fogStart = m_fogMax = 0.0f;

	m_pickBuffer.PrepareResources();

	m_updateTime = BeOS->GetCurrentFrameTime();

	m_planets.SetNotify( this );
	m_objects.SetNotify( this );
	m_uiObjects.SetNotify( this );
	
	m_dataTextureMgr.CreateInstance();
	m_postProcessPSBuffer.CreateInstance();
	// 2x sampling pattern
	m_taaSamplingPatterns[0] = Vector2( .5f, -.5f );
	m_taaSamplingPatterns[1] = Vector2( -.5f, .5f );
	// 4x sampling pattern
	m_taaSamplingPatterns[2] = Vector2( .25f, -.75f );
	m_taaSamplingPatterns[3] = Vector2( -.25f, .75f );
	m_taaSamplingPatterns[4] = Vector2( .75f, .25f );
	m_taaSamplingPatterns[5] = Vector2( -.75f, -.25f );
	// 3x sampling pattern
	m_taaSamplingPatterns[6] = Vector2( -.67f, -.1f );
	m_taaSamplingPatterns[7] = Vector2( .1f, .67f );
	m_taaSamplingPatterns[8] = Vector2( .67f, -.67f );

	m_visualizerEffects[VW_LIGHT_COUNT].type = VisualizerEffect::FULL_SCREEN_QUAD_OVERLAY;

	Tr2LightManager::ResetVariableStore();

	m_cameraAttachmentParent.CreateInstance();
}

IRoot* EveSpaceScene::GetCameraAttachments() const
{
	return m_cameraAttachmentParent->GetChildren().GetRawRoot();
}

EveSpaceScene::~EveSpaceScene()
{
	SetShLightingManager( nullptr );
	for( auto it = m_primaryBatches.begin(); it != m_primaryBatches.end(); ++it )
	{
		CCP_DELETE( it->second );
	}
	for( auto it = m_secondaryBatches.begin(); it != m_secondaryBatches.end(); ++it )
	{
		CCP_DELETE( it->second );
	}
}


void EveSpaceScene::ReleaseResources( TriStorage s )
{
	// handles
	if( m_envMapHandle )
	{
		m_envMapHandle->Clear();
	}

	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		m_perFrameVSBuffer.Destroy();
		m_perFramePSBuffer.Destroy();
		m_shadowPerFrameVSBuffer.Destroy();
	}
}

Tr2ShaderBufferPtr EveSpaceScene::GetPostProcessPSBuffer( )
{
	return m_postProcessPSBuffer;
}

bool EveSpaceScene::OnPrepareResources()
{
	return true;
}

void EveSpaceScene::Update( Be::Time realTime, Be::Time simTime )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_update )
	{
		return;
	}

	m_updateContext.SetTime( simTime );
	m_updateContext.UpdateOrigin( m_ballpark );
	m_updateContext.SetDataTextureManager( m_dataTextureMgr );

	{
		for( auto it = m_backgroundObjects.begin(); it != m_backgroundObjects.end(); ++it )
		{
			( *it )->UpdateSyncronous( m_updateContext );
		}
		for( auto it = m_backgroundObjects.begin(); it != m_backgroundObjects.end(); ++it )
		{
			( *it )->UpdateAsyncronous( m_updateContext );
		}
	}

	if( m_warpTunnel )
	{
		m_warpTunnel->UpdateSyncronous( m_updateContext );
		m_warpTunnel->UpdateAsyncronous( m_updateContext );
	}

	if( m_starfield )
	{
		m_starfield->Update( simTime );
	}

	for( auto it = m_staticParticles.begin(); it != m_staticParticles.end(); ++it )
	{
		(*it)->Update( m_updateContext );
	}
	
	if( m_dataTextureMgr )
	{
		m_dataTextureMgr->Update( m_updateContext );
	}
	
	for( auto it = m_distanceFields.begin(); it != m_distanceFields.end(); ++it )
	{
		(*it)->Update( m_updateContext );
	}

	// Update planets
	UpdatePlanets( m_updateContext );

	// Update lensflares
	for( EveLensflareVector::const_iterator it = m_lensflares.begin(); it != m_lensflares.end(); ++it )
	{
		(*it)->Update( realTime, simTime );
	}

	// Update all space objects
	for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		(*it)->Update( realTime, simTime );
	}

	{
		CCP_STATS_ZONE( "UpdateSyncronous" );

		for( IEveSpaceObject2Vector::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it )
		{
			(*it)->UpdateSyncronous( m_updateContext );
		}

		for( IEveSpaceObject2Vector::const_iterator it = m_uiObjects.begin(); it != m_uiObjects.end(); ++it )
		{
			(*it)->UpdateSyncronous( m_updateContext );
		}
	}
	{
		CCP_STATS_ZONE( "UpdateAsyncronous" );

		Tr2ParallelDo( m_objects.begin(), m_objects.end(), [&]( IEveSpaceObject2* obj ) { obj->UpdateAsyncronous( m_updateContext ); } );
		Tr2ParallelDo( m_uiObjects.begin(), m_uiObjects.end(), [&]( IEveSpaceObject2* obj ) { obj->UpdateAsyncronous( m_updateContext ); } );
	}

	// Update the sun direction from the ball
	// Since the egoBall is the center of the universe and is the player ship,
	// the direction of the sunlight is actually just the normalized position of the sun
	if( m_sunBall )
	{
		Vector3 sunDirection;

		m_sunBall->Update( &sunDirection, simTime );
		sunDirection = Normalize( sunDirection );
		m_sunData.DirWorld = -sunDirection;
	}

	// every space scene has a reference position
	Vector3d sceneReferencePoint = m_updateContext.GetOrigin();

	m_updateTime = simTime;
}

static bool GetShadowReceiverBounds( 
	IEveSpaceObject2* objectOfInterest,
	bool enableShadowObb,
	Vector4& boundingSphere,
	Obb& obb )
{
	if( !objectOfInterest->GetBoundingSphere( boundingSphere ) )
	{
		return false;
	}

	Vector3 minBounds, maxBounds;
	if( !enableShadowObb || !objectOfInterest->GetLocalBoundingBox( minBounds, maxBounds ) )
	{
		// convert the sphere to AABB
		BoundingBoxInitialize( boundingSphere, minBounds, maxBounds );
	}
	else
	{
		Vector3 size = maxBounds - minBounds;
		minBounds.x -= std::abs( size.x ) * 0.01f;
		minBounds.y -= std::abs( size.y ) * 0.01f;
		minBounds.z -= std::abs( size.z ) * 0.01f;
		maxBounds.x += std::abs( size.x ) * 0.01f;
		maxBounds.y += std::abs( size.y ) * 0.01f;
		maxBounds.z += std::abs( size.z ) * 0.01f;
	}
	Matrix localToWorld;
	objectOfInterest->GetLocalToWorldTransform( localToWorld );
	// passing viewFrustum works fine, but little benefit at expense of wobbly shadows. TODO revisit once better filtering is in place.
	obb.CreateClippedWorldBoundingObb( minBounds, maxBounds, localToWorld, /* &viewFrustum*/ NULL );
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Fills provided vector with non-NULL shadow casters from m_objects list.
// Arguments:
//   allShadowCasters - (out) list of all shadow casters in the scene
// --------------------------------------------------------------------------------------
void EveSpaceScene::GetAllShadowCasters( std::vector<IEveShadowCaster*>& allShadowCasters )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	allShadowCasters.reserve( m_objects.size() );
	for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		if( IEveShadowCasterPtr caster = BlueCastPtr( *it ) )
		{
			allShadowCasters.push_back( caster );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Collects renderables of shadow casters for the specified shadow receiver. Can be
//   called on separate threads.
// Arguments:
//   objectOfInterest - shadow receiver
//   objectOfInterestShadowCaster - shadow receiver as a shadow caster (for self-shadow)
//   allShadowCasters - list of all shadow casters in the scene
//   shadowRenderables - (out) list of renerables of shadow casters for objectOfInterest
//   debugShadowCasters - (out) list of shadow casters for objectOfInterest (used for
//     debug rendering)
// --------------------------------------------------------------------------------------
void EveSpaceScene::GetShadowCasterRenderables( 
	IEveSpaceObject2* objectOfInterest,
	IEveShadowCaster* objectOfInterestShadowCaster,
	const std::vector<IEveShadowCaster*>& allShadowCasters,
	std::vector<ITr2Renderable*>& shadowRenderables,
	std::vector<IEveShadowCaster*>& debugShadowCasters )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Vector4 boundingSphere;
	Obb obb;

	if( !GetShadowReceiverBounds( objectOfInterest, m_enableShadowObb, boundingSphere, obb ) )
	{
		CCP_LOGWARN( "EveSpaceScene::GetShadowCasterRenderables failed in GetShadowReceiverBounds" );
		return;
	}

	TriFrustumOrtho shadowFrustum;
	TriShadowMap::CalculateShadowFrustum( 
		m_sunData.DirWorld,
		boundingSphere.w * m_shadowCameraDistance,
		obb,
		shadowFrustum );

	// Always do self shadowing
	objectOfInterestShadowCaster->GetRenderablesCastingShadow( true, shadowFrustum, shadowRenderables );

	if( !m_selfShadowOnly )
	{
		unsigned int shadowCasterCount = 0;

		for( auto it = allShadowCasters.begin(); it != allShadowCasters.end(); ++it )
		{
			IEveShadowCaster* obj = *it;
			if( obj && obj != objectOfInterestShadowCaster )
			{
				if( obj->GetRenderablesCastingShadow( false, shadowFrustum, shadowRenderables ) )
				{
					if( m_debugShowShadowCasters )
					{
						debugShadowCasters.push_back( obj );
					}
					++shadowCasterCount;
					if( shadowCasterCount == m_shadowCasterMaxCount )
					{
						// Can't have more shadow casters in this shadow map
						break;
					}
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Prepares a shadow map for the given shadow receiver. Updates per-frame data with 
//   shadow information.
// Arguments:
//   objectOfInterest - shadow receiver
//   shadowRenderables - list of renerables of shadow casters
//   debugShadowCasters - list of shadow casters (used for debug rendering)
//   renderContext - render context
// --------------------------------------------------------------------------------------
void EveSpaceScene::PrepareShadowMap( 
	IEveSpaceObject2* objectOfInterest, 
	const std::vector<ITr2Renderable*>& shadowRenderables,
	const std::vector<IEveShadowCaster*>& debugShadowCasters,
	Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_shadowMap || !m_shadowMap->GetTexture().IsValid() )
	{
		return;
	}

	Vector4 boundingSphere;
	Obb obb;
	if( !GetShadowReceiverBounds( objectOfInterest, m_enableShadowObb, boundingSphere, obb ) )
	{
		CCP_LOGWARN( "EveSpaceScene::PrepareShadowMap failed in GetShadowReceiverBounds" );
		return;
	}

	for( auto it = shadowRenderables.begin(); it != shadowRenderables.end(); ++it )
	{
		ITr2Renderable* obj = *it;
		Tr2PerObjectData* objectData = obj->GetPerObjectData( m_shadowBatches );
		obj->GetShadowBatches( m_shadowBatches, objectData );
	}

	m_shadowBatches->Finalize();

	m_shadowMap->SetReceiverObb( obb );
	// Sun data stores direction towards light - shadow map wants direction of light
	m_shadowMap->SetLightDirection( m_sunData.DirWorld );
	m_shadowMap->SetLightDistance( boundingSphere.w * m_shadowCameraDistance );

	Matrix lightView;
	Matrix lightViewProj;
	Vector3 lightViewPosition;

	if( !m_shadowMap->BeginShadowRendering( lightViewPosition, lightView, lightViewProj, renderContext ) )
	{
		CCP_LOGWARN( "EveSpaceScene::PrepareShadowMap failed in BeginShadowRendering" );
		return;
	}

	if( m_debugShowShadowCasters )
	{
		// Draw a yellow line from the light view position to the receiver position
		Tr2Renderer::DrawLine( lightViewPosition, BoundingSphereGetCenter( boundingSphere ), 0xffffff00 );
	}

	if( m_debugShowShadowCasters )
	{
		Vector3 objectOfInterestPosition;
		objectOfInterest->GetModelCenterWorldPosition( objectOfInterestPosition );

		for( auto it = debugShadowCasters.begin(); it != debugShadowCasters.end(); ++it )
		{
			IEveSpaceObject2Ptr obj = BlueCastPtr( *it );
			// Draw a line from each shadow caster to receiver
			Vector3 casterPosition;
			obj->UpdateModelCenterWorldPosition( casterPosition, 0 );
			Tr2Renderer::DrawLine( casterPosition, 0xffffffff, objectOfInterestPosition, 0xffff0000 );
		}
	}

	ShadowPerFrameVSData data;
	// column_major for shaders
	data.ViewMat = Transpose( lightView );
	data.ViewProjectionMat = Transpose( lightViewProj );

	Vector3 lightMin, lightMax;
	m_shadowMap->GetReceiverLightAabb( lightMin, lightMax );
	Vector3 minBounds, maxBounds;
	m_shadowMap->GetBounds( minBounds, maxBounds );

	data.CameraRange.x = m_enableShadowDistanceTweak ? lightMax.z : maxBounds.z;
	data.CameraRange.y = m_enableShadowDistanceTweak ? lightMin.z : minBounds.z;
	
	static const unsigned perFrameVsMask = 
		( 1 << VERTEX_SHADER )					|
		SHADER_TYPE_EXISTS( COMPUTE_SHADER )	|
		SHADER_TYPE_EXISTS( GEOMETRY_SHADER )	|
		SHADER_TYPE_EXISTS( HULL_SHADER )		|
		SHADER_TYPE_EXISTS( DOMAIN_SHADER)		;
	FillAndSetConstants( m_shadowPerFrameVSBuffer, &data, sizeof( data ), perFrameVsMask, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );

	{
		renderContext.m_esm.SetInvertedDepthTest( false );
		ON_BLOCK_EXIT( [&] { renderContext.m_esm.SetInvertedDepthTest( true ); } );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
		renderContext.RenderBatches( m_shadowBatches );
	}
	m_shadowMap->EndShadowRendering();

	// column_major for shaders
	m_perFrameVS.ShadowViewMat = Transpose( lightView );
	m_perFrameVS.ShadowViewProjectionMat = Transpose( lightViewProj );

	m_perFramePS.ShadowCameraRange = data.CameraRange;

	m_shadowBatches->Clear();

	m_shadowMap->SetShadowTexture();

	CCP_STATS_INC( shadowsRendered );
}

void EveSpaceScene::ApplyPerFrameData( Tr2RenderContext& renderContext )
{
	static const unsigned perFrameVsMask = 
		( 1 << VERTEX_SHADER )					|
		SHADER_TYPE_EXISTS( COMPUTE_SHADER )	|
		SHADER_TYPE_EXISTS( GEOMETRY_SHADER )	|
		SHADER_TYPE_EXISTS( HULL_SHADER )		|
		SHADER_TYPE_EXISTS( DOMAIN_SHADER)		;

	FillAndSetConstants( m_perFrameVSBuffer, &m_perFrameVS, sizeof( m_perFrameVS ), perFrameVsMask, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
	FillAndSetConstants( m_perFramePSBuffer, &m_perFramePS, sizeof( m_perFramePS ), PIXEL_SHADER , Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Sorts and gathers transparent batches
// Arguments:
//   objectsWithTransparencies - a list of renderables with transparencies
//   batches - the batch map to be used
// --------------------------------------------------------------------------------------
void EveSpaceScene::PrepareTransparentBatch( Tr2RenderableSortList& objectsWithTransparencies, BatchMap& batches )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Sort objects front to back
	std::stable_sort( objectsWithTransparencies.begin(),  objectsWithTransparencies.end() );

	// Add the objects, back to front 
	for( Tr2RenderableSortList::const_reverse_iterator it =  objectsWithTransparencies.rbegin(); it !=  objectsWithTransparencies.rend(); ++it )
	{
		ITr2Renderable* r = it->m_object;
		Tr2PerObjectData* objectData = r->GetPerObjectData( batches[TRIBATCHTYPE_TRANSPARENT] );
		r->GetBatches( batches[TRIBATCHTYPE_TRANSPARENT], TRIBATCHTYPE_TRANSPARENT, objectData );
	}
}

void GetBatchesFromRenderables(	ITr2Renderable** const objectRenderables, const unsigned renderableCount, 
								Tr2RenderableSortList* const objectsWithTransparencies, 
								EveSpaceScene::BatchMap& batches, 
								const TriBatchType* batchTypes, const unsigned batchTypeCount )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( unsigned i = 0; i != renderableCount; ++i )
	{
		ITr2Renderable* r = objectRenderables[i];

		Tr2PerObjectData* objectData = r->GetPerObjectData( batches[TRIBATCHTYPE_OPAQUE] );
		
		for( unsigned type = 0; type != batchTypeCount; ++type )
		{
			r->GetBatches( batches[batchTypes[type]], batchTypes[type], objectData );
		}

		if( objectsWithTransparencies && r->HasTransparentBatches() )
		{
			ITr2RenderableEntry entry;
			entry.m_object = r;
			entry.m_distance = r->GetSortValue();
			objectsWithTransparencies->push_back( entry );	
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Gathers all batches from renderables and populates a list of renderables with
//   transparent areas.
// Arguments:
//   objectRenderables - object renderables that we want to gather batches from
//   objectsWithTransparencies - out, a list of renderables with transparencies
//   batches - the batch map to be used
// --------------------------------------------------------------------------------------
void EveSpaceScene::GetAllBatchesFromRenderables( std::vector<ITr2Renderable*>& objectRenderables, Tr2RenderableSortList& objectsWithTransparencies, BatchMap& batches )
{
	if( objectRenderables.empty() || !Tr2Renderer::GetPoolAllocator() )
	{
		return;
	}

	static const TriBatchType s_allTypes[] = 
	{
			TRIBATCHTYPE_OPAQUE,
			TRIBATCHTYPE_DECAL, 
			TRIBATCHTYPE_ADDITIVE,
			TRIBATCHTYPE_DEPTH,
			TRIBATCHTYPE_DISTORTION
	};

	unsigned typeCount = m_distortionMap ? 5 : 4;

	::GetBatchesFromRenderables( &objectRenderables[0], (unsigned int)objectRenderables.size(), &objectsWithTransparencies, batches, s_allTypes, typeCount );	
}

// --------------------------------------------------------------------------------------
// Description:
//   Gathers opaque and decal batches from renderables.
// Arguments:
//   objectRenderables - object renderables that we want to gather batches from
//   batches - the batch map to be used
// --------------------------------------------------------------------------------------
void EveSpaceScene::GetOpaqueBatchesFromRenderables( std::vector<ITr2Renderable*> &objectRenderables,  BatchMap& batches )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( objectRenderables.empty() || !Tr2Renderer::GetPoolAllocator() )
	{
		return;
	}

	static const TriBatchType s_allTypes[] = 
	{
			TRIBATCHTYPE_OPAQUE,
			TRIBATCHTYPE_DECAL
	};

	::GetBatchesFromRenderables( &objectRenderables[0], (unsigned int)objectRenderables.size(), nullptr, batches, s_allTypes, 2 );	
}

// --------------------------------------------------------------------------------------
// Description:
//   Gathers depth batches from renderables.
// Arguments:
//   objectRenderables - object renderables that we want to gather batches from
//   batches - the batch map to be used
// --------------------------------------------------------------------------------------
void EveSpaceScene::GetDepthBatchesFromRenderables( std::vector<ITr2Renderable*>& objectRenderables, BatchMap& batches )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( objectRenderables.empty() || !Tr2Renderer::GetPoolAllocator() )
	{
		return;
	}

	static const TriBatchType s_allTypes[] = 
	{
			TRIBATCHTYPE_DEPTH
	};

	::GetBatchesFromRenderables( &objectRenderables[0], (unsigned int)objectRenderables.size(), nullptr, batches, s_allTypes, 1 );
}

// --------------------------------------------------------------------------------------
// Description:
//   Gathers additive and distortion batches from a list of renderables and populates
//   a list of renderables with transparencies.
// Arguments:
//   objectRenderables - object renderables that we want to gather batches from
//   objectsWithTransparencies - a list of renderables with transparencies
//   batches - the batch map to be used
// --------------------------------------------------------------------------------------
void EveSpaceScene::GetTransparentBatchesFromRenderables( std::vector<ITr2Renderable*> &objectRenderables, Tr2RenderableSortList &objectsWithTransparencies, BatchMap& batches )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( objectRenderables.empty() || !Tr2Renderer::GetPoolAllocator() )
	{
		return;
	}

	static const TriBatchType s_allTypes[] = 
	{
			TRIBATCHTYPE_ADDITIVE,		
			TRIBATCHTYPE_DISTORTION		
	};

	unsigned typeCount = m_distortionMap ? 2 : 1;

	::GetBatchesFromRenderables( &objectRenderables[0], (unsigned int)objectRenderables.size(), &objectsWithTransparencies, batches, s_allTypes, typeCount );	
}

// --------------------------------------------------------------------------------------
// Description:
//   Gathers, finalizes, renders and clears batches of the type provided using the provided
//   rendering mode
// Arguments:
//   renderables - object renderables that we want to gather batches from
//   batch - the accumulator we gather the batches into
//   batchType - the batch type to gather
//   rm - the rendering mode to use
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderRenderables(	const std::vector<ITr2Renderable*> &renderables, 
										ITriRenderBatchAccumulator* batch, 
										TriBatchType batchType, 
										Tr2EffectStateManager::RenderingMode rm,
										Tr2RenderContext &renderContext )
{
	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	if( !allocator )
	{
		return;
	}
	for( auto it = renderables.cbegin(); it != renderables.cend(); ++it )
	{
		ITr2Renderable* r = *it;
		Tr2PerObjectData* objectData = r->GetPerObjectData( batch );
		r->GetBatches( batch, batchType, objectData );
	}

	RenderBatch( batch, rm, renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Finalizes the batch and renders with the specified rendering mode.
// Arguments:
//   batch - the accumulator we gather the batches into
//   batchType - the batch type to gather
//   rm - the rendering mode to use
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderBatch(	ITriRenderBatchAccumulator* batch, 
									Tr2EffectStateManager::RenderingMode rm, 
									Tr2RenderContext &renderContext )
{
	auto& visualizerEffect = m_visualizerEffects[m_visualizeMethod];

	batch->Finalize();
	renderContext.m_esm.ApplyStandardStates( rm );
	switch( visualizerEffect.type )
	{
	case VisualizerEffect::PIXEL_SHADER_REPLACEMENT:
		renderContext.RenderBatchesWithOverride( batch, visualizerEffect.effect );
		break;
	case VisualizerEffect::FULL_SCREEN_QUAD_OVERLAY:
		renderContext.RenderBatches( batch );
		break;
	default:
		break;
	}
	batch->Clear();
}

// --------------------------------------------------------------------------------------
// Description:
//   Renders opaque and decal batches from the provided BatchMap.
// Arguments:
//   renderingContext - Tr2RenderContext for rendering( unused at the moment ).
//   batches - BatchMap that contains the opaque and decal batches to be rendered
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderOpaqueBatches( BatchMap& batches, Tr2RenderContext &renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto& visualizerEffect = m_visualizerEffects[m_visualizeMethod];

	switch( visualizerEffect.type )
	{
	case VisualizerEffect::PIXEL_SHADER_REPLACEMENT:
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
		renderContext.RenderBatchesWithOverride( batches[TRIBATCHTYPE_OPAQUE], visualizerEffect.effect );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DECAL );
		renderContext.RenderBatchesWithOverride( batches[TRIBATCHTYPE_DECAL], visualizerEffect.effect );
		break;
	case VisualizerEffect::FULL_SCREEN_QUAD_OVERLAY:
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
		renderContext.RenderBatches( batches[TRIBATCHTYPE_OPAQUE] );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DECAL );
		renderContext.RenderBatches( batches[TRIBATCHTYPE_DECAL] );
		break;
	default:
		break;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Renders additive and transparent batches from the provided BatchMap.
// Arguments:
//   renderingContext - Tr2RenderContext for rendering( unused at the moment ).
//   batches - BatchMap that contains the additive and transparent batches to be rendered
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderTransparentBatches( BatchMap& batches, Tr2RenderContext &renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto& visualizerEffect = m_visualizerEffects[m_visualizeMethod];

	switch( visualizerEffect.type )
	{
	case VisualizerEffect::PIXEL_SHADER_REPLACEMENT:
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
		renderContext.RenderBatchesWithOverride( batches[TRIBATCHTYPE_TRANSPARENT], visualizerEffect.effect );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
		renderContext.RenderBatchesWithOverride( batches[TRIBATCHTYPE_ADDITIVE], visualizerEffect.effect );
		break;
	case VisualizerEffect::FULL_SCREEN_QUAD_OVERLAY:
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
		renderContext.RenderBatches( batches[TRIBATCHTYPE_TRANSPARENT] );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
		renderContext.RenderBatches( batches[TRIBATCHTYPE_ADDITIVE] );
		break;
	default:
		break;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Renders distortion batches from the provided BatchMap.
// Arguments:
//   renderingContext - Tr2RenderContext for rendering( unused at the moment ).
//   batches - BatchMap that contains the distortion batches to be rendered
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderDistortionBatches( BatchMap& batches, Tr2RenderContext &renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

    if( !m_distortionMap )
    {
        return;
    }

    // Hold on to original depth stencil and back buffer
	Tr2PushPopRT pushPopRT( *m_distortionMap, renderContext );
	Tr2PushPopDS pushPopDS( renderContext );
	
	if( m_depthMap && m_depthMap->IsValid() )
    {
		if( m_depthMap->m_depthStencil.GetMsaaDesc().samples > 1 )
		{
			Tr2Renderer::SetDepthStencilBuffer( nullDS, renderContext );
		}
		else
		{
			Tr2Renderer::SetDepthStencilBuffer( *m_depthMap, renderContext );        
		}
    }


	renderContext.Clear( CLEARFLAGS_TARGET, 0x007f7f00, 1.f, 0 );

    // Do the actual rendering
	ApplyPerFrameData( renderContext );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
    renderContext.RenderBatches( batches[TRIBATCHTYPE_DISTORTION] );
}

// --------------------------------------------------------------------------------------
// Description:
//   Render shadowed objects. Actual shadows are optional.
// Arguments:
//   renderingContext - Tr2RenderContext for rendering( unused at the moment ).
//   objectsReceivingShadows - the shadow renderables
//   renderShadows - is actual shadow rendering needed
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderObjectsReceivingShadows(	std::vector<ShadowReceiver>& objectsReceivingShadows, 
													bool renderShadows, 
													Tr2RenderContext &renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	if( !allocator )
	{
		return;
	}

	std::unique_ptr<std::vector<ITr2Renderable*>[]> shadowRenderables;
	std::unique_ptr<std::vector<IEveShadowCaster*>[]> debugCasters;
	std::vector<IEveShadowCaster*> shadowCasters;

	if( renderShadows && !objectsReceivingShadows.empty() )
	{
		shadowRenderables.reset( new std::vector<ITr2Renderable*>[objectsReceivingShadows.size()] );
		debugCasters.reset( new std::vector<IEveShadowCaster*>[objectsReceivingShadows.size()] );

		if( !m_selfShadowOnly )
		{
			GetAllShadowCasters( shadowCasters );
		}

		std::vector<IEveShadowCaster*> shadowReceivers;
		shadowReceivers.reserve( objectsReceivingShadows.size() );
		for( auto it = objectsReceivingShadows.begin(); it != objectsReceivingShadows.end(); ++it )
		{
			IEveShadowCasterPtr caster( BlueCastPtr( it->object ) );
			if( caster )
			{
				shadowReceivers.push_back( caster );
			}
		}
		
		if( m_shadowMap && m_shadowMap->GetTexture().IsValid() )
		{
			Tr2ParallelFor( 
				Tr2BlockedRange<size_t>( 0, objectsReceivingShadows.size() ), 
				[&]( const Tr2BlockedRange<size_t>& range ) -> void
				{
					for( size_t index = range.begin(); index != range.end(); ++index)
					{
						IEveSpaceObject2* obj = objectsReceivingShadows[index].object;
						GetShadowCasterRenderables( obj, shadowReceivers[index], shadowCasters, shadowRenderables[index], debugCasters[index] );
					}
				} );
		}
	}

	for( size_t i = 0; i < objectsReceivingShadows.size(); ++i )
	{
		IEveSpaceObject2* obj = objectsReceivingShadows[i].object;
		if( renderShadows )
		{
			float shadowStrength = 0.0f;
			if( ( m_shadowFadeThreshold - m_shadowThreshold ) > 0 )
			{
				shadowStrength = 1.0f - ( objectsReceivingShadows[i].estimatedSize - m_shadowThreshold ) / ( m_shadowFadeThreshold - m_shadowThreshold );
			}
			m_shadowLightnessVar = shadowStrength;
			PrepareShadowMap( obj, shadowRenderables[i], debugCasters[i], renderContext );
			if( m_shadowMap )
			{
				m_perFramePS.ShadowMapSettings = m_shadowMap->GetShadowMapSettings();
			}
			else
			{
				m_perFramePS.ShadowMapSettings = Vector4( 1.f, 1.f, 0.f, 0.f );
			}
			m_perFramePS.ShadowLightness = shadowStrength;
			ApplyPerFrameData( renderContext );
		}

		std::vector<ITr2Renderable*> objectRenderables;
		obj->GetRenderables( objectRenderables, nullptr );
		GetOpaqueBatchesFromRenderables( objectRenderables, m_secondaryBatches );

		FinalizeBatches( m_secondaryBatches );
		{
			if( m_velocityMap )
			{
				Tr2PushPopRT rt( *m_velocityMap, renderContext, 1 );
				RenderOpaqueBatches( m_secondaryBatches, renderContext );
			}
			else
			{
				RenderOpaqueBatches( m_secondaryBatches, renderContext );
			}
		}
		ClearBatches( m_secondaryBatches );

	}
}

void EveSpaceScene::TAAOffset()
{
	if( m_taaPattern == TAA_NONE )
	{
		m_xProjOffset = 0;
		m_yProjOffset = 0;
	}
	else if( m_taaPattern == TAA_RANDOM )
	{
		m_xProjOffset = m_taaPixelOffsetScale / (float)Tr2Renderer::GetRenderTargetWidth() * (2.f * TriFloatRandom01() - 1.f);
		m_yProjOffset = m_taaPixelOffsetScale / (float)Tr2Renderer::GetRenderTargetHeight() * (2.f * TriFloatRandom01() - 1.f);
	}
	else if( m_taaPattern == TAA_2X )
	{
		m_taaSamplingIndex = m_taaSamplingIndex % 2;
		m_xProjOffset = m_taaPixelOffsetScale / (float)Tr2Renderer::GetRenderTargetWidth() * m_taaSamplingPatterns[m_taaSamplingIndex].x;
		m_yProjOffset = m_taaPixelOffsetScale / (float)Tr2Renderer::GetRenderTargetHeight() * m_taaSamplingPatterns[m_taaSamplingIndex].y;
	}
	else if( m_taaPattern == TAA_3X )
	{
		m_taaSamplingIndex = m_taaSamplingIndex % 3;
		m_xProjOffset = m_taaPixelOffsetScale / (float)Tr2Renderer::GetRenderTargetWidth() * m_taaSamplingPatterns[m_taaSamplingIndex + 6].x;
		m_yProjOffset = m_taaPixelOffsetScale / (float)Tr2Renderer::GetRenderTargetHeight() * m_taaSamplingPatterns[m_taaSamplingIndex + 6].y;
	}
	else
	{
		m_taaSamplingIndex = m_taaSamplingIndex % 4;
		m_xProjOffset = m_taaPixelOffsetScale / (float)Tr2Renderer::GetRenderTargetWidth() * m_taaSamplingPatterns[m_taaSamplingIndex+2].x;
		m_yProjOffset = m_taaPixelOffsetScale / (float)Tr2Renderer::GetRenderTargetHeight() * m_taaSamplingPatterns[m_taaSamplingIndex+2].y;
	}
}

void EveSpaceScene::UpdatePostProcessPSData()
{
	double currentViewProjD[16];
	Matrix currentProj;
	
		currentProj = m_frameData.projection;
		currentProj = EveCamera::AddCenterOffset( currentProj, -m_xProjOffset, -m_yProjOffset, Tr2Renderer::GetFrontClip(), Tr2Renderer::GetBackClip() );

	// Find the current inverse view projection
	double viewTransform[16];
	double currentProjection[16];
	Matrix4dMultiply(
		currentViewProjD,
		Matrix4dFromMatrix( viewTransform, Tr2Renderer::GetViewTransform() ),
		Matrix4dFromMatrix( currentProjection, currentProj ) );
	double invViewProjD[16];
	Matrix4dInvert( invViewProjD, currentViewProjD );

	// Now construct the reprojection matrix
	double reprojection[16];
	Matrix4dMultiply( reprojection, invViewProjD, m_viewProjectLastD );
	Matrix repro = Matrix4dToMatrix( reprojection );
	m_postProcessPSData.ReprojectionMatrix = Transpose( repro );

	memcpy( m_viewProjectLastD, currentViewProjD, 128 );
	
	m_postProcessPSData.DeltaT = m_updateContext.GetDeltaT();
	m_postProcessPSData.OriginShift = m_updateContext.GetOriginShift();

	m_postProcessPSBuffer->SetData( (void*)&m_postProcessPSData, sizeof( m_postProcessPSData ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Set up rendering states, frustum and gather all batches.
// --------------------------------------------------------------------------------------
void EveSpaceScene::BeginRender( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_hasDepthPass = false;

	if( !m_display )
	{
		return;
	}

	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	if( !allocator )
	{
		return;
	}

	renderContext.AddGpuMarker( __FUNCTION__ );

	if( m_visualizeMethod != VM_NONE )
	{
		if( !m_visualizerEffects[m_visualizeMethod].effect )
		{
			m_visualizerEffects[m_visualizeMethod].effect.CreateInstance();
			const char* path = VISUALIZER_EFFECT_PATH[m_visualizeMethod];
			m_visualizerEffects[m_visualizeMethod].effect->SetEffectPathName( path );
		}
	}

	if( m_shLightingManager )
	{
		m_shLightingManager->UpdateWithDirectionalLight( m_sunData.DirWorld, Vector3( 1.f, 1.f, 1.f ) );
	}

	if( m_dataTextureMgr )
	{
		m_dataTextureMgr->SetVariables();
	}

	SetNoShadow();

	// Set up the frustum for visibility checking.
	// Todo: Solve the issue of getting renderables from objects that aren't visible but
	// still might trigger rendering, such as particle effects or turret firing effects.
	TriFrustum& frustum = m_frameData.frustum;
	frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &Tr2Renderer::GetViewPosition(), &Tr2Renderer::GetProjectionTransform(), Tr2Renderer::GetViewport() );
	
	TAAOffset();
	m_taaSamplingIndex++;

	Matrix proj = Tr2Renderer::GetProjectionTransform();
	if( m_taaPattern != TAA_NONE )
	{
		proj = EveCamera::AddCenterOffset( proj, m_xProjOffset, m_yProjOffset, Tr2Renderer::GetFrontClip(), Tr2Renderer::GetBackClip() );
	}
	m_frameData.projection = proj;
	Tr2Renderer::SetProjectionTransform( m_frameData.projection );

	renderContext.m_esm.BeginManagedRendering();
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
	
	GatherBatches( renderContext );

	UpdatePostProcessPSData();
	UpdateVariableStore();

	if( g_eveSpaceSceneDynamicLighting )
	{
		auto lightManager = Tr2LightManager::GetOrCreateInstance( "res:/graphics/effect/managed/space/system/computelightlists.fx" );

		CCP_STATS_SCOPED_TIME( gatherDynamicLights );

		lightManager->Clear();
		lightManager->SetFrustum( frustum );

		Tr2ParallelFor( Tr2BlockedRange<size_t>( 0, m_objects.size(), 20 ), [&] ( Tr2BlockedRange<size_t> range ) 
		{
			for( auto i = range.begin(); i != range.end(); ++i )
			{
				m_objects[i]->GetLights( *lightManager );
			}
		} );

		for( auto it = begin( m_backgroundObjects ); it != end( m_backgroundObjects ); ++it )
		{
			( *it )->GetLights( *lightManager );
		}
		m_cameraAttachmentParent->GetLights( *lightManager );
	}
	else
	{
		Tr2LightManager::DeleteInstance();
	}


	//  the lensflares need a special pre-render update
	for( auto it = m_lensflares.cbegin(); it != m_lensflares.cend(); ++it )
	{
		(*it)->PrepareRender( frustum );
	}
	
	if( m_velocityMap )
	{
		Tr2PushPopRT rt( *m_velocityMap, renderContext, 1 );
		renderContext.Clear( CLEARFLAGS_TARGET, 0x00000000, 1.f, 0, 1 );
	}

	PopulatePerFramePSData( m_perFramePS );
	PopulatePerFrameVSData( m_perFrameVS );
	ApplyPerFrameData( renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Gather all batches into the m_primaryBatches BatchMap.
// --------------------------------------------------------------------------------------
void EveSpaceScene::GatherBatches( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	TriFrustum& frustum = m_frameData.frustum;
	
	std::vector<ShadowReceiver>& objectsReceivingShadow = m_frameData.objectsReceivingShadow;
	std::vector<IEveSpaceObject2*> objectsNotReceivingShadow;
	std::vector<ITr2Renderable*> renderables;
	Tr2RenderableSortList transparentObjects;
	const Matrix& identity = IdentityMatrix();

	{
		CCP_STATS_ZONE( "UpdateVisibility" );
		Tr2ParallelDo( m_objects.begin(), m_objects.end(), [&]( IEveSpaceObject2* obj ) { obj->UpdateVisibility( frustum, identity ); } );

		m_cameraAttachmentParent->SetTransform( Tr2Renderer::GetInverseViewTransform() );
		m_cameraAttachmentParent->UpdateSyncronous( m_updateContext );
		m_cameraAttachmentParent->UpdateAsyncronous( m_updateContext );
		m_cameraAttachmentParent->UpdateVisibility( frustum, identity );

		Tr2ParallelDo( m_planets.begin(), m_planets.end(), [&]( EvePlanet* obj ) { obj->UpdateZOnlyVisibility( frustum ); } );
	}
	// Separate objects that will receive shadows from others. Shadowed objects will be rendered object by object,
	// with a shadow map generated for each object. Remaining objects will be batched up.
	for( IEveSpaceObject2Vector::iterator it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		IEveSpaceObject2* obj = *it;

		// If objects don't know their bounding sphere size they won't get shadows
		bool getsShadow = false;

		if( m_enableShadows && m_shadowMap )
		{
			IEveShadowCasterPtr shObj( BlueCastPtr( obj ) );
			if( shObj && shObj->IsShadowReceiveEnabled() )
			{
				Vector4 boundingSphere;
				if( obj->GetBoundingSphere( boundingSphere ) )
				{
					if( frustum.IsSphereVisible( &boundingSphere, true ) )
					{
						float estimatedSize = frustum.GetPixelSizeAccross( &boundingSphere );

						if( estimatedSize >= m_shadowThreshold )
						{
							objectsReceivingShadow.push_back( ShadowReceiver( estimatedSize, obj ) );
							getsShadow = true;
						}
					}
				}
			}
		}

		if( !getsShadow )
		{
			objectsNotReceivingShadow.push_back( obj );
		}
	}
	objectsNotReceivingShadow.push_back( m_cameraAttachmentParent );

	if( objectsReceivingShadow.size() > m_shadowReceiverMaxCount )
	{
		CCP_STATS_ZONE( "Trinity/EveSpaceScene/Render/SortShadows" );
		// We have too many objects that want to receive a shadow.
		// Sort the list by estimated size and allow the largest ones
		// to render their shadow map - the rest is pushed onto the list
		// of objects not receiving a shadow.
		std::sort( objectsReceivingShadow.begin(), objectsReceivingShadow.end() );
		size_t count = objectsReceivingShadow.size() - m_shadowReceiverMaxCount;
		auto end = objectsReceivingShadow.begin() + count;
		for( auto it = objectsReceivingShadow.begin(); it != end; ++it )
		{
			objectsNotReceivingShadow.push_back( it->object );
		}

		// Remove the excess shadow receivers.
		auto start = objectsReceivingShadow.begin();
		objectsReceivingShadow.erase(start, end);
	}

	if( m_impostorManager )
	{
		uint32_t size = 2;
		uint32_t threshold = uint32_t( g_eveSpaceSceneLowDetailThreshold ) / 2;
		while( size * 2 < threshold )
		{
			size *= 2;
		}
		m_impostorManager->SetItemSize( size, size );
		m_impostorManager->BeginUpdate();
	}

	for( std::vector<IEveSpaceObject2*>::iterator it = objectsNotReceivingShadow.begin(); it != objectsNotReceivingShadow.end(); ++it )
	{
		IEveSpaceObject2* obj = *it;
		obj->GetRenderables( renderables, m_impostorManager );
	}

	if( m_impostorManager )
	{
		m_impostorManager->EndUpdate();
	}

	UpdateImpostors();
	
	for( auto it = m_staticParticles.begin(); it != m_staticParticles.end(); ++it )
	{
		(*it)->GetRenderables( frustum, renderables );
	}

	std::vector<ITr2Renderable*> shadowRenderables;
	for( std::vector<ShadowReceiver>::iterator it = objectsReceivingShadow.begin(); it != objectsReceivingShadow.end(); ++it )
	{
		IEveSpaceObject2* obj = it->object;
		obj->GetRenderables( shadowRenderables, nullptr );
	}

	UpdateQuadRenderer( frustum, objectsReceivingShadow, objectsNotReceivingShadow, renderContext );
	Tr2QuadRenderer::Instance()->GetBatches( TRIBATCHTYPE_OPAQUE, m_primaryBatches[TRIBATCHTYPE_OPAQUE] );
	Tr2QuadRenderer::Instance()->GetBatches( TRIBATCHTYPE_ADDITIVE, m_primaryBatches[TRIBATCHTYPE_ADDITIVE] );

	GetAllBatchesFromRenderables( renderables, transparentObjects, m_primaryBatches );
	GetTransparentBatchesFromRenderables( shadowRenderables, transparentObjects, m_primaryBatches );
	GetDepthBatchesFromRenderables( shadowRenderables, m_primaryBatches );
	PrepareTransparentBatch( transparentObjects, m_primaryBatches );

	FinalizeBatches( m_primaryBatches );

	UpdateShLighting( objectsReceivingShadow, objectsNotReceivingShadow );
}

void EveSpaceScene::UpdateImpostors()
{
	if( !m_impostorManager || m_impostorManager->GetRenderQueueLength() == 0 )
	{
		return;
	}

	CCP_STATS_ZONE( __FUNCTION__ );

	CTriViewport fakeViewport;
	fakeViewport.width = 128;
	fakeViewport.height = 128;

	Tr2Renderer::PushViewTransform();
	Tr2Renderer::PushProjection();
	Tr2Renderer::PushViewport();

	ON_BLOCK_EXIT( [&]{ 
		Tr2Renderer::PopViewport();
		Tr2Renderer::PopProjection();
		Tr2Renderer::PopViewTransform();
	} );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	m_impostorManager->BeginUpdateAtlas( renderContext );
	ON_BLOCK_EXIT( [&]{ m_impostorManager->EndUpdateAtlas( renderContext ); } );

	Tr2DepthStencilPtr bkDepthMap = m_depthMap;
	m_depthMap = m_impostorManager->GetItemDepthStencil();
	ON_BLOCK_EXIT( [&]{ m_depthMap = bkDepthMap; } );

	ITr2TextureProvider* bkDepthMapTexVar = nullptr;

	if( m_depthMapVar.GetType() == TRIVARIABLE_TEXTURE_RES )
	{
		m_depthMapVar.GetValue( bkDepthMapTexVar );
		if( bkDepthMapTexVar )
		{
			bkDepthMapTexVar->Lock();
		}

		m_depthMapVar = m_impostorManager->GetItemDepthStencil();
	}
	ON_BLOCK_EXIT( [&]{ 
		if( m_depthMapVar.GetType() == TRIVARIABLE_TEXTURE_RES )
		{
			m_depthMapVar = bkDepthMapTexVar;
			if( bkDepthMapTexVar )
			{
				bkDepthMapTexVar->Unlock();
			}
		}
		UpdateVariableStore();
	} );

	SetNoShadow();
	UpdateVariableStore();

	Vector3 eye = Tr2Renderer::GetInverseViewTransform().GetTranslation();
	Vector3 up = TransformNormal( Vector3( 0, 1, 0 ), Tr2Renderer::GetInverseViewTransform() );

	for( size_t i = 0; i < m_impostorManager->GetRenderQueueLength(); ++i )
	{
		auto spaceObject = m_impostorManager->BeginImpostorUpdate( i, renderContext );
		Matrix transform;
		spaceObject->GetLocalToWorldTransform( transform );

		Vector4 sphere( 0.f, 0.f, 0.f, 1.f );
		spaceObject->GetImpostorBoundingSphere( sphere );

		Vector3 position( sphere.x, sphere.y, sphere.z );

		Vector3 viewDir = Normalize( eye - position );

		const float distance = sphere.w * 10;
		Matrix view( XMMatrixLookAtRH( position + viewDir * distance, position, up ) );
		Matrix proj( XMMatrixPerspectiveRH( sphere.w * 2, sphere.w * 2, distance - sphere.w, distance + sphere.w ) );

		Tr2Renderer::SetViewTransform( view );
		Tr2Renderer::SetProjectionTransform( proj );

		TriFrustum frustum = m_frameData.frustum;
		frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &Tr2Renderer::GetViewPosition(), &Tr2Renderer::GetProjectionTransform(), fakeViewport );

		PopulatePerFramePSData( m_perFramePS );
		PopulatePerFrameVSData( m_perFrameVS );
		m_perFrameVS.FogFactors.z = 0;
		ApplyPerFrameData( renderContext );

		spaceObject->GetImpostorBatches( frustum, m_primaryBatches );
		FinalizeBatches( m_primaryBatches );

		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
		renderContext.RenderBatches( m_primaryBatches[TRIBATCHTYPE_OPAQUE] );

		renderContext.SetReadOnlyDepth( true );

		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
		renderContext.RenderBatches( m_primaryBatches[TRIBATCHTYPE_TRANSPARENT] );

		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
		renderContext.RenderBatches( m_primaryBatches[TRIBATCHTYPE_ADDITIVE] );

		renderContext.SetReadOnlyDepth( false );

		ClearBatches( m_primaryBatches );

		m_impostorManager->EndImpostorUpdate( i, renderContext );
	}
}


// --------------------------------------------------------------------------------------
// Description:
//   Updates SH lighing for a set of space objects.
// Arguments:
//   objectsReceivingShadow - vector of objects receiving shadow
//   objectsNotReceivingShadow - vector of objects not receiving shadow
// --------------------------------------------------------------------------------------
void EveSpaceScene::UpdateShLighting( 
	const std::vector<ShadowReceiver>& objectsReceivingShadow, 
	const std::vector<IEveSpaceObject2*>& objectsNotReceivingShadow )
{
	CCP_STATS_SCOPED_TIME( shLightingUpdateTime );

	if( m_shLightingManager )
	{
		Tr2ParallelFor( Tr2BlockedRange<size_t>( 0, objectsReceivingShadow.size(), 20 ), [&] ( Tr2BlockedRange<size_t> range ) 
		{
			for( auto i = range.begin(); i != range.end(); ++i )
			{
				ITr2ShLightingReceiverPtr receiver = BlueCastPtr( objectsReceivingShadow[i].object );
				if( receiver != nullptr )
				{
					receiver->UpdateShLighting( *m_shLightingManager );
				}
			}
		} );

		Tr2ParallelFor( Tr2BlockedRange<size_t>( 0, objectsNotReceivingShadow.size(), 20 ), [&] ( Tr2BlockedRange<size_t> range ) 
		{
			for( auto i = range.begin(); i != range.end(); ++i )
			{
				ITr2ShLightingReceiverPtr receiver = BlueCastPtr( objectsNotReceivingShadow[i] );
				if( receiver != nullptr )
				{
					receiver->UpdateShLighting( *m_shLightingManager );
				}
			}
		} );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds quads from visible space objects to a quad renderer.
// Arguments:
//   objectsReceivingShadow - list of visible object
//   objectsNotReceivingShadow - another list of visible object
//   renderContext - current render context
// --------------------------------------------------------------------------------------
void EveSpaceScene::UpdateQuadRenderer( 
	const TriFrustum& frustum,
	const std::vector<ShadowReceiver>& objectsReceivingShadow, 
	const std::vector<IEveSpaceObject2*>& objectsNotReceivingShadow, 
	Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto& quadRenderer = *Tr2QuadRenderer::Instance();

	Tr2ParallelFor( Tr2BlockedRange<size_t>( 0, objectsReceivingShadow.size(), 20 ), [&] ( Tr2BlockedRange<size_t> range ) 
	{
		for( auto i = range.begin(); i != range.end(); ++i )
		{
			objectsReceivingShadow[i].object->AddQuadsToQuadRenderer( frustum, quadRenderer );
		}
	} );

	Tr2ParallelFor( Tr2BlockedRange<size_t>( 0, objectsNotReceivingShadow.size(), 20 ), [&] ( Tr2BlockedRange<size_t> range ) 
	{
		for( auto i = range.begin(); i != range.end(); ++i )
		{
			objectsNotReceivingShadow[i]->AddQuadsToQuadRenderer( frustum, quadRenderer );
		}
	} );
	
	quadRenderer.BeginRendering( renderContext );
}

// --------------------------------------------------------------------------------------
void EveSpaceScene::UpdateQuadRenderer( 
	const TriFrustum& frustum,
	PIEveSpaceObject2Vector& objects,
	Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto& quadRenderer = *Tr2QuadRenderer::Instance();

	Tr2ParallelFor( Tr2BlockedRange<size_t>( 0, objects.size(), 20 ), [&] ( Tr2BlockedRange<size_t> range ) 
	{
		for( auto i = range.begin(); i != range.end(); ++i )
		{
			auto obj = objects[i];
			obj->AddQuadsToQuadRenderer( frustum, quadRenderer );
		}
	} );

	quadRenderer.BeginRendering( renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns shared quad renderer pointer. Mostly for blue exposure.
// --------------------------------------------------------------------------------------
Tr2QuadRenderer* EveSpaceScene::GetQuadRenderer() const
{
	return Tr2QuadRenderer::Instance();
}

// --------------------------------------------------------------------------------------
// Description:
//   Render all background objects, nebula, stars, planets etc.
// Returns:
//   boolean indicating weather background distortion is required.
// --------------------------------------------------------------------------------------
bool EveSpaceScene::RenderBackgroundPass( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	bool doBackgroundDistortion = false;

	// "background" rendering
	if( !m_backgroundRenderingEnabled )
	{
		return false;
	}

	if( !m_display )
	{
		return false;
	}

	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	if( !allocator )
	{
		return false;
	}

	renderContext.AddGpuMarker( __FUNCTION__ );

	TriFrustum& frustum = m_frameData.frustum;
	std::vector<ITr2Renderable*> visible;
	Tr2RenderableSortList transparentObjects;

	// nebula
	if( m_backgroundEffect )
	{
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
		Tr2Renderer::DrawCameraSpaceScreenQuad( m_backgroundEffect->GetShaderStateInterface(), m_backgroundEffect );
	}

	// stars
	if( m_starfield )
	{
		m_starfield->GetBatches( m_secondaryBatches[TRIBATCHTYPE_ADDITIVE], 0 );
		RenderBatch(	m_secondaryBatches[TRIBATCHTYPE_ADDITIVE], 
						Tr2EffectStateManager::RM_ALPHA_ADDITIVE, 
						renderContext );
	}

	// backgroundobjects
	if( !m_backgroundObjects.empty() )
	{
		for( auto it = m_backgroundObjects.begin(); it != m_backgroundObjects.end(); ++it )
		{
			auto obj = *it;
			obj->UpdateVisibility( frustum, IdentityMatrix() );
			obj->GetRenderables( visible, nullptr );	
		}
		if( !visible.empty() )
		{
			GetAllBatchesFromRenderables( visible, transparentObjects, m_secondaryBatches );
			PrepareTransparentBatch( transparentObjects, m_secondaryBatches );
			FinalizeBatches( m_secondaryBatches );

			renderContext.m_esm.ApplyStandardStates(Tr2EffectStateManager::RM_DEPTH_ONLY);
			renderContext.RenderBatches(m_secondaryBatches[TRIBATCHTYPE_DEPTH], BlueSharedString("Depth"));
			
			RenderOpaqueBatches( m_secondaryBatches, renderContext );
			RenderTransparentBatches( m_secondaryBatches, renderContext );

			if( m_secondaryBatches[TRIBATCHTYPE_OPAQUE]->GetBatchCount() || m_secondaryBatches[TRIBATCHTYPE_DECAL]->GetBatchCount() )
			{
				Tr2Renderer::ClearDepthBuffer( 0.f );
			}

			ClearBatches( m_secondaryBatches );
		}
		visible.clear();
		transparentObjects.clear();
	}

	// planets
	if( !m_planets.empty() )
	{
		RenderPlanets( renderContext );
		// we must clear z-buffer if we rendered some planets because the planets
		// have their own view/projection and so their own z-depth, BUT before
		// clear, we must render the lensfalre occlusion queries, so we don't lose
		// the z info we have so far
		for( EveLensflareVector::const_iterator it = m_lensflares.begin(); it != m_lensflares.end(); ++it )
		{
			(*it)->RunBackgroundOcclusionQueries( renderContext, frustum );
		}

		// now it's ok to clear z-buffer
		Tr2Renderer::ClearDepthBuffer( 0.f );
	}

	if( m_warpTunnel )
	{
		m_warpTunnel->UpdateVisibility( frustum, IdentityMatrix() );
		m_warpTunnel->GetRenderables( visible, nullptr );	
		
		GetTransparentBatchesFromRenderables( visible, transparentObjects, m_secondaryBatches );
		PrepareTransparentBatch( transparentObjects, m_secondaryBatches );
		FinalizeBatches( m_secondaryBatches );
		RenderTransparentBatches( m_secondaryBatches, renderContext );
		if( m_secondaryBatches[TRIBATCHTYPE_DISTORTION]->GetBatchCount() > 0 )
		{
			RenderDistortionBatches( m_secondaryBatches, renderContext );
			doBackgroundDistortion = true;
		}
		ClearBatches( m_secondaryBatches );
	}

	renderContext.m_esm.EndManagedRendering();

	return doBackgroundDistortion;
}

// --------------------------------------------------------------------------------------
// Description:
//   Render opaque objects to populate a readable depth stencil.
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderDepthPass( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	
	renderContext.m_esm.BeginManagedRendering();

	std::vector<ITr2Renderable*> visible;

	// Render to depth map
	{
		renderContext.AddGpuMarker( __FUNCTION__ );

#if TRINITY_PLATFORM != TRINITY_DIRECTX9
		m_hasDepthPass = true;
#endif

		Tr2Renderer::ClearDepthBuffer( 0.0f );

		ApplyPerFrameData( renderContext );

		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DEPTH_ONLY );
		renderContext.RenderBatches( m_primaryBatches[TRIBATCHTYPE_OPAQUE], BlueSharedString( "Depth" ) );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DEPTH_ONLY );
		renderContext.RenderBatches( m_primaryBatches[TRIBATCHTYPE_DEPTH], BlueSharedString( "Depth" ) );
		
		// Planet z areas and shadowed objects need special treatment
		for( auto it = m_planets.begin(); it != m_planets.end(); ++it )
		{
			EvePlanet* obj = *it;
			obj->GetZOnlyRenderables( visible );
		}

		for( auto it = m_frameData.objectsReceivingShadow.begin(); it != m_frameData.objectsReceivingShadow.end(); ++it )
		{
			IEveSpaceObject2* obj = it->object;
			obj->GetRenderables( visible, nullptr );
		}

		if( !visible.empty() )
		{
			// secondary batches don't run through a ::GetAllBatchesX() function, so collect all batches individually
			GetOpaqueBatchesFromRenderables( visible, m_secondaryBatches );
			GetDepthBatchesFromRenderables( visible, m_secondaryBatches );

			FinalizeBatches( m_secondaryBatches );
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DEPTH_ONLY );
			renderContext.RenderBatches( m_secondaryBatches[TRIBATCHTYPE_OPAQUE], BlueSharedString( "Depth" ) );
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DEPTH_ONLY );
			renderContext.RenderBatches( m_secondaryBatches[TRIBATCHTYPE_DEPTH], BlueSharedString( "Depth" ) );
			ClearBatches( m_secondaryBatches );

			visible.clear();
		}
	}

	renderContext.m_esm.EndManagedRendering();
}

// --------------------------------------------------------------------------------------
// Description:
//   Main rendering of foreground objects.
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderMainPass( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	
	renderContext.m_esm.BeginManagedRendering();

	if( !m_display )
	{
		return;
	}

	renderContext.AddGpuMarker( __FUNCTION__ );

	if( m_hasDepthPass )
	{
		renderContext.SetReadOnlyDepth( true );
	}

	std::vector<ITr2Renderable*> objectRenderables;
	
	if( auto lightManager = Tr2LightManager::GetInstance() )
	{
		CCP_STATS_SCOPED_TIME( updateDynamicLightLists );

		uint32_t msaaType = 0;
		if( m_depthMap )
		{
			msaaType = std::max( m_depthMap->m_depthStencil.GetMsaaDesc().samples, 1u );
		}
		lightManager->UpdateLists( msaaType, renderContext );
	}
	
	if( !m_hasDepthPass )
	{
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DEPTH_ONLY );
		renderContext.RenderBatches( m_primaryBatches[TRIBATCHTYPE_DEPTH], BlueSharedString( "Depth" ) );
	}

	// Draw the planets to the z-buffer to occlude any stations etc.
	// that might be drawn behind moons.
	for( EvePlanetVector::iterator it = m_planets.begin(); it != m_planets.end(); ++it )
	{
		EvePlanet* obj = *it;
		obj->GetZOnlyRenderables( objectRenderables );
	}
	if( !objectRenderables.empty() )
	{
		RenderRenderables(	objectRenderables, 
							m_secondaryBatches[TRIBATCHTYPE_OPAQUE], 
							TRIBATCHTYPE_OPAQUE, 
							Tr2EffectStateManager::RM_OPAQUE,
							renderContext );
		objectRenderables.clear();
	}

	{
		if( m_velocityMap )
		{
			Tr2PushPopRT rt( *m_velocityMap, renderContext, 1 );
			RenderOpaqueBatches( m_primaryBatches, renderContext );
		}
		else
		{
			RenderOpaqueBatches( m_primaryBatches, renderContext );
		}
	}
	RenderObjectsReceivingShadows( m_frameData.objectsReceivingShadow, true, renderContext );

	SetNoShadow();
	ApplyPerFrameData( renderContext );

	if( !m_hasDepthPass )
	{
		renderContext.SetReadOnlyDepth( true );
	}
	RenderTransparentBatches( m_primaryBatches, renderContext );
	RenderDistortionBatches( m_primaryBatches, renderContext );

	//GPU particles
	if( GetGpuParticleSystem() )
	{
		GetGpuParticleSystem()->Update( m_updateTime, m_updateContext.GetOriginShift(), renderContext );
		GetGpuParticleSystem()->Render( renderContext );
	}

	renderContext.SetReadOnlyDepth( false );

	renderContext.m_esm.EndManagedRendering();
}

// --------------------------------------------------------------------------------------
// Description:
//   Render lensflares. Clear primary batches and frame dependant object lists.
// --------------------------------------------------------------------------------------
void EveSpaceScene::EndRender( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_display )
	{
		return;
	}

	renderContext.AddGpuMarker( __FUNCTION__ );

	Tr2QuadRenderer::Instance()->DoneRendering( renderContext );

	renderContext.m_esm.BeginManagedRendering();
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );

	PopulatePerFramePSData( m_perFramePS );
	PopulatePerFrameVSData( m_perFrameVS );
	ApplyPerFrameData( renderContext );


	// Set up the frustum for visibility checking.
	TriFrustum& frustum = m_frameData.frustum;	
	// collect visible lensflares
	std::vector<ITr2Renderable*> visible;

	PopulatePerFramePSData( m_perFramePS );
	PopulatePerFrameVSData( m_perFrameVS );
	ApplyPerFrameData( renderContext );

	if( !visible.empty() )
	{
		renderContext.SetReadOnlyDepth( true );
		RenderRenderables(	visible, 
							m_secondaryBatches[TRIBATCHTYPE_TRANSPARENT], 
							TRIBATCHTYPE_TRANSPARENT, 
							Tr2EffectStateManager::RM_ALPHA,
							renderContext );
		RenderRenderables(	visible, 
							m_secondaryBatches[TRIBATCHTYPE_ADDITIVE], 
							TRIBATCHTYPE_ADDITIVE, 
							Tr2EffectStateManager::RM_ALPHA_ADDITIVE,
							renderContext );
		renderContext.SetReadOnlyDepth( false );
		visible.clear();
	}

	// lensflares
	if( !m_lensflares.empty() )
	{
		// at first, do all the occlusion queries
		for( auto it = m_lensflares.cbegin(); it != m_lensflares.cend(); ++it )
		{
			(*it)->RunOcclusionQueries( renderContext, frustum );
		}
		// lensflares
		for( auto it = m_lensflares.cbegin(); it != m_lensflares.cend(); ++it )
		{
			(*it)->GetRenderables( frustum, visible );
		}

		if( !visible.empty() )
		{
			renderContext.SetReadOnlyDepth( true );
			RenderRenderables(	visible, 
								m_secondaryBatches[TRIBATCHTYPE_ADDITIVE], 
								TRIBATCHTYPE_ADDITIVE, 
								Tr2EffectStateManager::RM_ALPHA_ADDITIVE,
								renderContext );
			renderContext.SetReadOnlyDepth( false );
		}
	}

	renderContext.m_esm.EndManagedRendering();

	// Clear shadow object list
	m_frameData.objectsReceivingShadow.clear();
	// Clear primary batches
	ClearBatches( m_primaryBatches );

	if( m_displayShadowMap )
	{
		if( m_shadowMap && m_shadowMap->GetTexture().IsValid() )
		{
			Tr2Renderer::DrawTexture( m_shadowMap->GetTexture() );
		}
	}

	float xOffset = m_xProjOffset;
	float yOffset = m_yProjOffset;
	TAAOffset();

	Matrix currentProj = m_frameData.projection;
	currentProj = EveCamera::AddCenterOffset( currentProj, m_xProjOffset-xOffset, m_yProjOffset-yOffset, Tr2Renderer::GetFrontClip(), Tr2Renderer::GetBackClip() );

	m_viewProjectLast = Tr2Renderer::GetViewTransform() * currentProj; 
	ClearVariableStore();

	if( m_visualizerEffects[m_visualizeMethod].type == VisualizerEffect::FULL_SCREEN_QUAD_OVERLAY )
	{
		Tr2Renderer::DrawTexture( m_visualizerEffects[m_visualizeMethod].effect, Vector2( 0, 0 ), Vector2( 1, 1 ) );
	}
}

// --------------------------------------------------------------------------------------
void EveSpaceScene::Render3DUI( Tr2RenderContext& renderContext )
{
	renderContext.AddGpuMarker( __FUNCTION__ );

	RenderDebugInfo( renderContext );

	Matrix identity = IdentityMatrix();
	std::vector<ITr2Renderable*> renderables;
	Tr2RenderableSortList transparentObjects;

	TriFrustum& frustum = m_frameData.frustum;
	frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &Tr2Renderer::GetViewPosition(), &Tr2Renderer::GetProjectionTransform(), Tr2Renderer::GetViewport() );
	
	renderContext.m_esm.BeginManagedRendering();
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
	
	PopulatePerFramePSData( m_perFramePS );
	PopulatePerFrameVSData( m_perFrameVS );
	
	Tr2ParallelDo( m_uiObjects.begin(), m_uiObjects.end(), [&]( IEveSpaceObject2* obj ) { obj->UpdateVisibility( m_frameData.frustum, identity ); } );

	for( auto it = m_uiObjects.begin(); it != m_uiObjects.end(); ++it )
	{
		IEveSpaceObject2* obj = *it;
		obj->GetRenderables( renderables, nullptr );
	}

	GetAllBatchesFromRenderables( renderables, transparentObjects, m_secondaryBatches );
	PrepareTransparentBatch( transparentObjects, m_secondaryBatches );
	
	UpdateQuadRenderer( frustum, m_uiObjects, renderContext );
	Tr2QuadRenderer::Instance()->GetBatches( TRIBATCHTYPE_OPAQUE, m_secondaryBatches[TRIBATCHTYPE_OPAQUE] );
	Tr2QuadRenderer::Instance()->GetBatches( TRIBATCHTYPE_ADDITIVE, m_secondaryBatches[TRIBATCHTYPE_ADDITIVE] );
	
	FinalizeBatches( m_secondaryBatches );
	UpdateVariableStore();

	SetNoShadow();
	ApplyPerFrameData( renderContext );

	// --------------------------------------------------------------------------------------
	RenderOpaqueBatches( m_secondaryBatches, renderContext );

	renderContext.SetReadOnlyDepth( true );
	RenderTransparentBatches( m_secondaryBatches, renderContext );
	renderContext.SetReadOnlyDepth( false );
	
	Tr2QuadRenderer::Instance()->DoneRendering( renderContext );

	renderContext.m_esm.EndManagedRendering();
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );

	ClearBatches( m_secondaryBatches );
}

void EveSpaceScene::PopulateAndApplyPerFrameData( Tr2RenderContext& renderContext ) 
{
	PopulatePerFramePSData( m_perFramePS );
	PopulatePerFrameVSData( m_perFrameVS );
	ApplyPerFrameData( renderContext );
}

void EveSpaceScene::Render( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.SetInvertedDepthTest( true );
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.SetInvertedDepthTest( false ); } );

	BeginRender( renderContext );
	RenderBackgroundPass( renderContext );
	//RenderDepthPass( renderContext );
	RenderMainPass( renderContext );
	EndRender( renderContext );
}

ITr2MultiPassScene::RenderPassResult EveSpaceScene::RenderPass( PassType pass, Tr2RenderContext& renderContext )
{
	renderContext.m_esm.SetInvertedDepthTest( true );
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.SetInvertedDepthTest( false ); } );

	switch( pass )
	{
	case RP_BEGIN_RENDER:
		BeginRender( renderContext );
		break;
	case RP_END_RENDER:
		EndRender( renderContext );
		break;
	case RP_BACKGROUND_RENDER:
		if( !RenderBackgroundPass( renderContext ) && m_distortionMap )
		{
			return PASS_RESULT_TERMINATE;
		}
		break;
	case RP_MAIN_RENDER:
		RenderMainPass( renderContext );
		break;
	case RP_DEPTH_PASS:
		RenderDepthPass( renderContext );
		break;
	case RP_SET_PERFRAME_DATA:
		PopulateAndApplyPerFrameData( renderContext );
		break;
	case RP_RENDER_UI:
		Render3DUI( renderContext );
		break;
	default:
		break;
	}
	return PASS_RESULT_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Clear all batches in the batch map
// Arguments:
//   batches - batches to clear
// --------------------------------------------------------------------------------------
void EveSpaceScene::ClearBatches( BatchMap& batches )
{
	for( auto it = batches.begin(); it != batches.end(); ++it )
	{
		it->second->Clear();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Finalize all batches in the batch map
// Arguments:
//   batches - batches to finalize
// --------------------------------------------------------------------------------------
void EveSpaceScene::FinalizeBatches( BatchMap& batches )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = batches.begin(); it != batches.end(); ++it )
	{
		it->second->Finalize();
	}
}

void EveSpaceScene::UpdateVariableStore()
{
	m_envMap1Var = m_envMap1;
	m_envMap2Var = m_envMap2;
	m_reflectionMapVar = m_envMap1;
	m_reflectionMaskMapVar = m_envMap2;
	m_nebulaBrightnessOverrideVar = m_nebulaBrightnessOverride <= 0.0f ? 1.0f : m_nebulaBrightnessOverride;

	// the environment cubemap (aka nebula) is passed theough the global variable store
	m_envMapHandle->SetValue( m_envMapTextureRes );
}

void EveSpaceScene::ClearVariableStore()
{
	m_envMap1Var.Clear();
	m_envMap2Var.Clear();
	m_reflectionMapVar.Clear();
	m_reflectionMaskMapVar.Clear();

	// shadowmap: clear store here
	if( m_shadowMap )
	{
		m_shadowMap->ClearVariableStore();
	}
}


void EveSpaceScene::PopulatePerFrameVSData( PerFrameVSData &data )
{
	// column_major for shaders
	data.ViewMat = Transpose( Tr2Renderer::GetViewTransform() );
	Matrix proj = Tr2Renderer::GetReversedDepthProjectionTransform();
	data.ProjectionMat = Transpose( proj );
	Matrix viewProject( Tr2Renderer::GetViewTransform() * proj );
	data.ViewProjectionMat = Transpose( viewProject );
	// attention: need the transposed, but shader also needs column_major, so it is transpose(transpose(m)) == m
	data.ViewInverseTransposeMat = Tr2Renderer::GetInverseViewTransform();
	
#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 || TRINITY_PLATFORM==TRINITY_OPENGL4 )
	data.ViewProjectionLast = Transpose( m_viewProjectLast );
#endif

	// each scene has a nebula and that can be rotated and inverted (via scaling)
	data.EnvMapRotationMat = Transpose( RotationMatrix( m_envMapRotation ) );

	// sun data
	data.Sun = m_sunData;
	data.Sun.DiffuseColor = m_useSunColorWithDynamicLights && g_eveSpaceSceneDynamicLighting ? m_sunColorWithDynamicLights : m_sunColor;

	// make sure whatever direction we get in here, it is normalized! And inverted: Shaders work with direction to light...
	data.Sun.DirWorld = -Normalize( data.Sun.DirWorld );

	// resolution of rendertarget
	data.TargetResolution.x = (float)Tr2Renderer::GetRenderTargetWidth();
	data.TargetResolution.y = (float)Tr2Renderer::GetRenderTargetHeight();

	// fov in both ways: width (x) and (height (y)
	data.FovXY.y = EveCamera::CalculateFovFromProjection( Tr2Renderer::GetProjectionTransform() );
	data.FovXY.x = data.FovXY.y * Tr2Renderer::GetAspectRatio();

    float distance = m_fogEnd - m_fogStart;
    if( fabsf( distance ) < 1e-5f )
    {
        distance = 1e-5f;
    }
    data.FogFactors = Vector3( m_fogEnd / distance, 1.f / distance, m_fogMax );
	
	// Derived from SetupViewport in Tr2Renderer.cpp
	const Tr2Viewport& deviceViewport = Tr2Renderer::GetDeviceViewport();
	const TriViewport& viewport = Tr2Renderer::GetViewport();
	data.ViewportAdjustment.x = viewport.x < 0 ? -1.0f : 1.0f;
	data.ViewportAdjustment.y = viewport.y + viewport.height > (int)Tr2Renderer::GetRenderTargetHeight() ? -1.0f : 1.0f;
	data.ViewportAdjustment.z = deviceViewport.m_width  / viewport.width;
	data.ViewportAdjustment.w = deviceViewport.m_height / viewport.height;
	data.Time = Tr2Renderer::GetAnimationTime();

	data.ViewportSize.x = Tr2Renderer::GetDeviceViewport().m_width;
	data.ViewportSize.y = Tr2Renderer::GetDeviceViewport().m_height;
}

void EveSpaceScene::PopulatePerFramePSData( PerFramePSData &data )
{
	// column_major for shaders
	data.ViewMat = Transpose( Tr2Renderer::GetViewTransform() );
	// attention: need the transposed, but shader also needs column_major, so it is transpose(transpose(m)) == m
	data.ViewInverseTransposeMat = Tr2Renderer::GetInverseViewTransform();
	
	// each scene has a nebula and that can be rotated
	data.EnvMapRotationMat = Transpose( RotationMatrix( m_envMapRotation ) );

	data.Sun = m_sunData;
	data.Sun.DiffuseColor = m_useSunColorWithDynamicLights && g_eveSpaceSceneDynamicLighting ? m_sunColorWithDynamicLights : m_sunColor;
	// make sure whatever direction we get in here, it is normalized! And inverted: Shaders work with direction to light...
	data.Sun.DirWorld = -Normalize( data.Sun.DirWorld );
	data.AmbientColor = Vector3( m_ambientColor.r, m_ambientColor.g, m_ambientColor.b );
	data.NebulaIntensity = m_nebulaBrightnessOverride > 0.0f ? m_nebulaBrightnessOverride : m_nebulaIntensity;
	data.FogColor = Vector4( m_fogColor.r, m_fogColor.g, m_fogColor.b, m_fogMax );

	// ps gamma brightness
	data.GammaBrightness = g_eveSpaceSceneGammaBrightness;

	// resolution of rendertarget
	data.TargetResolution.x = (float)Tr2Renderer::GetRenderTargetWidth();
	data.TargetResolution.y = (float)Tr2Renderer::GetRenderTargetHeight();

	// fov in both ways: width (x) and (height (y)
	data.FovXY.y = EveCamera::CalculateFovFromProjection( Tr2Renderer::GetProjectionTransform() );
	data.FovXY.x = data.FovXY.y * Tr2Renderer::GetAspectRatio();

	// disable shadows per default
	data.ShadowCameraRange = Vector2( 1.f, 0.f );

	data.ViewportOffset.x = (float)Tr2Renderer::GetViewport().x;
	data.ViewportOffset.y = (float)Tr2Renderer::GetViewport().y;

	data.ViewportSize.x = Tr2Renderer::GetDeviceViewport().m_width;
	data.ViewportSize.y = Tr2Renderer::GetDeviceViewport().m_height;

	data.Time = Tr2Renderer::GetAnimationTime();
	if( m_shadowMap )
	{
		data.ShadowMapSettings = m_shadowMap->GetShadowMapSettings();
	}
	else
	{
		data.ShadowMapSettings = Vector4( 1.f, 1.f, 0.f, 0.f );
	}
	data.ShadowLightness = 0;
	data.DepthMapSampleCount = float( m_msaaSamples );

	const Matrix& projection = Tr2Renderer::GetReversedDepthProjectionTransform();
	data.ProjectionToView.x = projection._43;
	data.ProjectionToView.y = projection._33;

	data.Debug = m_perFrameDebug;
}

bool EveSpaceScene::Initialize()
{
	// the environment cubemap aka the nebula
	if( m_envMapHandle )
	{
		BeResMan->GetResource( m_envMapResPath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_envMapTextureRes );
	}

	if( !m_envMap1ResPath.empty() )
	{
		BeResMan->GetResource( m_envMap1ResPath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_envMap1 );
	}
	if( !m_envMap2ResPath.empty() )
	{
		BeResMan->GetResource( m_envMap2ResPath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_envMap2 );
	}
	if( !m_envMap3ResPath.empty() )
	{
		BeResMan->GetResource( m_envMap3ResPath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_envMap3 );
	}

	if( m_shLightingManager )
	{
		Tr2ShLightingManagerPtr manager = m_shLightingManager;
		m_shLightingManager = nullptr;
		SetShLightingManager( manager );
	}

	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		( *it )->RegisterWithQuadRenderer( *Tr2QuadRenderer::Instance() );
	}
	m_cameraAttachmentParent->RegisterWithQuadRenderer( *Tr2QuadRenderer::Instance() );


	for( auto it = begin( m_uiObjects ); it != end( m_uiObjects ); ++it )
	{
		( *it )->RegisterWithQuadRenderer( *Tr2QuadRenderer::Instance() );
	}

	return true;
}

bool EveSpaceScene::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_envMapResPath ) )
	{
		m_envMapTextureRes.Unlock();
		if( !m_envMapResPath.empty() )
		{
			BeResMan->GetResource( m_envMapResPath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_envMapTextureRes );
		}
	}
	if( IsMatch( value, m_envMap1ResPath ) )
	{
		m_envMap1.Unlock();
		if( !m_envMap1ResPath.empty() )
		{
			BeResMan->GetResource( m_envMap1ResPath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_envMap1 );
		}
	}
	if( IsMatch( value, m_envMap2ResPath ) )
	{
		m_envMap2.Unlock();
		if( !m_envMap2ResPath.empty() )
		{
			BeResMan->GetResource( m_envMap2ResPath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_envMap2 );
		}
	}
	if( IsMatch( value, m_envMap3ResPath ) )
	{
		m_envMap3.Unlock();
		if( !m_envMap3ResPath.empty() )
		{
			BeResMan->GetResource( m_envMap3ResPath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_envMap3 );
		}
	}

	return true;
}

Tr2ShLightingManagerPtr EveSpaceScene::GetShLightingManager() const
{
	return m_shLightingManager;
}

void EveSpaceScene::SetShLightingManager( Tr2ShLightingManager* manager )
{
	if( m_shLightingManager == manager )
	{
		return;
	}
	if( m_shLightingManager )
	{
		for( auto it = m_planets.begin(); it != m_planets.end(); ++it )
		{
			ITr2SecondaryLightSourcePtr lightSource = BlueCastPtr( *it );
			if( lightSource )
			{
				lightSource->UnregisterSecondaryLightSource( *m_shLightingManager );
			}
		}
		for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
		{
			ITr2SecondaryLightSourcePtr lightSource = BlueCastPtr( *it );
			if( lightSource )
			{
				lightSource->UnregisterSecondaryLightSource( *m_shLightingManager );
			}
			ITr2ShLightingReceiverPtr receiver = BlueCastPtr( *it );
			if( receiver )
			{
				receiver->ClearShLighting();
			}
		}
	}
	m_shLightingManager = manager;
	if( m_shLightingManager )
	{
		for( auto it = m_planets.begin(); it != m_planets.end(); ++it )
		{
			ITr2SecondaryLightSourcePtr lightSource = BlueCastPtr( *it );
			if( lightSource )
			{
				lightSource->RegisterSecondaryLightSource( *m_shLightingManager );
			}
		}
		for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
		{
			ITr2SecondaryLightSourcePtr lightSource = BlueCastPtr( *it );
			if( lightSource )
			{
				lightSource->RegisterSecondaryLightSource( *m_shLightingManager );
			}
		}
	}
}

void EveSpaceScene::OnListModified(
	long event,		// BLUELISTEVENT values
	ssize_t key,
	ssize_t key2,
	IRoot* value,
	const struct IList* theList )
{
	switch( event & BELIST_EVENTMASK )
	{
	case BELIST_UNLOADSTART:
		if( m_shLightingManager )
		{
			for( ssize_t i = 0; i < theList->GetSize(); ++i )
			{
				if( ITr2SecondaryLightSourcePtr lightSource = BlueCastPtr( theList->GetAt( i ) ) )
				{
					lightSource->UnregisterSecondaryLightSource( *m_shLightingManager );
				}
				if( ITr2ShLightingReceiverPtr receiver = BlueCastPtr( theList->GetAt( i ) ) )
				{
					receiver->ClearShLighting();
				}
			}
		}
		break;
	case BELIST_INSERTED:
		{
			if( m_shLightingManager )
			{
				ITr2SecondaryLightSourcePtr lightSource = BlueCastPtr( value );
				if( lightSource )
				{
					lightSource->RegisterSecondaryLightSource( *m_shLightingManager );
				}
			}

			IEveSpaceObject2Ptr spaceObject = BlueCastPtr( value );
			if( spaceObject )
			{
				spaceObject->RegisterWithQuadRenderer( *Tr2QuadRenderer::Instance() );
			}
		}
		break;
	case BELIST_REMOVED:
		{
			if( m_shLightingManager )
			{
				ITr2SecondaryLightSourcePtr lightSource = BlueCastPtr( value );
				if( lightSource )
				{
					lightSource->UnregisterSecondaryLightSource( *m_shLightingManager );
				}
				ITr2ShLightingReceiverPtr receiver = BlueCastPtr( value );
				if( receiver )
				{
					receiver->ClearShLighting();
				}
			}
		}
		break;
	}
}

IRoot* EveSpaceScene::PickObject( int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport,
								 Be::OptionalWithDefaultValue<Tr2PickTypes, PICK_TYPE_PICKING | PICK_TYPE_OPAQUE> filter )
{
	unsigned int id;
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return PickObjectAndArea( x, y, proj, view, viewport, id, filter, renderContext );
}

IRoot* EveSpaceScene::PickObjectAndArea( int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, unsigned int& areaID, Tr2PickTypes pickTypes, Tr2RenderContext& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return nullptr;
	}

	// Backup current state
	Tr2Renderer::PushProjection();
	ON_BLOCK_EXIT( Tr2Renderer::PopProjection );
	Tr2Renderer::PushViewTransform();
	ON_BLOCK_EXIT( Tr2Renderer::PopViewTransform );
	Tr2Renderer::PushViewport();
	ON_BLOCK_EXIT( Tr2Renderer::PopViewport );

	float fx,fy;
	Vector3 startWorld;
	Vector3 dirWorld;
	gTriDev->ScreenToProjection( x, y, &fx, &fy, viewport );
	IRoot* result = NULL;

	// Get view and projection transforms
	Matrix projTransform;
	proj->GetMatrixWithoutViewAdjustment( projTransform );
	const Matrix& viewTransform = view->GetTransform();

	ConvertProjectionCoordToWorldPickRay( fx, fy, &projTransform, &viewTransform, &startWorld, &dirWorld );

	float dist = HUGE_NUMBER;

	// Render for picking, limit our view to the pick ray
	SetupTransformsForPicking( fx, fy, proj, view, viewport, renderContext );


	Tr2DebugObjectReference gizmo = nullptr;
	float gizmoDepth = 0;
	if( m_debugRenderer )
	{
		gizmo = m_debugRenderer->Pick( gizmoDepth, renderContext );
	}

	// Find objects inside our 1-by-1 pick frustum
	std::vector<std::pair<ITr2Pickable*, ITr2Renderable*>> collisionSet;
	std::vector<ITr2Renderable*> visibleObjects;
	GetPickingObjectsToRender( visibleObjects );

	// Collect vector of objects to render
	for( std::vector<ITr2Renderable*>::const_iterator it = visibleObjects.begin(); it != visibleObjects.end(); ++it )
	{		
		ITr2PickablePtr pickedObj( BlueCastPtr( *it ) );
		if( pickedObj )
		{
			collisionSet.push_back( std::make_pair( pickedObj, *it ) );
		}
	}


	if ( !collisionSet.empty() )
	{
		renderContext.m_esm.SetInvertedDepthTest( true );
		ON_BLOCK_EXIT( [&] { renderContext.m_esm.SetInvertedDepthTest( false ); } );

		renderContext.m_esm.BeginManagedRendering();
		ON_BLOCK_EXIT( [&]{ renderContext.m_esm.EndManagedRendering(); } );
		
		CR_RETURN_VAL( Tr2Renderer::BeginRenderContext(), nullptr );
		ON_BLOCK_EXIT( [&]{ Tr2Renderer::EndRenderContext(); } );

		float initialDepth = (dist - Tr2Renderer::GetFrontClip()) / (Tr2Renderer::GetBackClip() - Tr2Renderer::GetFrontClip());
		initialDepth = std::max( 0.0f, std::min( 1.0f, initialDepth ) );

		unsigned short objId = 0xffff;
		unsigned short aId = 0xffff;

		if ( m_pickBuffer.BeginRendering( std::max( gizmoDepth, 1 - initialDepth ), renderContext ) )
		{
			for ( unsigned int i = 0; i < collisionSet.size(); i++ )
			{
				// We cannot rely on the object data to be up-to-date because this would assume that all
				// objects in the picked list were rendered on the previous frame and that is tooooo much of an assumption.
				// <halldor 2008-04-23>

				ITr2Renderable* renderable = collisionSet[i].second;
				ITr2Pickable* pickable = collisionSet[i].first;

				Tr2PerObjectData* perObjectData = renderable->GetPerObjectData( m_pickingBatches );
				if( perObjectData )
				{
					perObjectData->SetUserData( i );
				}
				
				// We always pick against the opaque geometry that's rendered
				if( pickTypes != PICK_TYPE_PICKING )
				{
					pickable->GetPickingBatches( m_pickingBatches, pickTypes & ~PICK_TYPE_PICKING, perObjectData);
				}
				// Additionally, we can pick against geometry that's only rendered for picking,
				// allowing us to put placeholders in for things that are partly transparent, but still should be pickable
				if( ( pickTypes & PICK_TYPE_PICKING ) != 0 )
				{
					pickable->GetPickingBatches( m_pickingBatches, PICK_TYPE_PICKING, perObjectData);
				}
			}

			Tr2Renderer::SetWorldTransform( IdentityMatrix() );

			m_pickingBatches->Finalize();

			if( m_pickingBatches != NULL )
			{
				renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_PICKING );
				renderContext.RenderBatchesForPicking( m_pickingBatches, BlueSharedString( "Picking" ) );
			}

			if( m_pickBuffer.EndRendering( renderContext ) )
			{
				GetPickingResults( m_pickBuffer, renderContext, objId, aId, dist );
			}

			m_pickingBatches->Clear();
		}

		if( objId < collisionSet.size() )
		{
			result = collisionSet[objId].first->GetID( aId );
			areaID = aId;
		}
		else if( gizmo )
		{
			result = gizmo.m_object;
			areaID = gizmo.m_area;
		}
	}

	return result;
}


void EveSpaceScene::SetupTransformsForPicking( float fx, float fy, TriProjection* proj,  TriView* view, TriViewport* viewport, Tr2RenderContext& renderContext )
{
	Tr2Renderer::SetViewTransform( view->GetTransform() );
	proj->SetProjection();
	// scale up the projection so we have 1 pixel rendered to the viewport
	Vector2 scaling( float(viewport->width), float(viewport->height) );
	// translate the projection so that we center around the pick ray origin,
	// while remembering to scale this value as well:
	Vector2 translation;
	translation.x = -fx*scaling.x;
	translation.y = -fy*scaling.y;
	Tr2Renderer::AdjustProjection( scaling, translation );

	EveSpaceScene::PerFramePSData perFramePS;
	EveSpaceScene::PerFrameVSData perFrameVS;

	EveSpaceScene::PopulatePerFramePSData( perFramePS );
	EveSpaceScene::PopulatePerFrameVSData( perFrameVS );

	// set the rendertarget resolution, which always 1x1 for icking
	perFrameVS.TargetResolution.x = perFrameVS.TargetResolution.y = 1.f;

	static const unsigned perFrameVsMask = 
		( 1 << VERTEX_SHADER )					|
		SHADER_TYPE_EXISTS( COMPUTE_SHADER )	|
		SHADER_TYPE_EXISTS( GEOMETRY_SHADER )	|
		SHADER_TYPE_EXISTS( HULL_SHADER )		|
		SHADER_TYPE_EXISTS( DOMAIN_SHADER)		;
	FillAndSetConstants( m_perFrameVSBuffer, &perFrameVS, sizeof( perFrameVS ), perFrameVsMask, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
	FillAndSetConstants( m_perFramePSBuffer, &perFramePS, sizeof( perFramePS ), PIXEL_SHADER , Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
}

void EveSpaceScene::GetPickingObjectsToRender( std::vector<ITr2Renderable*>& pickableRenderObjects )
{
	TriFrustum pickFrustum;

	pickableRenderObjects.clear();

	// Read the view matrix, projection matrix and camera position from the device,
	// assuming they were just set by PickObject above. The reason I'm not passing it
	// down is that GetViewPosition() is using the view's inverse matrix which I'd
	// prefer not to recalculate.
	const Vector3& camPos = Tr2Renderer::GetViewPosition();
	Matrix proj           = Tr2Renderer::GetProjectionTransform();
	const Matrix& view    = Tr2Renderer::GetViewTransform();

	pickFrustum.DeriveFrustum(
		&view, &camPos, &proj, gTriDev->mViewport
	);

	for( IEveSpaceObject2Vector::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		if( ( *it )->IsPickable() )
		{
			// Note: Here we are relying on the EveSpaceObject's GetRenderables
			// function to decide what objects are pickable in the given frustum.
			// This is not necessarily what we want, as some objects might be
			// renderable but not pickable (particle clouds?)
			( *it )->UpdateVisibility( pickFrustum, IdentityMatrix() );
			( *it )->GetRenderables( pickableRenderObjects, nullptr );
		}
	}
	m_cameraAttachmentParent->UpdateVisibility( pickFrustum, IdentityMatrix() );
	m_cameraAttachmentParent->GetRenderables( pickableRenderObjects, nullptr );

}

void EveSpaceScene::UpdatePlanets( EveUpdateContext& updateContext )
{
	// The planet view matrix must be set up for update to accommodate
	// potential transform modifiers in the transform hierarchy of the
	// planets.
	Matrix orgViewMatrix = SetupPlanetViewMatrix();

	for( EvePlanetVector::iterator it = m_planets.begin(); it != m_planets.end(); ++it )
	{
		(*it)->Update( updateContext );
	}

	Tr2Renderer::SetViewTransform( orgViewMatrix );
}

void EveSpaceScene::RenderPlanets( Tr2RenderContext& renderContext )
{
	// Backup current state
	Tr2Renderer::PushProjection();
	ScopeGuard guardPopProjection = MakeGuard( Tr2Renderer::PopProjection );
	Tr2Renderer::PushViewTransform();
	ScopeGuard guardPopViewTransform = MakeGuard( Tr2Renderer::PopViewTransform );

	Matrix orgViewMatrix = SetupPlanetViewMatrix();
	Matrix planetProjection = EveCamera::ModifyClipPlanes( Tr2Renderer::GetProjectionTransform(), 0.01f, 1e5f );
	Tr2Renderer::SetProjectionTransform( planetProjection );
	const Matrix& identity = IdentityMatrix();

	// Planets are rendered with a custom frustum
	TriFrustum frustum;
	frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &Tr2Renderer::GetViewPosition(), &Tr2Renderer::GetProjectionTransform(), gTriDev->mViewport );

	{
		CCP_STATS_ZONE( "UpdateVisibility Planets" );
		Tr2ParallelDo( m_planets.begin(), m_planets.end(), [&]( EvePlanet* obj ) { obj->SetRenderScale( m_planetScale ); obj->UpdateVisibility( frustum, identity ); } );
	}
	std::vector<ITr2Renderable*> planetRenderables;
	for( EvePlanetVector::iterator it = m_planets.begin(); it != m_planets.end(); ++it )
	{
		EvePlanet* obj = *it;
		obj->GetRenderables( planetRenderables );
	}

	if( planetRenderables.empty() )
	{
		return;
	}

	// Need to populate per frame data again as view/projection matrices changed
	PopulatePerFramePSData( m_perFramePS );
	PopulatePerFrameVSData( m_perFrameVS );
	ApplyPerFrameData( renderContext );

	Tr2RenderableSortList renderablesWithTransparencies;
	GetAllBatchesFromRenderables( planetRenderables, renderablesWithTransparencies, m_secondaryBatches );
	PrepareTransparentBatch( renderablesWithTransparencies, m_secondaryBatches );
	FinalizeBatches( m_secondaryBatches );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DEPTH_ONLY );
	renderContext.RenderBatches( m_secondaryBatches[TRIBATCHTYPE_DEPTH], BlueSharedString( "Depth" ) );
	RenderOpaqueBatches( m_secondaryBatches, renderContext );

	renderContext.SetReadOnlyDepth( true );
	RenderTransparentBatches( m_secondaryBatches, renderContext );
	renderContext.SetReadOnlyDepth( false );
	ClearBatches( m_secondaryBatches );

	// Put view/projection back to normal
	Tr2Renderer::PopProjection();
	guardPopProjection.Dismiss();
	
	Tr2Renderer::PopViewTransform();
	guardPopViewTransform.Dismiss();

	PopulatePerFramePSData( m_perFramePS );
	PopulatePerFrameVSData( m_perFrameVS );
	ApplyPerFrameData( renderContext );
}

Matrix EveSpaceScene::SetupPlanetViewMatrix()
{
	Matrix orgViewMatrix = Tr2Renderer::GetViewTransform();
	Matrix planetViewMatrix = orgViewMatrix;
	const float planetScale = 1.0f / m_planetCameraScale;
	Vector3& viewPos = planetViewMatrix.GetTranslation();
	viewPos *= planetScale;
	Tr2Renderer::SetViewTransform( planetViewMatrix );

	return orgViewMatrix;
}

void EveSpaceScene::SetNoShadow()
{
	// disable shadows by useing a blank, empty depthmap
	if( m_shadowMap )
	{
		m_shadowMap->SetShadowTexture( true );
	}

	m_perFramePS.ShadowCameraRange = Vector2( 1.f, 0.f );
}

void EveSpaceScene::GetPickingResults( Tr2PickBuffer& pickBuffer, Tr2RenderContext& renderContext, 
									   unsigned short& objId, 
					                   unsigned short& areaId, float& depth )
{
	const void* data;
	uint32_t pitch;
	if( pickBuffer.PrepareGetResults( data, pitch, renderContext ) )
	{
		DecodeBufferPixel( data, objId, areaId, depth );
		pickBuffer.UnlockBuffer( renderContext );
	}
}

void EveSpaceScene::DecodeBufferPixel( const void* pBuffer, unsigned short& objId, 
									   unsigned short& areaId, float& depth ) const
{
	// helpers: get each channel
	unsigned int a = (unsigned int)(*((unsigned char*)pBuffer + 3));
	unsigned int r = (unsigned int)(*((unsigned char*)pBuffer + 2));
	unsigned int g = (unsigned int)(*((unsigned char*)pBuffer + 1));
	unsigned int b = (unsigned int)(*((unsigned char*)pBuffer + 0));
	// put it "together"
	objId = (unsigned short)(((r & 0xff) << 8) | (g & 0xff));
	objId--;
	areaId = (unsigned short)(((b & 0xff) << 8) | (a & 0xff));
	areaId--;
	// sorry, no depth anymore
	depth = 0.f;
}

bool EveSpaceScene::IsMeshUnloadingEnabled()
{
	return g_eveIsSpaceObjectResourceUnloadingEnabled;
}

void EveSpaceScene::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	if( m_debugRenderer )
	{
		m_debugRenderer->BeginRender();
		auto bkProjection = Tr2Renderer::GetProjectionRawTransform();
		Tr2Renderer::SetProjectionTransform( Tr2Renderer::GetReversedDepthProjectionTransform() );
		ON_BLOCK_EXIT( [&] { Tr2Renderer::SetProjectionTransform( bkProjection ); } );

		for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
		{
			if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
			{
				renderable->RenderDebugInfo( *m_debugRenderer );
			}
		}

		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( m_cameraAttachmentParent.p ) )
		{
			renderable->RenderDebugInfo( *m_debugRenderer );
		}
		
		for( auto it = m_staticParticles.begin(); it != m_staticParticles.end(); ++it )
		{
			(*it)->RenderDebugInfo( *m_debugRenderer );
		}

		for( auto it = m_distanceFields.begin(); it != m_distanceFields.end(); ++it )
		{
			(*it)->RenderDebugInfo( *m_debugRenderer );
		}
		m_debugRenderer->EndRender( renderContext );

		Tr2Renderer::RenderDebugInfo( renderContext );
	}
}

Vector3 EveSpaceScene::PickInfinity( int x, int y, Matrix proj, Matrix view )
{
	float xp, yp;
	gTriDev->ScreenToProjection(x, y, &xp, &yp);

	// Get the startpoint in world coordinates and pick ray
	Vector3 dir;
	Vector3 startWorld;
	ConvertProjectionCoordToWorldPickRay(xp, yp, &proj, &view, &startWorld, &dir );

	return dir;
}

void EveSpaceScene::UpdateSceneFromScript( Be::Time time )
{
	Update( time, time );
}
