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
#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"
#include "TriFrustum.h"
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
#include "ITr2TextureProvider.h"
#include "Tr2ImpostorManager.h"
#include "Tr2DebugRenderer.h"
#include "EveEffectRoot2.h"
#include "Tr2ReflectionProbe.h"
#include "VirtualCamera/EveVirtualCameraSystem.h"
#include "Eve/EveEntity.h"
#include "Tr2SSAO.h"
#include "Lights/ITr2LightOwner.h"
#include "../Tr2VolumetricsRenderer.h"
#include "../Tr2BoneTransformBuffer.h"
#include <ScopedBlockTrap.h>
#include "Raytracing/Tr2RaytracingManager.h"
#include "../Resources/TriTextureRes.h"

using namespace Tr2RenderContextEnum;

class TriPoolAllocator;

CCP_STATS_DECLARE( shadowsRendered, "Trinity/EveSpaceScene/shadowsRendered", true, CST_COUNTER_LOW, "How many times are shadows rendered per frame?" );
CCP_STATS_DECLARE( raytracedShadowsTime, "Trinity/EveSpaceScene/raytracedShadowsTime", true, CST_TIME, "Time it took to set up raytraced shadows" );
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

int g_eveReflectionMode = EntityComponents::REFLECT_NEVER;
TRI_REGISTER_SETTING( "eveReflectionSetting", g_eveReflectionMode );

bool g_eveSpaceSceneRaytracedShadows = true;
TRI_REGISTER_SETTING( "eveSpaceSceneRaytracedShadows", g_eveSpaceSceneRaytracedShadows );

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

	ShadowReceiver( float es, IEveSpaceObject2* obj ) :
		estimatedSize( es ),
		object( obj )
	{
	}
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
	m_shadowQuality( SHADOW_RAYTRACED ),
	m_enableShadows( true ),
	m_displayShadowMap( false ),
	m_shadowView( IdentityMatrix() ),
	m_enableRaytracing( false ),
	m_visualizeMethod( VM_NONE ),
	m_perFrameDebug( 0.f ),
	m_pickBuffer( NULL, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 1 ),
	m_envMapRotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_backgroundRenderingEnabled( false ),
	m_updateContext( 0 ),
	m_ssaoMapHandle( nullptr ),
	m_envMapHandle( NULL ),
	m_envMap1Var( "EnvMap1", m_envMap1 ),
	m_envMap2Var( "EnvMap2", m_envMap2 ),
	m_reflectionMapVar( "ReflectionMap", m_envMap1 ),
	m_reflectionMaskMapVar( "ReflectionMaskMap", m_envMap2 ),
	m_depthMapVar( "DepthMap", (ITr2TextureProvider*)nullptr ),
	m_envMapTransformVar( "EnvMapTransform", IdentityMatrix() ),
	m_reflectionMapTransformVar( "ReflectionMapTransform", IdentityMatrix() ),
	m_suncVecVar( "SunVec", Vector3( 0.0f, 0.0f, 1.0f ) ),
	m_nebulaIntensity( 1.f ),
	m_nebulaIntensityVar( "NebulaIntensity", m_nebulaIntensity ),
	m_planetScale( 1e6 ),
	m_planetCameraScale( 1e6 ),
	m_sunColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_sunColorWithDynamicLights( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_useSunColorWithDynamicLights( false ),
	m_reflectionIntensity( 1.35f ),
	m_reflectionBackLightingContrast( 8.0f ),
	m_reflectionBackLightingColor( 2.0f, 2.0f, 2.0f, 2.0f ),
	m_dynamicObjectReflectionEnabled( true ),
	m_hasBackgroundDistortionBatches( false ),
	m_hasForegroundDistortionBatches( false ),
	m_freezeFrustum( false ),
	m_velocityMapDirty( false ),
	m_virtualCameraSystem(),
	m_projection( IdentityMatrix() ),
	m_projectionLast( IdentityMatrix() ),
	m_jitteredProjection( IdentityMatrix() ),
	m_reprojectionMatrix( IdentityMatrix() ),
	m_jitter( 0.f, 0.f, 0.f, 0.f ),
	m_upscalingAmount( 1.0f ),
	m_usingUpscaling( false )
{
	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	m_primaryBatches[TRIBATCHTYPE_OPAQUE] = CCP_NEW( "EveSpaceScene/m_batches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_primaryBatches[TRIBATCHTYPE_DECAL] = CCP_NEW( "EveSpaceScene/m_decalBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_primaryBatches[TRIBATCHTYPE_DECAL_ADDITIVE] = CCP_NEW( "EveSpaceScene/m_decalAdditiveBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_primaryBatches[TRIBATCHTYPE_ADDITIVE] = CCP_NEW( "EveSpaceScene/m_additiveBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_primaryBatches[TRIBATCHTYPE_DISTORTION] = CCP_NEW( "EveSpaceScene/m_distortionBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_primaryBatches[TRIBATCHTYPE_TRANSPARENT] = CCP_NEW( "EveSpaceScene/m_sortedBatches" ) TriRenderBatchAccumulator<>( allocator );
	m_primaryBatches[TRIBATCHTYPE_DEPTH] = CCP_NEW( "EveSpaceScene/m_depthBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );

	m_secondaryBatches[TRIBATCHTYPE_OPAQUE] = CCP_NEW( "EveSpaceScene/m_batches2" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_secondaryBatches[TRIBATCHTYPE_DECAL] = CCP_NEW( "EveSpaceScene/m_decalBatches2" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_secondaryBatches[TRIBATCHTYPE_DECAL_ADDITIVE] = CCP_NEW( "EveSpaceScene/m_decalAdditiveBatches2" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_secondaryBatches[TRIBATCHTYPE_ADDITIVE] = CCP_NEW( "EveSpaceScene/m_additiveBatches2" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_secondaryBatches[TRIBATCHTYPE_DISTORTION] = CCP_NEW( "EveSpaceScene/m_distortionBatches2" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
	m_secondaryBatches[TRIBATCHTYPE_TRANSPARENT] = CCP_NEW( "EveSpaceScene/m_sortedBatches2" ) TriRenderBatchAccumulator<>( allocator );
	m_secondaryBatches[TRIBATCHTYPE_DEPTH] = CCP_NEW( "EveSpaceScene/m_depthBatches2" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );

	m_perThreadBatches[TRIBATCHTYPE_OPAQUE] = {};
	m_perThreadBatches[TRIBATCHTYPE_DECAL] = {};
	m_perThreadBatches[TRIBATCHTYPE_DECAL_ADDITIVE] = {};
	m_perThreadBatches[TRIBATCHTYPE_ADDITIVE] = {};
	m_perThreadBatches[TRIBATCHTYPE_DISTORTION] = {};
	m_perThreadBatches[TRIBATCHTYPE_TRANSPARENT] = {};
	m_perThreadBatches[TRIBATCHTYPE_DEPTH] = {};

	m_shadowAllocators.resize( SHADOW_FRUSTUM_COUNT );
	m_shadowBatches.resize( SHADOW_FRUSTUM_COUNT );
	for( unsigned int i = 0; i < SHADOW_FRUSTUM_COUNT; ++i )
	{
		m_shadowBatches[i].reset( new TriRenderBatchAccumulator<EffectKeyGenerator>( &m_shadowAllocators[i] ) );
	}

	// global textures
	// register variable handle to texture
	m_envMapHandle = GlobalStore().RegisterVariable( "EveSpaceSceneEnvMap", (ITr2TextureProvider*)nullptr );
	m_ssaoMapHandle = GlobalStore().RegisterVariable( "SSAOMap", (ITr2TextureProvider*)nullptr );
	GlobalStore().RegisterVariable( "BoneTransforms", &Tr2BoneTransformBuffer::GetInstance() );

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

	m_backgroundObjects.SetNotify( this );
	m_planets.SetNotify( this );
	m_objects.SetNotify( this );
	m_uiObjects.SetNotify( this );

	m_dataTextureMgr.CreateInstance();
	m_postProcessPSBuffer.CreateInstance();
	m_planetPerObjBuffer.CreateInstance();

	m_visualizerEffects[VW_LIGHT_COUNT].type = VisualizerEffect::FULL_SCREEN_QUAD_OVERLAY;

	Tr2LightManager::ResetVariableStore();

	m_cameraAttachmentParent.CreateInstance();
	m_reflectionProbe.CreateInstance();
	m_componentRegistry.CreateInstance();
	m_ssao.CreateInstance();

	m_volumetricsRenderer.CreateInstance();

	m_rtManager.CreateInstance();

	m_emptyShadowMap.CreateInstance();
	BeResMan->GetResource( "res:/texture/global/white.dds", "", m_emptyShadowMap );
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
    CCP_DELETE( m_pickingBatches );

	ClearComponentRegistry();
}

void EveSpaceScene::ReleaseResources( TriStorage s )
{
	// handles
	if( m_envMapHandle )
	{
		m_envMapHandle->Clear();
	}

	if( m_ssaoMapHandle )
	{
		m_ssaoMapHandle->Clear();
	}

	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		m_perFrameVSBuffer = Tr2ConstantBufferAL();
		m_perFramePSBuffer = Tr2ConstantBufferAL();
		m_shadowPerFrameVSBuffer = Tr2ConstantBufferAL();
	}
}

Tr2PostProcess2Ptr EveSpaceScene::GetPostProcess()
{
	if( !m_display )
	{
		return nullptr;
	}
	else
	{
		return m_postProcess;
	}
}

Tr2ShaderBufferPtr EveSpaceScene::GetPostProcessPSBuffer()
{
	return m_postProcessPSBuffer;
}

void EveSpaceScene::SetVelocityMap( Tr2RenderTargetPtr velocityMap )
{
	m_velocityMap = velocityMap;
}

bool EveSpaceScene::OnPrepareResources()
{
	return true;
}

void EveSpaceScene::Update( Be::Time realTime, Be::Time simTime )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		Tr2BoneTransformBuffer::GetInstance().SetFrameNumbers( renderContext.GetRecordingFrameNumber(), renderContext.GetRenderedFrameNumber() );
	}

	if( !m_update )
	{
		return;
	}

	m_updateContext.SetTime( simTime );
	m_updateContext.UpdateOrigin( m_ballpark );
	m_updateContext.SetDataTextureManager( m_dataTextureMgr );

	TriFrustum frustum;
	frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &Tr2Renderer::GetViewPosition(), &Tr2Renderer::GetProjectionTransform(), Tr2Renderer::GetViewport() );

	m_updateContext.SetFrustum( frustum );
	m_updateContext.SetHighDetailThreshold( g_eveSpaceSceneHighDetailThreshold / m_upscalingAmount );
	m_updateContext.SetMediumDetailThreshold( g_eveSpaceSceneMediumDetailThreshold / m_upscalingAmount );
	m_updateContext.SetLowDetailThreshold( g_eveSpaceSceneLowDetailThreshold / m_upscalingAmount );
	m_updateContext.SetVisibilityThreshold( g_eveSpaceSceneVisibilityThreshold / m_upscalingAmount );
	m_updateContext.SetLodFactor( g_eveSpaceSceneLODFactor / m_upscalingAmount );

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
		( *it )->Update( m_updateContext );
	}

	if( m_dataTextureMgr )
	{
		m_dataTextureMgr->Update( m_updateContext );
	}

	for( auto it = m_distanceFields.begin(); it != m_distanceFields.end(); ++it )
	{
		( *it )->Update( m_updateContext );
	}

	// Update lensflares
	for( EveLensflareVector::const_iterator it = m_lensflares.begin(); it != m_lensflares.end(); ++it )
	{
		( *it )->Update( realTime, simTime );
	}

	// Update planets
	UpdatePlanets( m_updateContext );

	if( m_virtualCameraSystem )
	{
		m_virtualCameraSystem->Update( realTime );
	}

	// Update all space objects
	for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( realTime, simTime );
	}

	{
		CCP_STATS_ZONE( "UpdateSyncronous" );

		for( IEveSpaceObject2Vector::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it )
		{
			( *it )->UpdateSyncronous( m_updateContext );
		}

		for( IEveSpaceObject2Vector::const_iterator it = m_uiObjects.begin(); it != m_uiObjects.end(); ++it )
		{
			( *it )->UpdateSyncronous( m_updateContext );
		}
	}
	{
		CCP_STATS_ZONE( "UpdateAsyncronous" );
		ScopedBlockTrap blockTrap;

		Tr2ParallelTaskGroup taskGroup = {};
		m_updateContext.SetTaskGroup( &taskGroup );
		for( auto& object : m_objects )
		{
			taskGroup.run( [object, this] { object->UpdateAsyncronous( m_updateContext ); } );
		}
		for( auto& object : m_uiObjects )
		{
			taskGroup.run( [object, this] { object->UpdateAsyncronous( m_updateContext ); } );
		}
		taskGroup.wait();
		m_updateContext.SetTaskGroup( nullptr );
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

void EveSpaceScene::SetupCascadedShadows( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_cascadedShadowMap || !m_componentRegistry )
	{
		return;
	}

	size_t shadowCasterCount = m_componentRegistry->ComponentCount<IEveShadowCaster>();
	size_t volumetricCount = m_componentRegistry->ComponentCount<ITr2VolumetricRenderable>();

	if( shadowCasterCount + volumetricCount == 0 )
	{
		return;
	}

	// set up frustums
	for( unsigned int splitIndex = 0; splitIndex < SHADOW_FRUSTUM_COUNT; ++splitIndex )
	{
		ShadowMap::SplitSetup splitSetupInfo = m_cascadedShadowMap->SetupShadowSplit( splitIndex, m_shadowView, m_sunData.DirWorld, m_updateContext.GetFrustum().m_zNear );

		// Get the split up camera frustum so we can use it to do some "half space culling" for objects
		TriFrustum frustum;
		const Matrix viewProj = Inverse( splitSetupInfo.invViewProj );
		frustum.ExtractFrustum( &viewProj );
		m_cameraFrustums[splitIndex] = frustum;

		m_shadowFrustums[splitIndex] = splitSetupInfo.shadowFrustum;

		// debug frustum
		if( m_debugRenderer && m_freezeFrustum )
		{
			Vector3 min = Vector3( -1, -1, 0 );
			Vector3 max = Vector3( 1, 1, 1 );
			Vector3 aabbMin = splitSetupInfo.aabb.m_min;
			Vector3 aabbMax = splitSetupInfo.aabb.m_max;

			m_debugRenderer->DrawBox( this, splitSetupInfo.invViewProj, min, max, Tr2DebugRenderer::Wireframe, m_cascadedShadowMap->GetDebugColors( splitIndex ) );
			m_debugRenderer->DrawBox( this, Inverse( splitSetupInfo.lightViewProjection ), aabbMin, aabbMax, Tr2DebugRenderer::Wireframe, m_cascadedShadowMap->GetDebugColors( splitIndex ) );
		}

		m_splitSetup[splitIndex] = splitSetupInfo;
	}

	// if the shadow map DS isn't set then skip everything
	if( !m_cascadedShadowMap->PrepareShadowRendering( renderContext ) )
	{
		return;
	}

	// Get shadow batches in parallel
	std::vector<size_t> indices;

	std::vector<std::vector<EveSpaceScene::ShadowInfo>> shadowCasterInfo;
	shadowCasterInfo.resize( SHADOW_FRUSTUM_COUNT );

	for( unsigned int i = 0; i < SHADOW_FRUSTUM_COUNT; ++i )
	{
		indices.push_back( i );
	}

	{
		CCP_STATS_ZONE( "GetBatches" );
		unsigned int shadowMapSize = m_cascadedShadowMap->GetShadowMapSize();
		auto shadowCasters = m_componentRegistry->GetComponents<IEveShadowCaster>();

		{
			CCP_STATS_ZONE( "Find shadow casters" );
			Tr2ParallelDo( begin( indices ), end( indices ), [&]( size_t frustumIndex ) {
				auto sunDir = m_sunData.DirWorld;
				auto cameraFrustum = m_cameraFrustums[frustumIndex];
				auto shadowFrustum = m_shadowFrustums[frustumIndex];
				auto casters = shadowCasterInfo[frustumIndex];

				auto frustumShadowCasterInfo = std::vector<EveSpaceScene::ShadowInfo>();
				frustumShadowCasterInfo.reserve( shadowCasterCount );

				for( auto& caster : shadowCasters )
				{
					float radius;
					if( caster->IsCastingShadow( cameraFrustum, shadowFrustum, shadowMapSize, sunDir, radius ) )
					{
						frustumShadowCasterInfo.push_back( EveSpaceScene::ShadowInfo( radius, caster, nullptr ) );
					}
				}
				shadowCasterInfo[frustumIndex] = frustumShadowCasterInfo;
			} );
		}
		{
			CCP_STATS_ZONE( "Per object data" );

			// This is not thread safe, hence no threading...
			for( unsigned int frustumIndex = 0; frustumIndex < SHADOW_FRUSTUM_COUNT; ++frustumIndex )
			{
				auto batches = m_shadowBatches[frustumIndex].get();
				for( auto& info : shadowCasterInfo[frustumIndex] )
				{
					info.perObjectData = info.caster->GetShadowPerObjectData( batches );
				}
			}
		}

		{
			CCP_STATS_ZONE( "get batches" );
			Tr2ParallelDo( begin( indices ), end( indices ), [&]( size_t frustumIndex ) {
				for( const auto& info : shadowCasterInfo[frustumIndex] )
				{
					info.caster->GetShadowBatches( m_shadowBatches[frustumIndex].get(), info.perObjectData, info.radius );
				}

				m_shadowBatches[frustumIndex]->Finalize();
			} );
		}
	}

	{
		GPU_REGION( renderContext, "Cascaded shadow maps" );

		{
			GPU_REGION( renderContext, "Cascade rendering" );

			for( unsigned int i = 0; i < SHADOW_FRUSTUM_COUNT; ++i )
			{
				if( shadowCasterInfo[i].empty() )
				{
					continue;
				}

				m_cascadedShadowMap->BeginShadowRendering( renderContext, i );

				// column_major for shaders
				PerFrameVSData data;
				data.ViewProjectionMat = Transpose( m_splitSetup[i].lightViewProjection );

				static const unsigned perFrameVsMask =
					( 1 << VERTEX_SHADER ) |
					SHADER_TYPE_EXISTS( COMPUTE_SHADER ) |
					SHADER_TYPE_EXISTS( GEOMETRY_SHADER ) |
					SHADER_TYPE_EXISTS( HULL_SHADER ) |
					SHADER_TYPE_EXISTS( DOMAIN_SHADER );
				FillAndSetConstants( m_shadowPerFrameVSBuffer, &data, sizeof( data ), perFrameVsMask, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );

				//***** Do the actual shadow rendering to the atlas (cascaded shadow depth map)
				{
					CCP_STATS_ZONE( "ShadowRendering" );

					renderContext.m_esm.SetInvertedDepthTest( false );
					ON_BLOCK_EXIT( [&] { renderContext.m_esm.SetInvertedDepthTest( true ); } );
					renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
					renderContext.RenderBatches( m_shadowBatches[i].get(), BlueSharedString( "Shadow" ) );
				}

				m_shadowBatches[i]->Clear();
				m_shadowAllocators[i].Clear();
			}
			m_cascadedShadowMap->EndShadowRendering( renderContext );
		}

		PopulatePerFramePSData( m_perFramePS, renderContext );
		ApplyPerFrameData( renderContext );
		SetupPlanetsAsShadowCaster( renderContext );
		m_cascadedShadowMap->DrawToShadowMapResult( renderContext, m_depthMap );

		if( m_componentRegistry && m_volumetricsRenderer && volumetricCount > 0 )
		{
			m_volumetricsRenderer->RenderShadows( *m_componentRegistry, m_cascadedShadowMap->GetShadowMap(), renderContext );
		}
	}
}

void EveSpaceScene::ApplyUpscalingToPerFrameData( uint32_t width, uint32_t height, Tr2RenderContext& renderContext )
{
	// Vs Data
	m_perFrameVS.TargetResolution.x = (float)width;
	m_perFrameVS.TargetResolution.y = (float)height;
	m_perFrameVS.ViewportSize.x = (float)width;
	m_perFrameVS.ViewportSize.y = (float)height;
	// Ps Data
	m_perFramePS.TargetResolution.x = (float)width;
	m_perFramePS.TargetResolution.y = (float)height;
	m_perFramePS.ViewportSize.x = (float)width;
	m_perFramePS.ViewportSize.y = (float)height;
	ApplyPerFrameData( renderContext );
}

void EveSpaceScene::DisableShadows()
{
	GlobalStore().RegisterVariable( "EveSpaceSceneShadowMap", m_emptyShadowMap );
}

void EveSpaceScene::ApplyPerFrameData( Tr2RenderContext& renderContext )
{
	static const unsigned perFrameVsMask =
		( 1 << VERTEX_SHADER ) |
		SHADER_TYPE_EXISTS( COMPUTE_SHADER ) |
		SHADER_TYPE_EXISTS( GEOMETRY_SHADER ) |
		SHADER_TYPE_EXISTS( HULL_SHADER ) |
		SHADER_TYPE_EXISTS( DOMAIN_SHADER );

	FillAndSetConstants( m_perFrameVSBuffer, &m_perFrameVS, sizeof( m_perFrameVS ), perFrameVsMask, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
	FillAndSetConstants( m_perFramePSBuffer, &m_perFramePS, sizeof( m_perFramePS ), PIXEL_SHADER, Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Sorts and gathers transparent batches
// Arguments:
//   objectsWithTransparencies - a list of renderables with transparencies
//   batches - the batch map to be used
// --------------------------------------------------------------------------------------
void EveSpaceScene::PrepareTransparentBatch( Tr2RenderableSortList& objectsWithTransparencies, BatchMap& batches, Tr2RenderReason reason )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Sort objects front to back
	std::stable_sort( objectsWithTransparencies.begin(), objectsWithTransparencies.end() );

	// Add the objects, back to front
	for( Tr2RenderableSortList::const_reverse_iterator it = objectsWithTransparencies.rbegin(); it != objectsWithTransparencies.rend(); ++it )
	{
		ITr2Renderable* r = it->m_object;
		Tr2PerObjectData* objectData = r->GetPerObjectData( batches[TRIBATCHTYPE_TRANSPARENT] );
		r->GetBatches( batches[TRIBATCHTYPE_TRANSPARENT], TRIBATCHTYPE_TRANSPARENT, objectData, reason );
	}
}

void GetBatchesFromRenderables( ITr2Renderable** const objectRenderables, const unsigned renderableCount, Tr2RenderableSortList* const objectsWithTransparencies, EveSpaceScene::BatchMap& batches, const TriBatchType* batchTypes, const unsigned batchTypeCount, Tr2RenderReason reason )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( unsigned i = 0; i != renderableCount; ++i )
	{
		ITr2Renderable* r = objectRenderables[i];

		Tr2PerObjectData* objectData = r->GetPerObjectData( batches[TRIBATCHTYPE_OPAQUE] );

		for( unsigned type = 0; type != batchTypeCount; ++type )
		{
			r->GetBatches( batches[batchTypes[type]], batchTypes[type], objectData, reason );
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

void GetBatchesFromRenderables(
	ITr2Renderable** const objectRenderables,
	const unsigned renderableCount,
	Tr2RenderableSortList* const objectsWithTransparencies,
	EveSpaceScene::BatchMap& batches,
	EveSpaceScene::PerThreadBatchMap& perThreadBatches,
	const TriBatchType* batchTypes, 
	const unsigned batchTypeCount, 
	Tr2RenderReason reason )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<Tr2PerObjectData*> perObjectData;
	perObjectData.reserve( renderableCount );

	{
		CCP_STATS_ZONE( "PerObjectData" );

		for( unsigned i = 0; i != renderableCount; ++i )
		{
			ITr2Renderable* r = objectRenderables[i];

			perObjectData.push_back( r->GetPerObjectData( batches[TRIBATCHTYPE_OPAQUE] ) );
		}
	}
	{
		CCP_STATS_ZONE( "GetBatches" );

		Tr2ParallelFor( unsigned( 0 ), renderableCount, [&]( unsigned i ) {
			ITr2Renderable* r = objectRenderables[i];
			for( unsigned type = 0; type != batchTypeCount; ++type )
			{
				r->GetBatches( &perThreadBatches[batchTypes[type]].local(), batchTypes[type], perObjectData[i], reason );
			}
		} );
	}
	{
		CCP_STATS_ZONE( "Combine" );

		for( unsigned type = 0; type != batchTypeCount; ++type )
		{
			auto& dest = batches[batchTypes[type]];
			for( auto& src : perThreadBatches[batchTypes[type]] )
			{
				dest->TransferFrom( &src );
			}
		}
	}
	if( objectsWithTransparencies )
	{
		CCP_STATS_ZONE( "Transparencies" );

		objectsWithTransparencies->reserve( renderableCount );

		for( unsigned i = 0; i != renderableCount; ++i )
		{
			ITr2Renderable* r = objectRenderables[i];
			if( r->HasTransparentBatches() )
			{
				ITr2RenderableEntry entry;
				entry.m_object = r;
				entry.m_distance = r->GetSortValue();
				objectsWithTransparencies->push_back( entry );
			}
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
void EveSpaceScene::GetAllBatchesFromRenderables( std::vector<ITr2Renderable*>& objectRenderables, Tr2RenderableSortList& objectsWithTransparencies, BatchMap& batches, Tr2RenderReason reason )
{
	if( objectRenderables.empty() || !Tr2Renderer::GetPoolAllocator() )
	{
		return;
	}

	static const TriBatchType s_allTypes[] =
		{
			TRIBATCHTYPE_OPAQUE,
			TRIBATCHTYPE_DECAL,
			TRIBATCHTYPE_DECAL_ADDITIVE,
			TRIBATCHTYPE_ADDITIVE,
			TRIBATCHTYPE_DEPTH,
			TRIBATCHTYPE_DISTORTION
		};

	unsigned typeCount = m_distortionMap ? 5 : 4;

	::GetBatchesFromRenderables( &objectRenderables[0], (unsigned int)objectRenderables.size(), &objectsWithTransparencies, batches, m_perThreadBatches, s_allTypes, typeCount, reason );
}

// --------------------------------------------------------------------------------------
// Description:
//   Gathers opaque and decal batches from renderables.
// Arguments:
//   objectRenderables - object renderables that we want to gather batches from
//   batches - the batch map to be used
// --------------------------------------------------------------------------------------
void EveSpaceScene::GetOpaqueBatchesFromRenderables( std::vector<ITr2Renderable*>& objectRenderables, BatchMap& batches, Tr2RenderReason reason )
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

	::GetBatchesFromRenderables( &objectRenderables[0], (unsigned int)objectRenderables.size(), nullptr, batches, m_perThreadBatches, s_allTypes, 2, reason );
}

// --------------------------------------------------------------------------------------
// Description:
//   Gathers depth batches from renderables.
// Arguments:
//   objectRenderables - object renderables that we want to gather batches from
//   batches - the batch map to be used
// --------------------------------------------------------------------------------------
void EveSpaceScene::GetDepthBatchesFromRenderables( std::vector<ITr2Renderable*>& objectRenderables, BatchMap& batches, Tr2RenderReason reason )
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

	::GetBatchesFromRenderables( &objectRenderables[0], (unsigned int)objectRenderables.size(), nullptr, batches, m_perThreadBatches, s_allTypes, 1, reason );
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
void EveSpaceScene::GetTransparentBatchesFromRenderables( std::vector<ITr2Renderable*>& objectRenderables, Tr2RenderableSortList& objectsWithTransparencies, BatchMap& batches, Tr2RenderReason reason )
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

	::GetBatchesFromRenderables( &objectRenderables[0], (unsigned int)objectRenderables.size(), &objectsWithTransparencies, batches, m_perThreadBatches, s_allTypes, typeCount, reason );
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
void EveSpaceScene::RenderRenderables( const std::vector<ITr2Renderable*>& renderables,
									   ITriRenderBatchAccumulator* batch,
									   TriBatchType batchType,
									   Tr2EffectStateManager::RenderingMode rm,
									   Tr2RenderContext& renderContext,
									   Tr2RenderReason reason )
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
		r->GetBatches( batch, batchType, objectData, reason );
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
void EveSpaceScene::RenderBatch( ITriRenderBatchAccumulator* batch,
								 Tr2EffectStateManager::RenderingMode rm,
								 Tr2RenderContext& renderContext )
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
void EveSpaceScene::RenderOpaqueBatches( BatchMap& batches, Tr2RenderContext& renderContext )
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
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
		renderContext.RenderBatchesWithOverride( batches[TRIBATCHTYPE_DECAL_ADDITIVE], visualizerEffect.effect );
		break;
	case VisualizerEffect::FULL_SCREEN_QUAD_OVERLAY:
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
		renderContext.RenderBatches( batches[TRIBATCHTYPE_OPAQUE] );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DECAL );
		renderContext.RenderBatches( batches[TRIBATCHTYPE_DECAL] );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
		renderContext.RenderBatches( batches[TRIBATCHTYPE_DECAL_ADDITIVE] );
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
void EveSpaceScene::RenderTransparentBatches( BatchMap& batches, Tr2RenderContext& renderContext )
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
void EveSpaceScene::RenderTransparentBatches2( BatchMap& batches, Tr2RenderContext& renderContext, bool pass)
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto& visualizerEffect = m_visualizerEffects[m_visualizeMethod];

	switch( visualizerEffect.type )
	{
	case VisualizerEffect::PIXEL_SHADER_REPLACEMENT:
		if( !pass )
		{
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
			renderContext.RenderBatchesWithOverride( batches[TRIBATCHTYPE_TRANSPARENT], visualizerEffect.effect );
		}
		else
		{
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
			renderContext.RenderBatchesWithOverride( batches[TRIBATCHTYPE_ADDITIVE], visualizerEffect.effect );
		}
		break;
	case VisualizerEffect::FULL_SCREEN_QUAD_OVERLAY:
		if( !pass )
		{
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
			renderContext.RenderBatches( batches[TRIBATCHTYPE_TRANSPARENT] );
		}
		else
		{
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
			renderContext.RenderBatches( batches[TRIBATCHTYPE_ADDITIVE] );
		}
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
bool EveSpaceScene::RenderDistortionBatches( BatchMap& batches, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_distortionMap || !batches[TRIBATCHTYPE_DISTORTION]->GetBatchCount() )
	{
		return false;
	}

	// Hold on to original depth stencil and back buffer
	Tr2PushPopRT pushPopRT( *m_distortionMap, renderContext );
	Tr2PushPopDS pushPopDS( renderContext );

	if( m_depthMap && m_depthMap->IsValid() )
	{
		renderContext.m_esm.SetDepthStencilBuffer( *m_depthMap );
	}


	renderContext.Clear( CLEARFLAGS_TARGET, 0x007f7f00, 1.f, 0 );

	// Do the actual rendering
	ApplyPerFrameData( renderContext );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
	renderContext.RenderBatches( batches[TRIBATCHTYPE_DISTORTION] );
	return true;
}

void EveSpaceScene::Jitter( Tr2RenderContext& renderContext )
{
	m_projection = Tr2Renderer::GetProjectionTransform();

	auto upscalingInfo = renderContext.GetPrimaryRenderContext().GetUpscalingInfo( Tr2Renderer::GetUpscalingContextID() );
	if( m_usingUpscaling && upscalingInfo.temporal )
	{	
		m_jitter.x = upscalingInfo.jitterX;
		m_jitter.y = upscalingInfo.jitterY;
		m_jitterMatrix = TranslationMatrix( Vector3( m_jitter.x, m_jitter.y, 0 ) );
		m_jitteredProjection = m_projection * m_jitterMatrix;
	}
	else if( m_postProcess )
	{
		auto rtWidth = renderContext.m_esm.GetRenderTargetWidth();
		auto rtHeight = renderContext.m_esm.GetRenderTargetHeight();

		m_postProcess->GetJitter( rtWidth, rtHeight, m_jitter.x, m_jitter.y );
		auto currJitterOffset = Vector3( m_jitter.x, m_jitter.y, 0 );

		m_jitterMatrix = TranslationMatrix( currJitterOffset );
		m_jitteredProjection = m_projection * m_jitterMatrix;
	}
	else
	{
		m_jitterMatrix = IdentityMatrix();
		m_jitteredProjection = m_projection;
	}
}

void EveSpaceScene::UpdatePostProcessPSData()
{
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
	auto upscalingInfo = renderContext.GetPrimaryRenderContext().GetUpscalingInfo( Tr2Renderer::GetUpscalingContextID() );
	m_usingUpscaling = upscalingInfo.technique != Tr2UpscalingAL::NONE;

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

	Jitter( renderContext );

	m_frameData.projection = m_jitteredProjection;
	Tr2Renderer::SetProjectionTransform( m_frameData.projection );
	m_reprojectionMatrix = Inverse( m_projection ) * Inverse( Tr2Renderer::GetViewTransform() ) * m_viewLast * m_projectionLast;
	
	m_velocityMapDirty = false;

	renderContext.m_esm.BeginManagedRendering();
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );

	if( g_eveSpaceSceneDynamicLighting )
	{
		if( auto lightManager = Tr2LightManager::GetOrCreateInstance( "res:/graphics/effect/managed/space/system/computelightlists.fx" ) )
		{
			lightManager->SetVariableStore();
		}
	}
	else
	{
		Tr2LightManager::DeleteInstance();
	}

	GatherBatches( renderContext );

	if( m_shadowQuality == SHADOW_RAYTRACED && m_enableShadows )
	{
		PrepareRaytracedShadows( renderContext );
	}

	UpdatePostProcessPSData();
	UpdateVariableStore();

	if( auto lightManager = Tr2LightManager::GetInstance() )
	{
		CCP_STATS_SCOPED_TIME( gatherDynamicLights );

		lightManager->Clear( renderContext );
		lightManager->SetFrustum( m_updateContext.GetFrustum() );
		lightManager->AdjustLightCutoff( m_updateContext.GetLodFactor() );

		Tr2ParallelFor( Tr2BlockedRange<size_t>( 0, m_objects.size(), 20 ), [&]( Tr2BlockedRange<size_t> range ) {
			for( auto i = range.begin(); i != range.end(); ++i )
			{
				ITr2LightOwnerPtr lightOwner( BlueCastPtr( m_objects[i] ) );
				if( lightOwner )
				{
					lightOwner->GetLights( *lightManager );
				}
			}
		} );

		for( auto it = begin( m_backgroundObjects ); it != end( m_backgroundObjects ); ++it )
		{
			ITr2LightOwnerPtr lightOwner( BlueCastPtr( *it ) );
			if( lightOwner )
			{
				lightOwner->GetLights( *lightManager );
			}
		}
		m_cameraAttachmentParent->GetLights( *lightManager );
	}

	//  the lensflares need a special pre-render update
	for( auto it = m_lensflares.cbegin(); it != m_lensflares.cend(); ++it )
	{
		( *it )->PrepareRender( m_updateContext.GetFrustum() );
	}

	if( m_velocityMap )
	{
#if !TRINITY_PLATFORM_SUPPORTS_RENDER_PASS_HINTS
		Tr2PushPopRT rt( *m_velocityMap, renderContext, 1 );
		renderContext.Clear( CLEARFLAGS_TARGET, 0x00000000, 1.f, 0, 1 );
#endif
	}

	PopulatePerFramePSData( m_perFramePS, renderContext );
	PopulatePerFrameVSData( m_perFrameVS, renderContext );
	ApplyPerFrameData( renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Gather all batches into the m_primaryBatches BatchMap.
// --------------------------------------------------------------------------------------
void EveSpaceScene::GatherBatches( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	
	std::vector<IEveSpaceObject2*> allObjects;
	std::vector<ITr2Renderable*> renderables;
	Tr2RenderableSortList transparentObjects;
	const Matrix& identity = IdentityMatrix();

	{
		CCP_STATS_ZONE( "UpdateVisibility" );
		Tr2ParallelDo( m_objects.begin(), m_objects.end(), [&]( IEveSpaceObject2* obj ) {
			obj->UpdateVisibility( m_updateContext, identity );
		} );

		m_cameraAttachmentParent->SetTransform( Tr2Renderer::GetInverseViewTransform() );
		m_cameraAttachmentParent->UpdateSyncronous( m_updateContext );
		m_cameraAttachmentParent->UpdateAsyncronous( m_updateContext );
		m_cameraAttachmentParent->UpdateVisibility( m_updateContext, identity );

		Tr2ParallelDo( m_staticParticles.begin(), m_staticParticles.end(), [&]( EveSceneStaticParticles* staticParticles ){
			staticParticles->UpdateVisibility( m_updateContext );
		} );
		
		Tr2ParallelDo( m_planets.begin(), m_planets.end(), [&]( EvePlanet* obj ) {
			obj->UpdateZOnlyVisibility( m_updateContext );
		} );

		// until we have proper support for multiple lensflares we just do it in a list...
		for( auto& lensflare : m_lensflares ){
			lensflare->UpdateVisibility( m_updateContext );
		}
	}

	// objects will be batched up.
	for( IEveSpaceObject2Vector::iterator it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		IEveSpaceObject2* obj = *it;

		allObjects.push_back( obj );
	}
	allObjects.push_back( m_cameraAttachmentParent );

	Tr2ImpostorManager* impostorManager = nullptr;
	if( m_impostorManager && ( !m_reflectionProbe || m_reflectionProbe->HasData() ) )
	{
		uint32_t size = 2;
		uint32_t threshold = uint32_t( g_eveSpaceSceneLowDetailThreshold ) / 2;
		while( size * 2 < threshold )
		{
			size *= 2;
		}
		m_impostorManager->SetItemSize( size, size );
		m_impostorManager->BeginUpdate();
		impostorManager = m_impostorManager;
	}

	for( std::vector<IEveSpaceObject2*>::iterator it = allObjects.begin(); it != allObjects.end(); ++it )
	{
		IEveSpaceObject2* obj = *it;
		obj->GetRenderables( renderables, m_impostorManager );
	}

	if( impostorManager )
	{
		impostorManager->EndUpdate();
		UpdateImpostors( renderContext );
	}

	for( auto it = m_staticParticles.begin(); it != m_staticParticles.end(); ++it )
	{
		( *it )->GetRenderables( m_updateContext.GetFrustum(), renderables );
	}

	UpdateQuadRenderer( m_updateContext.GetFrustum(), allObjects, renderContext );
	Tr2QuadRenderer::Instance()->GetBatches( TRIBATCHTYPE_OPAQUE, m_primaryBatches[TRIBATCHTYPE_OPAQUE] );
	Tr2QuadRenderer::Instance()->GetBatches( TRIBATCHTYPE_ADDITIVE, m_primaryBatches[TRIBATCHTYPE_ADDITIVE] );

	GetAllBatchesFromRenderables( renderables, transparentObjects, m_primaryBatches );
	PrepareTransparentBatch( transparentObjects, m_primaryBatches );

	FinalizeBatches( m_primaryBatches );

	UpdateShLighting( allObjects );
}

void EveSpaceScene::PrepareRaytracedShadows( Tr2RenderContext& renderContext )
{
	if( m_objects.empty() )
	{
		return;
	}

	if( !m_rtManager )
	{
		return;
	}

	CCP_STATS_SCOPED_TIME( raytracedShadowsTime );
	m_rtManager->GetGeometry().BeginSceneUpdate();

	m_componentRegistry->ProcessComponents<IEveShadowCaster>([this]( IEveShadowCaster* caster ) -> void {
		caster->PushRtGeometry( *m_rtManager );
	} );

	m_rtManager->GetGeometry().EndSceneUpdate( renderContext );
}

void EveSpaceScene::UpdateImpostors( Tr2RenderContext& renderContext )
{
	if( !m_impostorManager || m_impostorManager->GetRenderQueueLength() == 0 )
	{
		return;
	}

	CCP_STATS_ZONE( __FUNCTION__ );
	GPU_REGION( renderContext, "Impostors Update" );

	CTriViewport fakeViewport;
	fakeViewport.width = 128;
	fakeViewport.height = 128;

	Tr2Renderer::PushViewTransform();
	Tr2Renderer::PushProjection();
	renderContext.m_esm.PushViewport();

	ON_BLOCK_EXIT( [&] {
		renderContext.m_esm.PopViewport();
		Tr2Renderer::PopProjection();
		Tr2Renderer::PopViewTransform();
	} );

	m_impostorManager->BeginUpdateAtlas( renderContext );
	ON_BLOCK_EXIT( [&] { m_impostorManager->EndUpdateAtlas( renderContext ); } );

	Tr2DepthStencilPtr bkDepthMap = m_depthMap;
	m_depthMap = m_impostorManager->GetItemDepthStencil();
	ON_BLOCK_EXIT( [&] { m_depthMap = bkDepthMap; } );

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
	ON_BLOCK_EXIT( [&] {
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

	UpdateVariableStore();
	if( m_ssao )
	{
		m_ssaoMapHandle->SetValue( m_ssao->GetBlankOutput() );
	}

	Vector3 eye = Tr2Renderer::GetInverseViewTransform().GetTranslation();
	Vector3 up = TransformNormal( Vector3( 0, 1, 0 ), Tr2Renderer::GetInverseViewTransform() );

	// disable shadows because otherwise we get black ships all the time
	DisableShadows();

	for( size_t i = 0; i < m_impostorManager->GetRenderQueueLength(); ++i )
	{
		GPU_REGION( renderContext, "Impostor Update" );

		auto spaceObject = m_impostorManager->BeginImpostorUpdate( i, renderContext );
		Matrix transform;
		spaceObject->GetLocalToWorldTransform( transform );

		Vector4 sphere( 0.f, 0.f, 0.f, 1.f );
		spaceObject->GetImpostorBoundingSphere( sphere );

		Vector3 position( sphere.x, sphere.y, sphere.z );

		Vector3 viewDir = Normalize( eye - position );

		const float distance = sphere.w * 10;
		Vector3 eyePosition = position + viewDir * distance;
		Matrix view( XMMatrixLookToLH( eyePosition, viewDir, up ) );
		Matrix proj( XMMatrixPerspectiveRH( sphere.w * 2, sphere.w * 2, distance - sphere.w, distance + sphere.w ) );

		Tr2Renderer::SetViewTransform( view );
		Tr2Renderer::SetProjectionTransform( proj );

		TriFrustum frustum;
		frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &Tr2Renderer::GetViewPosition(), &Tr2Renderer::GetProjectionTransform(), fakeViewport );

		PopulatePerFramePSData( m_perFramePS, renderContext );
		PopulatePerFrameVSData( m_perFrameVS, renderContext );
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
//   allObjects - vector of objects in scene
// --------------------------------------------------------------------------------------
void EveSpaceScene::UpdateShLighting(
	const std::vector<IEveSpaceObject2*>& allObjects )
{
	CCP_STATS_SCOPED_TIME( shLightingUpdateTime );

	if( m_shLightingManager )
	{
		Tr2ParallelFor( Tr2BlockedRange<size_t>( 0, allObjects.size(), 20 ), [&]( Tr2BlockedRange<size_t> range ) {
			for( auto i = range.begin(); i != range.end(); ++i )
			{
				ITr2ShLightingReceiverPtr receiver = BlueCastPtr( allObjects[i] );
				if( receiver != nullptr )
				{
					receiver->UpdateShLighting( *m_shLightingManager, m_updateContext );
				}
			}
		} );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds quads from visible space objects to a quad renderer.
// Arguments:
//   allObjects - list of visible object
//   renderContext - current render context
// --------------------------------------------------------------------------------------
void EveSpaceScene::UpdateQuadRenderer(
	const TriFrustum& frustum,
	const std::vector<IEveSpaceObject2*>& allObjects,
	Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto& quadRenderer = *Tr2QuadRenderer::Instance();

	Tr2ParallelFor( Tr2BlockedRange<size_t>( 0, allObjects.size(), 20 ), [&]( Tr2BlockedRange<size_t> range ) {
		for( auto i = range.begin(); i != range.end(); ++i )
		{
			allObjects[i]->AddQuadsToQuadRenderer( frustum, quadRenderer );
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

	Tr2ParallelFor( Tr2BlockedRange<size_t>( 0, objects.size(), 20 ), [&]( Tr2BlockedRange<size_t> range ) {
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

void EveSpaceScene::RenderReflectionPass( Tr2RenderContext& renderContext )
{
	if( !HasReflectionProbe() || !m_display )
	{
		return;
	}

	CCP_STATS_ZONE( __FUNCTION__ );

	// set the current reflection
	GPU_REGION( renderContext, "Reflection" );

	// lower the reflection intensity for the objects rendered into the reflection
	// (so the reflections in the reflections don't get brighter and brighter)
	// we cap it at 0.8 for no good reason, just felt like a good number to cap the reflection intensity in the reflections
	auto tmp = m_reflectionIntensity;
	if( m_reflectionIntensity != 0.0f )
	{
		m_reflectionIntensity = min( 0.8f, 1.0f / ( m_reflectionIntensity * m_reflectionIntensity ) );
	}

	// disable ssao
	if( m_ssao )
	{
		m_ssaoMapHandle->SetValue( m_ssao->GetBlankOutput() );
	}

	// disable shadows
	GlobalStore().RegisterVariable( "EveSpaceSceneShadowMap", m_emptyShadowMap );

	// we change the frustum during reflection updates/renderings
	// we need the old one for resetting
	auto normalFrustum = m_updateContext.GetFrustum();

	m_reflectionProbe->InitRenderPass( renderContext );
	for( unsigned i = m_reflectionProbe->GetStartFace(); i < m_reflectionProbe->GetEndFace(); i++ )
	{
		std::string contextName = "Reflection Face " + std::to_string( i );

		GPU_REGION( renderContext, contextName.c_str() );
		m_reflectionProbe->StartRenderFace( i, renderContext );

		PopulatePerFramePSData( m_perFramePS, renderContext );
		PopulatePerFrameVSData( m_perFrameVS, renderContext );
		ApplyPerFrameData( renderContext );

		{
			Matrix orgViewMatrix = Tr2Renderer::GetViewTransform();
			Matrix planetViewMatrix = CreatePlanetViewMatrix( orgViewMatrix );
			Tr2Renderer::SetViewTransform( planetViewMatrix );

			TriFrustum frustum;
			Matrix planetProjection = EveCamera::ModifyClipPlanes( Tr2Renderer::GetProjectionTransform(), 0.01f, 1e5f );
			frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &Tr2Renderer::GetViewPosition(), &planetProjection, renderContext.m_esm.GetViewport() );
			m_updateContext.SetFrustum(frustum);

			for( auto& planet : m_planets )
			{
				planet->UpdatePlanetVisibility( m_updateContext, m_planetScale );
			}

			Tr2Renderer::SetViewTransform( orgViewMatrix );
		}

		TriFrustum currentFrustum = m_reflectionProbe->GetFrustum( i, renderContext );
		m_updateContext.SetFrustum( currentFrustum );
		// get the background reflection renderables from the component registry
		RenderBackgroundPassObjects( renderContext, BackgroundRenderingReason::BACKGROUND_RENDER_REFLECTION );

		if( !m_lensflares.empty() && g_eveReflectionMode == EntityComponents::REFLECTION_SETTING_ULTRA )
		{
			GPU_REGION( renderContext, "Lens Flares in reflections" );
			std::vector<ITr2Renderable*> visible;

			// lensflares
			for( auto it = m_lensflares.cbegin(); it != m_lensflares.cend(); ++it )
			{
				( *it )->GetRenderables( currentFrustum, visible );
			}

			if( !visible.empty() )
			{
				renderContext.SetReadOnlyDepth( true );
				RenderRenderables( visible,
								   m_secondaryBatches[TRIBATCHTYPE_ADDITIVE],
								   TRIBATCHTYPE_ADDITIVE,
								   Tr2EffectStateManager::RM_ALPHA_ADDITIVE,
								   renderContext,
								   Tr2RenderReason::TR2RENDERREASON_REFLECTION );
				renderContext.SetReadOnlyDepth( false );
			}
		}

		if( m_dynamicObjectReflectionEnabled && m_reflectionProbe->ReadyForDynamicObjectReflections() )
		{
			std::vector<ITr2Renderable*> visibleRenderables;
			visibleRenderables.reserve( m_componentRegistry->ComponentCount<ITr2Renderable>() );

			// Filter out the non-visible reflection renderables based on the current frustum
			m_componentRegistry->ProcessComponents<ITr2Renderable>(
				[&]( ITr2Renderable* renderable ) -> void {
					if( renderable->IsVisible( m_updateContext ) )
					{
						visibleRenderables.push_back( renderable );
					}
				}
			);

			if( !visibleRenderables.empty() )
			{
				// clear planets
				renderContext.Clear( CLEARFLAGS_ZBUFFER, 0, 0, 0 );
				ClearBatches( m_secondaryBatches );

				// We are rendering in a left hand coordinate system (for cubemaps) so don't override the cullmode!!!
				renderContext.m_esm.BeginManagedRendering( CullMode::CULLMODE_NONE );

				Tr2RenderableSortList transparentObjects;
				GetAllBatchesFromRenderables( visibleRenderables, transparentObjects, m_secondaryBatches, Tr2RenderReason::TR2RENDERREASON_REFLECTION );

				PrepareTransparentBatch( transparentObjects, m_secondaryBatches, Tr2RenderReason::TR2RENDERREASON_REFLECTION );
				FinalizeBatches( m_secondaryBatches );

				renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DEPTH_ONLY );
				renderContext.RenderBatches( m_secondaryBatches[TRIBATCHTYPE_DEPTH], BlueSharedString( "Depth" ) );

				RenderOpaqueBatches( m_secondaryBatches, renderContext );
				RenderTransparentBatches( m_secondaryBatches, renderContext );

				ClearBatches( m_secondaryBatches );
				renderContext.m_esm.EndManagedRendering();
			}
		}
	}

	m_reflectionProbe->EndRenderPass( renderContext );

	m_updateContext.SetFrustum( normalFrustum );

	// reset ssao
	if( m_ssao )
	{
		m_ssaoMapHandle->SetValue( m_ssao->GetOutput() );
	}

	// reset the reflection intensity
	m_reflectionIntensity = tmp;

	PopulatePerFramePSData( m_perFramePS, renderContext );
	PopulatePerFrameVSData( m_perFrameVS, renderContext );
	ApplyPerFrameData( renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Render the background scene with all objects, and generate reflection map
// Returns:
//   boolean indicating whether background distortion is required.
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderBackgroundPass( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_hasBackgroundDistortionBatches = false;

	// "background" rendering
	if( !m_backgroundRenderingEnabled )
	{
		return;
	}

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
	Matrix orgViewMatrix = Tr2Renderer::GetViewTransform();
	Matrix planetView = CreatePlanetViewMatrix( orgViewMatrix );
	Tr2Renderer::SetViewTransform( planetView );
	// Update planet LODs and render planets
	TriFrustum frustum;
	Matrix planetProjection = EveCamera::ModifyClipPlanes( Tr2Renderer::GetProjectionTransform(), 0.01f, 1e5f );
	frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &Tr2Renderer::GetViewPosition(), &planetProjection, renderContext.m_esm.GetViewport() );
	
	auto normalFrustum = m_updateContext.GetFrustum();

	m_updateContext.SetFrustum(frustum);

	for( auto it = m_planets.begin(); it != m_planets.end(); ++it )
	{
		EvePlanet* obj = *it;
		obj->SetRenderScale( m_planetScale );
		obj->UpdateLOD();
	}

	Tr2ParallelDo( m_planets.begin(), m_planets.end(), [&]( EvePlanet* obj ) {
		obj->UpdatePlanetVisibility( m_updateContext, m_planetScale );
	} );

	m_updateContext.SetFrustum( normalFrustum );

	Tr2Renderer::SetViewTransform( orgViewMatrix );

	{
		GPU_REGION( renderContext, "Background" );

		if( m_velocityMap )
		{
			if( !m_velocityMapDirty )
			{
				ClearRenderTargetIfNoBatches( m_velocityMap, 1, renderContext, m_primaryBatches[TRIBATCHTYPE_OPAQUE]->GetBatchCount() );
			}

			Tr2PushPopRT rt( *m_velocityMap, renderContext, 1 );
			renderContext.RenderPassHint( { Tr2LoadAction::LOAD, Tr2StoreAction::STORE }, { m_velocityMapDirty ? Tr2LoadAction::LOAD : Tr2LoadAction::CLEAR, Tr2StoreAction::STORE }, { Tr2LoadAction::LOAD, Tr2StoreAction::STORE } );
			RenderBackgroundPassObjects( renderContext, BACKGROUND_RENDER_COLOR );
			m_velocityMapDirty = true;
		}
		else
		{
			renderContext.RenderPassHint( { Tr2LoadAction::LOAD, Tr2StoreAction::STORE }, { Tr2LoadAction::LOAD, Tr2StoreAction::DONT_CARE } );
			RenderBackgroundPassObjects( renderContext, BACKGROUND_RENDER_COLOR );
		}
		if( !m_planets.empty() )
		{
			renderContext.Clear( CLEARFLAGS_ZBUFFER, 0, 0, 0 );
		}
	}

	renderContext.m_esm.EndManagedRendering();
}

// --------------------------------------------------------------------------------------
// Description:
//   Render all background objects, nebula, stars, planets etc.
// Returns:
//   boolean indicating whether background distortion is required.
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderBackgroundPassObjects( Tr2RenderContext& renderContext, BackgroundRenderingReason reason )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<ITr2Renderable*> visible;
	Tr2RenderableSortList transparentObjects;

	// nebula
	if( m_backgroundEffect )
	{
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
		Tr2Renderer::DrawCameraSpaceScreenQuad( renderContext, m_backgroundEffect->GetShaderStateInterface(), m_backgroundEffect );
	}

	// stars
	if( m_starfield )
	{
		m_starfield->GetBatches( m_secondaryBatches[TRIBATCHTYPE_ADDITIVE], 0 );
		RenderBatch( m_secondaryBatches[TRIBATCHTYPE_ADDITIVE],
					 Tr2EffectStateManager::RM_ALPHA_ADDITIVE,
					 renderContext );
	}

	// backgroundobjects
	if( !m_backgroundObjects.empty() )
	{
		for( auto it = m_backgroundObjects.begin(); it != m_backgroundObjects.end(); ++it )
		{
			auto obj = *it;
			obj->UpdateVisibility( m_updateContext, IdentityMatrix() );
			obj->GetRenderables( visible, nullptr );
		}
		if( !visible.empty() )
		{
			GetAllBatchesFromRenderables( visible, transparentObjects, m_secondaryBatches );
			PrepareTransparentBatch( transparentObjects, m_secondaryBatches );
			FinalizeBatches( m_secondaryBatches );

			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DEPTH_ONLY );
			renderContext.RenderBatches( m_secondaryBatches[TRIBATCHTYPE_DEPTH], BlueSharedString( "Depth" ) );

			RenderOpaqueBatches( m_secondaryBatches, renderContext );
			RenderTransparentBatches( m_secondaryBatches, renderContext );

			if( m_secondaryBatches[TRIBATCHTYPE_OPAQUE]->GetBatchCount() || m_secondaryBatches[TRIBATCHTYPE_DECAL]->GetBatchCount() || m_secondaryBatches[TRIBATCHTYPE_DECAL_ADDITIVE]->GetBatchCount() )
			{
				renderContext.Clear( CLEARFLAGS_ZBUFFER, 0, 0, 0 );
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
		if( reason == BACKGROUND_RENDER_COLOR )
		{
			renderContext.SetReadOnlyDepth( true );
			for( EveLensflareVector::const_iterator it = m_lensflares.begin(); it != m_lensflares.end(); ++it )
			{
				( *it )->RunBackgroundOcclusionQueries( renderContext, m_updateContext );
			}
			renderContext.SetReadOnlyDepth( false );
		}
	}

	if( m_warpTunnel )
	{
		renderContext.Clear( CLEARFLAGS_ZBUFFER, 0, 0, 0 );

		m_warpTunnel->UpdateVisibility( m_updateContext, IdentityMatrix() );
		m_warpTunnel->GetRenderables( visible, nullptr );

		GetTransparentBatchesFromRenderables( visible, transparentObjects, m_secondaryBatches );
		PrepareTransparentBatch( transparentObjects, m_secondaryBatches );
		FinalizeBatches( m_secondaryBatches );
		RenderTransparentBatches( m_secondaryBatches, renderContext );
		if( reason == BACKGROUND_RENDER_COLOR )
		{
			m_hasBackgroundDistortionBatches = RenderDistortionBatches( m_secondaryBatches, renderContext );
		}
		ClearBatches( m_secondaryBatches );
	}

	renderContext.m_esm.EndManagedRendering();
}

// --------------------------------------------------------------------------------------
// Description:
//   Render opaque objects to populate a readable depth stencil.
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderDepthPass( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
    
	if( !m_display )
	{
		return;
	}

	renderContext.m_esm.BeginManagedRendering();

	std::vector<ITr2Renderable*> visible;

	// Render to depth map
	{
		renderContext.AddGpuMarker( __FUNCTION__ );
		GPU_REGION( renderContext, "Depth Pass" );

		Tr2EffectStateManager::RenderingMode renderingMode;
		if( m_normalMap )
		{
			renderContext.m_esm.PushRenderTarget();
			renderContext.m_esm.SetRenderTarget( 0, *m_normalMap );
			renderContext.RenderPassHint( { Tr2LoadAction::CLEAR, Tr2StoreAction::STORE }, { Tr2LoadAction::CLEAR, Tr2StoreAction::STORE } );
			renderContext.Clear( CLEARFLAGS_TARGET | CLEARFLAGS_ZBUFFER, 0, 0, 0 );
			renderingMode = Tr2EffectStateManager::RM_OPAQUE;
		}
		else
		{
#if TRINITY_PLATFORM == TRINITY_METAL
			renderContext.m_esm.PushRenderTarget();
			Tr2TextureAL nullTex;
			renderContext.SetRenderTarget( nullTex, 0 );
#endif
			renderContext.RenderPassHint( {}, { Tr2LoadAction::CLEAR, Tr2StoreAction::STORE } );
			renderContext.Clear( CLEARFLAGS_ZBUFFER, 0, 0, 0 );
			renderingMode = Tr2EffectStateManager::RM_DEPTH_ONLY;
		}


		ApplyPerFrameData( renderContext );

		renderContext.m_esm.ApplyStandardStates( renderingMode );
		renderContext.RenderBatches( m_primaryBatches[TRIBATCHTYPE_OPAQUE], BlueSharedString( "Depth" ) );
		renderContext.m_esm.ApplyStandardStates( renderingMode );
		renderContext.RenderBatches( m_primaryBatches[TRIBATCHTYPE_DEPTH], BlueSharedString( "Depth" ) );

		// Planet z areas need special treatment
		for( auto it = m_planets.begin(); it != m_planets.end(); ++it )
		{
			EvePlanet* obj = *it;
			obj->GetZOnlyRenderables( visible );
		}

		if( !visible.empty() )
		{
			// secondary batches don't run through a ::GetAllBatchesX() function, so collect all batches individually
			GetOpaqueBatchesFromRenderables( visible, m_secondaryBatches );
			GetDepthBatchesFromRenderables( visible, m_secondaryBatches );

			FinalizeBatches( m_secondaryBatches );
			renderContext.m_esm.ApplyStandardStates( renderingMode );
			renderContext.RenderBatches( m_secondaryBatches[TRIBATCHTYPE_OPAQUE], BlueSharedString( "Depth" ) );
			renderContext.m_esm.ApplyStandardStates( renderingMode );
			renderContext.RenderBatches( m_secondaryBatches[TRIBATCHTYPE_DEPTH], BlueSharedString( "Depth" ) );
			ClearBatches( m_secondaryBatches );

			visible.clear();
		}

		if( m_normalMap )
		{
#if TRINITY_PLATFORM_SUPPORTS_RENDER_PASS_HINTS
			renderContext.EndRenderPassHint();
#endif
			renderContext.m_esm.PopRenderTarget();
		}
		else
		{
#if TRINITY_PLATFORM == TRINITY_METAL
			renderContext.m_esm.PopRenderTarget();
#endif
		}
	}

	renderContext.m_esm.EndManagedRendering();
	
	GlobalStore().RegisterVariable( "EveSpaceSceneShadowMap", m_emptyShadowMap );

	if( m_rtManager && m_shadowQuality == SHADOW_RAYTRACED && m_enableShadows && m_depthMap && m_depthMap->IsValid() && !m_objects.empty() )
	{
		size_t volumetricCount = m_componentRegistry->ComponentCount<ITr2VolumetricRenderable>();
		size_t shadowCasterCount = m_componentRegistry->ComponentCount<IEveShadowCaster>();
		if( volumetricCount + shadowCasterCount != 0 )
		{
			renderContext.SetReadOnlyDepth( true );

			constexpr size_t maxPlanets = 2;
			CcpMath::Sphere planets[maxPlanets];
			SetupPlanetsAsShadowCaster( planets, maxPlanets );
			m_rtManager->RenderShadows( m_depthMap, m_normalMap, m_sunData.DirWorld, planets, maxPlanets, renderContext );

			if( m_componentRegistry && m_volumetricsRenderer )
			{
				m_volumetricsRenderer->RenderShadows( *m_componentRegistry, m_rtManager->GetShadowMap(), renderContext );

				RenderVolumetricShadowMap( renderContext );

				PopulatePerFramePSData( m_perFramePS, renderContext );
				ApplyPerFrameData( renderContext );
			}

			renderContext.SetReadOnlyDepth( false );
		}
	}
	
	if( m_ssao )
	{
		renderContext.SetReadOnlyDepth( true );
		m_ssao->SetInputBuffers( m_depthMap, m_normalMap );
		m_ssao->Filter( renderContext );
		renderContext.SetReadOnlyDepth( false );
	}
}

void EveSpaceScene::RenderVolumetricShadowMap( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	
	auto shadowCasters = m_componentRegistry->GetComponents<IEveShadowCaster>();

	m_componentRegistry->ProcessComponents<ITr2VolumetricRenderable>( [this, &renderContext, &shadowCasters]( ITr2VolumetricRenderable* volumetric ) -> void {
		ITr2VolumetricRenderable::ShadowInfo shadowInfo;
		volumetric->GetVolumetricShadowInfo( shadowInfo, m_sunData.DirWorld );

		if( volumetric->PrepareCloudShadowMap( renderContext ) )
		{
			RenderIntoCloudShadowMap( renderContext, &shadowInfo, shadowCasters );
			volumetric->SetCloudShadowMapHandle();
		}
	} );
}

void EveSpaceScene::RenderIntoCloudShadowMap( Tr2RenderContext& renderContext, const ITr2VolumetricRenderable::ShadowInfo* cloudShadowInformation, std::vector<IEveShadowCaster*> shadowCasters )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Get distance from camera to zFar
	auto direction = Tr2Renderer::GetViewPosition() - cloudShadowInformation->aabbMax;
	float dist = Length( direction );

	auto projection = PerspectiveFovMatrix( Tr2Renderer::GetFieldOfView(), Tr2Renderer::GetAspectRatio(), m_updateContext.GetFrustum().m_zNear, dist );
	auto invViewProj = Inverse( projection ) * Tr2Renderer::GetInverseViewTransform();
	const Matrix viewProj = Inverse( invViewProj );

	TriFrustum frustum;
	frustum.ExtractFrustum( &viewProj );

	{
		CCP_STATS_ZONE( "get shadowbatches for volumetrics" );
		float sizeInShadow = 0.0f;
		for( auto& caster : shadowCasters )
		{
			caster->IsCastingShadow( frustum, cloudShadowInformation->shadowFrustum, cloudShadowInformation->shadowMapSize, m_sunData.DirWorld, sizeInShadow );
			// special threshold check
			if( sizeInShadow > 5.0f )
			{
				auto perObjData = caster->GetShadowPerObjectData( m_shadowBatches[0].get() );
				caster->GetShadowBatches( m_shadowBatches[0].get(), perObjData, sizeInShadow );
			}
		}
	}

	m_shadowBatches[0]->Finalize();
	PerFrameVSData data;
	data.ViewProjectionMat = Transpose( cloudShadowInformation->lightViewProj );

	static const unsigned perFrameVsMask =
		( 1 << VERTEX_SHADER ) |
		SHADER_TYPE_EXISTS( COMPUTE_SHADER ) |
		SHADER_TYPE_EXISTS( GEOMETRY_SHADER ) |
		SHADER_TYPE_EXISTS( HULL_SHADER ) |
		SHADER_TYPE_EXISTS( DOMAIN_SHADER );
	FillAndSetConstants( m_shadowPerFrameVSBuffer, &data, sizeof( data ), perFrameVsMask, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );

	if( m_shadowBatches[0]->GetBatchCount() )
	{
		renderContext.m_esm.SetInvertedDepthTest( false );
		ON_BLOCK_EXIT( [&] { renderContext.m_esm.SetInvertedDepthTest( true ); } );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
		renderContext.RenderBatches( m_shadowBatches[0].get(), BlueSharedString( "Shadow" ) );
	}

	m_shadowBatches[0]->Clear();

	renderContext.SetReadOnlyDepth( false );

	renderContext.m_esm.PopRenderTarget();
	renderContext.m_esm.PopDepthStencilBuffer();
	renderContext.m_esm.PopViewport();
}

void EveSpaceScene::RenderVolumetrics( Tr2RenderContext& renderContext )
{
	if( !m_componentRegistry || !m_volumetricsRenderer || !m_depthMap )
	{
		return;
	}
	m_volumetricsRenderer->RenderVolumetrics( *m_componentRegistry, m_updateContext.GetFrustum(), *m_depthMap, m_sunData.DirWorld, m_perFramePS.VolumetricSlices, renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Main rendering of foreground objects.
// --------------------------------------------------------------------------------------
void EveSpaceScene::RenderMainPass( Tr2RenderContext& renderContext, CullMode cullmode )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_hasForegroundDistortionBatches = false;

	renderContext.m_esm.BeginManagedRendering( cullmode );
	
	if( !m_display )
	{
		return;
	}
	
	if( !m_freezeFrustum && m_enableShadows && ( m_shadowQuality == SHADOW_LOW || m_shadowQuality == SHADOW_HIGH ) )
	{
		m_shadowView = Tr2Renderer::GetInverseViewTransform();
		SetupCascadedShadows( renderContext );
	}
	renderContext.AddGpuMarker( __FUNCTION__ );

	renderContext.SetReadOnlyDepth( true );

	if( auto lightManager = Tr2LightManager::GetInstance() )
	{
		GPU_REGION( renderContext, "Lighting" );
		CCP_STATS_SCOPED_TIME( updateDynamicLightLists );

		lightManager->UpdateLists(renderContext );
	}

	GPU_REGION( renderContext, "Color Pass" );

	{
		GPU_REGION( renderContext, "Opaque" );

		{
			if( m_velocityMap )
			{
				if( !m_velocityMapDirty )
				{
					ClearRenderTargetIfNoBatches( m_velocityMap, 1, renderContext, m_primaryBatches[TRIBATCHTYPE_OPAQUE]->GetBatchCount() );
				}

				Tr2PushPopRT rt( *m_velocityMap, renderContext, 1 );
				renderContext.RenderPassHint( { Tr2LoadAction::LOAD, Tr2StoreAction::STORE }, { m_velocityMapDirty ? Tr2LoadAction::LOAD : Tr2LoadAction::CLEAR, Tr2StoreAction::STORE }, { Tr2LoadAction::LOAD, Tr2StoreAction::STORE } );
				RenderOpaqueBatches( m_primaryBatches, renderContext );
				m_velocityMapDirty = true;
			}
			else
			{
				renderContext.RenderPassHint( { Tr2LoadAction::LOAD, Tr2StoreAction::STORE }, { Tr2LoadAction::LOAD, Tr2StoreAction::DONT_CARE } );
				RenderOpaqueBatches( m_primaryBatches, renderContext );
			}
		}
	}


	if( m_opaqueColorMap != nullptr )
	{
		renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
		renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );
		renderContext.m_esm.PushRenderTarget( *m_opaqueColorMap );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
		Tr2Renderer::DrawTexture( renderContext, *m_colorMap );
		renderContext.m_esm.PopRenderTarget();
		renderContext.m_esm.PopDepthStencilBuffer();
	}

	Tr2Renderer::SetProjectionTransform( m_frameData.projection );
	
	PopulateAndApplyPerFrameData( renderContext );
	RenderVolumetrics( renderContext );

	{
		GPU_REGION( renderContext, "Transparent" );

		RenderTransparentBatches( m_primaryBatches, renderContext );
		m_hasForegroundDistortionBatches = RenderDistortionBatches( m_primaryBatches, renderContext );
	}

	// at first, do all the occlusion queries
	for( auto& lensflare : m_lensflares )
	{
		lensflare->RunOcclusionQueries( renderContext, m_updateContext );
	}

	//GPU particles
	if( GetGpuParticleSystem() )
	{
		GPU_REGION( renderContext, "Particles" );
		GetGpuParticleSystem()->Update( m_updateTime, m_updateContext.GetOriginShift(), renderContext );
		GetGpuParticleSystem()->Render( renderContext );
	}

	Tr2OcclusionBuffer::GetInstance().ProcessBuffer( renderContext );

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

	PopulatePerFramePSData( m_perFramePS, renderContext );
	PopulatePerFrameVSData( m_perFrameVS, renderContext );
	ApplyPerFrameData( renderContext );

	// collect visible lensflares
	std::vector<ITr2Renderable*> visible;

	PopulatePerFramePSData( m_perFramePS, renderContext );
	PopulatePerFrameVSData( m_perFrameVS, renderContext );
	ApplyPerFrameData( renderContext );

	if( !visible.empty() )
	{
		renderContext.SetReadOnlyDepth( true );
		RenderRenderables( visible,
						   m_secondaryBatches[TRIBATCHTYPE_TRANSPARENT],
						   TRIBATCHTYPE_TRANSPARENT,
						   Tr2EffectStateManager::RM_ALPHA,
						   renderContext );
		RenderRenderables( visible,
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
		GPU_REGION( renderContext, "Lens Flares" );
		// lensflares
		for( auto it = m_lensflares.cbegin(); it != m_lensflares.cend(); ++it )
		{
			( *it )->GetRenderables( m_updateContext.GetFrustum(), visible );
		}

		if( !visible.empty() )
		{
			renderContext.SetReadOnlyDepth( true );
			RenderRenderables( visible,
							   m_secondaryBatches[TRIBATCHTYPE_ADDITIVE],
							   TRIBATCHTYPE_ADDITIVE,
							   Tr2EffectStateManager::RM_ALPHA_ADDITIVE,
							   renderContext );
			renderContext.SetReadOnlyDepth( false );
		}
	}

	renderContext.m_esm.EndManagedRendering();

	// Clear primary batches
	ClearBatches( m_primaryBatches );

	if( m_displayShadowMap && m_cascadedShadowMap )
	{
		Tr2RenderTargetPtr shadowMapRT = m_cascadedShadowMap->GetCascadedShadowMapRT();
		if( shadowMapRT && shadowMapRT->IsValid() )
		{
			Tr2TextureAL* texture = shadowMapRT->GetTexture();
			Tr2Renderer::DrawTexture( renderContext, *texture );
		}
	}
	
	// Store the view transform from this frame
	m_viewLast = Tr2Renderer::GetViewTransform();
	m_projectionLast = m_projection;

	ClearVariableStore();

	if( m_visualizerEffects[m_visualizeMethod].type == VisualizerEffect::FULL_SCREEN_QUAD_OVERLAY )
	{
		Tr2Renderer::DrawTexture( renderContext, m_visualizerEffects[m_visualizeMethod].effect, Vector2( 0, 0 ), Vector2( 1, 1 ) );
	}

	if( m_cascadedShadowMap )
	{
		if( m_cascadedShadowMap->GetDebugSplitValue() )
		{
			Tr2Renderer::DrawTexture( renderContext, m_cascadedShadowMap->GetShadowEffect(), Vector2( 0, 0 ), Vector2( 1, 1 ) );
		}
	}
}

// --------------------------------------------------------------------------------------
void EveSpaceScene::Render3DUI( Tr2RenderContext& renderContext )
{
	renderContext.AddGpuMarker( __FUNCTION__ );

	Matrix identity = IdentityMatrix();
	std::vector<ITr2Renderable*> renderables;
	Tr2RenderableSortList transparentObjects;

	TriFrustum frustum;
	frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &Tr2Renderer::GetViewPosition(), &Tr2Renderer::GetProjectionTransform(), renderContext.m_esm.GetViewport() );

	renderContext.m_esm.BeginManagedRendering();
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );

	PopulatePerFramePSData( m_perFramePS, renderContext );
	PopulatePerFrameVSData( m_perFrameVS, renderContext );

	Tr2ParallelDo( m_uiObjects.begin(), m_uiObjects.end(), [&]( IEveSpaceObject2* obj ) { obj->UpdateVisibility( m_updateContext, identity ); } );

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

	ApplyPerFrameData( renderContext );

	// --------------------------------------------------------------------------------------
	RenderOpaqueBatches( m_secondaryBatches, renderContext );

	renderContext.SetReadOnlyDepth( true );
	RenderTransparentBatches( m_secondaryBatches, renderContext );
	renderContext.SetReadOnlyDepth( false );

	Tr2QuadRenderer::Instance()->DoneRendering( renderContext );

	RenderDebugInfo( renderContext );

	renderContext.m_esm.EndManagedRendering();
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );

	ClearBatches( m_secondaryBatches );
}

void EveSpaceScene::PopulateAndApplyPerFrameData( Tr2RenderContext& renderContext )
{
	PopulatePerFramePSData( m_perFramePS, renderContext );
	PopulatePerFrameVSData( m_perFrameVS, renderContext );
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
	case RP_REFLECTION_RENDER:
		RenderReflectionPass( renderContext );
		break;
	case RP_BACKGROUND_RENDER:
		RenderBackgroundPass( renderContext );
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
	m_nebulaIntensityVar = m_nebulaIntensity;
	// the environment cubemap (aka nebula) is passed theough the global variable store
	m_envMapHandle->SetValue( m_envMapTextureRes );

	if( m_ssao )
	{
		m_ssaoMapHandle->SetValue( m_ssao->GetOutput() );
	}
	if( m_volumetricsRenderer )
	{
		m_volumetricsRenderer->UpdateVariableStore();
	}
}

void EveSpaceScene::ClearVariableStore()
{
	m_envMap1Var.Clear();
	m_envMap2Var.Clear();
	m_reflectionMapVar.Clear();
	m_reflectionMaskMapVar.Clear();

	// shadowmap: clear store here
	if( m_cascadedShadowMap )
	{
		m_cascadedShadowMap->ClearVariableStore();
	}
}


void EveSpaceScene::PopulatePerFrameVSData( PerFrameVSData& data, Tr2RenderContext& renderContext )
{
	// column_major for shaders
	data.ViewMat = Transpose( Tr2Renderer::GetViewTransform() );
	Matrix proj = Tr2Renderer::GetReversedDepthProjectionTransform();
	data.ProjectionMat = Transpose( proj );
	Matrix viewProject( Tr2Renderer::GetViewTransform() * proj );
	data.ViewProjectionMat = Transpose( viewProject );
	// attention: need the transposed, but shader also needs column_major, so it is transpose(transpose(m)) == m
	data.ViewInverseTransposeMat = Tr2Renderer::GetInverseViewTransform();

	Matrix lastProjection = m_projectionLast * m_jitterMatrix;
	data.ViewProjectionLast = Transpose( m_viewLast * lastProjection );
	data.ViewLast = Transpose( m_viewLast );
	data.ProjLast = Transpose( lastProjection );

	// each scene has a nebula and that can be rotated and inverted (via scaling)
	data.EnvMapRotationMat = Transpose( RotationMatrix( m_envMapRotation ) );

	// sun data
	data.Sun = m_sunData;
	data.Sun.DiffuseColor = m_useSunColorWithDynamicLights && g_eveSpaceSceneDynamicLighting ? m_sunColorWithDynamicLights : m_sunColor;

	// make sure whatever direction we get in here, it is normalized! And inverted: Shaders work with direction to light...
	data.Sun.DirWorld = -Normalize( data.Sun.DirWorld );

	// resolution of rendertarget
	data.TargetResolution.x = (float)renderContext.m_esm.GetRenderTargetWidth();
	data.TargetResolution.y = (float)renderContext.m_esm.GetRenderTargetHeight();

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
	const Tr2Viewport& deviceViewport = renderContext.m_esm.GetDeviceViewport();
	const TriViewport& viewport = renderContext.m_esm.GetViewport();
	data.ViewportAdjustment.x = viewport.x < 0 ? -1.0f : 1.0f;
	data.ViewportAdjustment.y = viewport.y + viewport.height > (int)renderContext.m_esm.GetRenderTargetHeight() ? -1.0f : 1.0f;
	data.ViewportAdjustment.z = deviceViewport.m_width / viewport.width;
	data.ViewportAdjustment.w = deviceViewport.m_height / viewport.height;
	data.Time = Tr2Renderer::GetAnimationTime();
	data.Upscaling = m_upscalingAmount;

	data.ViewportSize.x = renderContext.m_esm.GetDeviceViewport().m_width;
	data.ViewportSize.y = renderContext.m_esm.GetDeviceViewport().m_height;
}

void EveSpaceScene::PopulatePerFramePSData( PerFramePSData& data, Tr2RenderContext& renderContext )
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

	data.ReflectionIntensity = m_reflectionIntensity * m_nebulaIntensity;
	data.FogColor = Vector4( m_fogColor.r, m_fogColor.g, m_fogColor.b, m_fogMax );

	// ps gamma brightness
	data.GammaBrightness = g_eveSpaceSceneGammaBrightness;

	// resolution of rendertarget
	data.TargetResolution.x = (float)renderContext.m_esm.GetRenderTargetWidth();
	data.TargetResolution.y = (float)renderContext.m_esm.GetRenderTargetHeight();

	// fov in both ways: width (x) and (height (y)
	data.FovXY.y = EveCamera::CalculateFovFromProjection( Tr2Renderer::GetProjectionTransform() );
	data.FovXY.x = data.FovXY.y * Tr2Renderer::GetAspectRatio();

	// disable shadows per default
	data.ShadowCameraRange = Vector2( 1.f, 0.f );

	data.ViewportOffset.x = (float)renderContext.m_esm.GetViewport().x;
	data.ViewportOffset.y = (float)renderContext.m_esm.GetViewport().y;

	data.ViewportSize.x = renderContext.m_esm.GetDeviceViewport().m_width;
	data.ViewportSize.y = renderContext.m_esm.GetDeviceViewport().m_height;

	data.Time = Tr2Renderer::GetAnimationTime();
	data.Upscaling = 1.0f;
	
	data.FrameIndex = (uint32_t) Tr2Renderer::GetCurrentFrameCounter();
	data.Jittering = m_jitter != Vector4(0, 0, 0, 0);

	data.ShadowMapSettings = Vector4( 1.f, 1.f, 0.f, 0.f );
	
	data.ShadowLightness = 0;
	data.DepthMapSampleCount = 1.0f; //legacy

	Matrix projection = Tr2Renderer::GetReversedDepthProjectionTransform();
	data.ProjectionToView.x = projection._43;
	data.ProjectionToView.y = projection._33;

	data.SceneMipLodBias = 0.0f;
	m_upscalingAmount = 1.0f;

	if( m_usingUpscaling )
	{
		auto upscalingInfo = renderContext.GetPrimaryRenderContext().GetUpscalingInfo( Tr2Renderer::GetUpscalingContextID() );

		m_upscalingAmount = upscalingInfo.upscalingAmount;
		data.SceneMipLodBias = upscalingInfo.mipLevelBias;
		if( !upscalingInfo.temporal && m_postProcess )
		{
			data.SceneMipLodBias += m_postProcess->GetMipLodBias();
		}
		data.Upscaling = m_upscalingAmount; 
	}
	else if( m_postProcess )
	{
		data.SceneMipLodBias = m_postProcess->GetMipLodBias();
	}

	if( m_cascadedShadowMap )
	{
		data.ShadowMapValues[0] = m_cascadedShadowMap->m_perSplitData.ShadowMapValues[0];
		data.ShadowMapValues[1] = m_cascadedShadowMap->m_perSplitData.ShadowMapValues[1];
		data.ShadowMapValues[2] = m_cascadedShadowMap->m_perSplitData.ShadowMapValues[2];
		data.ShadowMapValues[3] = m_cascadedShadowMap->m_perSplitData.ShadowMapValues[3];

		for( int i = 0; i < SHADOW_FRUSTUM_COUNT; ++i )
		{
			data.ShadowMatrixVal[i] = m_cascadedShadowMap->m_perSplitData.ShadowMatrixVal[i];
		}
		data.SplitInfo = m_cascadedShadowMap->m_perSplitData.SplitInfo;
	}

	// m_perFrameVS.ProjectionMat is already transposed
	data.ProjectionInverseMat = Inverse( m_perFrameVS.ProjectionMat );
	data.Debug = m_perFrameDebug;

	data.VolumetricSlices[0] = 1000;
	data.VolumetricSlices[1] = 10000;
	data.VolumetricSlices[2] = 100000;
	data.VolumetricSlices[3] = 1000000;
}

bool EveSpaceScene::Initialize()
{
	// the environment cubemap aka the nebula
	if( m_reflectionProbe && m_reflectionProbe->IsValid() )
	{
		m_envMapTextureRes = m_reflectionProbe->GetReflection();

		m_reflectionProbe->SetBackLightColor( m_reflectionBackLightingColor );
		m_reflectionProbe->SetBackLightContrast( m_reflectionBackLightingContrast );
	}
	else if( m_envMapHandle )
	{
		BeResMan->GetResource( m_envMapResPath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_envMapTextureRes );
	}

	if( !m_envMap1ResPath.empty() )
	{
		BeResMan->GetResource( m_envMap1ResPath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_envMap1 );
	}
	if( !m_envMap2ResPath.empty() )
	{
		BeResMan->GetResource( m_envMap2ResPath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_envMap2 );
	}
	if( !m_envMap3ResPath.empty() )
	{
		BeResMan->GetResource( m_envMap3ResPath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_envMap3 );
	}

	if( m_shLightingManager )
	{
		Tr2ShLightingManagerPtr manager = m_shLightingManager;
		m_shLightingManager = nullptr;
		SetShLightingManager( manager );
	}

	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		if( EveEntityPtr entity = BlueCastPtr( *it ) )
		{
			entity->Register( m_componentRegistry );
		}
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
	if( IsMatch( value, m_reflectionProbe ) || IsMatch( value, m_envMapResPath ) )
	{
		m_envMapTextureRes.Unlock();
		if( HasReflectionProbe() )
		{
			m_envMapTextureRes = m_reflectionProbe->GetReflection();
			m_reflectionProbe->SetBackLightColor( m_reflectionBackLightingColor );
			m_reflectionProbe->SetBackLightContrast( m_reflectionBackLightingContrast );
		}
		else if( !m_envMapResPath.empty() )
		{
			BeResMan->GetResource( m_envMapResPath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_envMapTextureRes );
		}
	}
	if( IsMatch( value, m_envMap1ResPath ) )
	{
		m_envMap1.Unlock();
		if( !m_envMap1ResPath.empty() )
		{
			BeResMan->GetResource( m_envMap1ResPath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_envMap1 );
		}
	}
	if( IsMatch( value, m_envMap2ResPath ) )
	{
		m_envMap2.Unlock();
		if( !m_envMap2ResPath.empty() )
		{
			BeResMan->GetResource( m_envMap2ResPath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_envMap2 );
		}
	}
	if( IsMatch( value, m_envMap3ResPath ) )
	{
		m_envMap3.Unlock();
		if( !m_envMap3ResPath.empty() )
		{
			BeResMan->GetResource( m_envMap3ResPath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_envMap3 );
		}
	}

	if( IsMatch( value, m_reflectionBackLightingColor ) && HasReflectionProbe() )
	{
		m_reflectionProbe->SetBackLightColor( m_reflectionBackLightingColor );
	}
	if( IsMatch( value, m_reflectionBackLightingContrast ) && HasReflectionProbe() )
	{
		m_reflectionProbe->SetBackLightContrast( m_reflectionBackLightingContrast );
	}

	if( IsMatch( value, m_shadowQuality ) )
	{
		g_eveSpaceSceneRaytracedShadows = m_shadowQuality == SHADOW_RAYTRACED;

		if( m_shadowQuality == SHADOW_LOW )
		{
			if( m_cascadedShadowMap )
			{
				m_cascadedShadowMap->ShouldUseDenoiser( false );
			}
		}
		if( m_shadowQuality == SHADOW_HIGH )
		{
			if( m_cascadedShadowMap )
			{
				m_cascadedShadowMap->ShouldUseDenoiser( true );
			}
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
	long event, // BLUELISTEVENT values
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
		if( theList == &m_objects )
		{
			for( ssize_t i = 0; i < theList->GetSize(); ++i )
			{
				if( EveEntityPtr entity = BlueCastPtr( theList->GetAt( i ) ) )
				{
					entity->UnRegister( m_componentRegistry );
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
		if( theList == &m_objects )
		{
			if( EveEntityPtr entity = BlueCastPtr( value ) )
			{
				entity->Register( m_componentRegistry );
			}
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
		if( theList == &m_objects )
		{
			if( EveEntityPtr entity = BlueCastPtr( value ) )
			{
				entity->UnRegister( m_componentRegistry );
			}
		}
	}
	break;
	}
}

IRoot* EveSpaceScene::PickObject( int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, Be::OptionalWithDefaultValue<Tr2PickTypes, PICK_TYPE_PICKING | PICK_TYPE_OPAQUE> filter )
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
	renderContext.m_esm.PushViewport();
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.PopViewport(); } );

	float fx, fy;
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
		if( gizmo )
		{
			result = gizmo.m_object;
			areaID = gizmo.m_area;
		}
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


	if( !collisionSet.empty() )
	{
		renderContext.m_esm.SetInvertedDepthTest( true );
		ON_BLOCK_EXIT( [&] { renderContext.m_esm.SetInvertedDepthTest( false ); } );

		renderContext.m_esm.BeginManagedRendering();
		ON_BLOCK_EXIT( [&] { renderContext.m_esm.EndManagedRendering(); } );

		CR_RETURN_VAL( Tr2Renderer::BeginRenderContext(), nullptr );
		ON_BLOCK_EXIT( [&] { Tr2Renderer::EndRenderContext(); } );

		float initialDepth = ( dist - Tr2Renderer::GetFrontClip() ) / ( Tr2Renderer::GetBackClip() - Tr2Renderer::GetFrontClip() );
		initialDepth = std::max( 0.0f, std::min( 1.0f, initialDepth ) );

		unsigned short objId = 0xffff;
		unsigned short aId = 0xffff;

		if( m_pickBuffer.BeginRendering( std::max( gizmoDepth, 1 - initialDepth ), renderContext ) )
		{
			for( unsigned int i = 0; i < collisionSet.size(); i++ )
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
					pickable->GetPickingBatches( m_pickingBatches, pickTypes & ~PICK_TYPE_PICKING, perObjectData );
				}
				// Additionally, we can pick against geometry that's only rendered for picking,
				// allowing us to put placeholders in for things that are partly transparent, but still should be pickable
				if( ( pickTypes & PICK_TYPE_PICKING ) != 0 )
				{
					pickable->GetPickingBatches( m_pickingBatches, PICK_TYPE_PICKING, perObjectData );
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
	}

	return result;
}


void EveSpaceScene::SetupTransformsForPicking( float fx, float fy, TriProjection* proj, TriView* view, TriViewport* viewport, Tr2RenderContext& renderContext )
{
	Tr2Renderer::SetViewTransform( view->GetTransform() );
	proj->SetProjection();
	// scale up the projection so we have 1 pixel rendered to the viewport
	Vector2 scaling( float( viewport->width ), float( viewport->height ) );
	// translate the projection so that we center around the pick ray origin,
	// while remembering to scale this value as well:
	Vector2 translation;
	translation.x = -fx * scaling.x;
	translation.y = -fy * scaling.y;
	Tr2Renderer::AdjustProjection( scaling, translation );

	EveSpaceScene::PerFramePSData perFramePS;
	EveSpaceScene::PerFrameVSData perFrameVS;

	EveSpaceScene::PopulatePerFramePSData( perFramePS, renderContext );
	EveSpaceScene::PopulatePerFrameVSData( perFrameVS, renderContext );

	// set the rendertarget resolution, which always 1x1 for icking
	perFrameVS.TargetResolution.x = perFrameVS.TargetResolution.y = 1.f;

	static const unsigned perFrameVsMask =
		( 1 << VERTEX_SHADER ) |
		SHADER_TYPE_EXISTS( COMPUTE_SHADER ) |
		SHADER_TYPE_EXISTS( GEOMETRY_SHADER ) |
		SHADER_TYPE_EXISTS( HULL_SHADER ) |
		SHADER_TYPE_EXISTS( DOMAIN_SHADER );
	FillAndSetConstants( m_perFrameVSBuffer, &perFrameVS, sizeof( perFrameVS ), perFrameVsMask, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
	FillAndSetConstants( m_perFramePSBuffer, &perFramePS, sizeof( perFramePS ), PIXEL_SHADER, Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
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
	Matrix proj = Tr2Renderer::GetProjectionTransform();
	const Matrix& view = Tr2Renderer::GetViewTransform();

	CTriViewport vp;
	vp.width = vp.height = 1;

	pickFrustum.DeriveFrustum( &view, &camPos, &proj, vp );

	auto oldFrustum = m_updateContext.GetFrustum();
	m_updateContext.SetFrustum(pickFrustum);

	for( IEveSpaceObject2Vector::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		if( ( *it )->IsPickable() )
		{
			// Note: Here we are relying on the EveSpaceObject's GetRenderables
			// function to decide what objects are pickable in the given frustum.
			// This is not necessarily what we want, as some objects might be
			// renderable but not pickable (particle clouds?)
			( *it )->UpdateVisibility( m_updateContext, IdentityMatrix() );
			( *it )->GetRenderables( pickableRenderObjects, nullptr );
		}
	}
	m_updateContext.SetFrustum( oldFrustum );

	m_cameraAttachmentParent->UpdateVisibility( m_updateContext, IdentityMatrix() );
	m_cameraAttachmentParent->GetRenderables( pickableRenderObjects, nullptr );
}

void EveSpaceScene::UpdatePlanets( const EveUpdateContext& updateContext )
{
	// The planet view matrix must be set up for update to accommodate
	// potential transform modifiers in the transform hierarchy of the
	// planets.
	Matrix orgViewMatrix = Tr2Renderer::GetViewTransform();
	Tr2Renderer::SetViewTransform( CreatePlanetViewMatrix( orgViewMatrix ) );

	for( EvePlanetVector::iterator it = m_planets.begin(); it != m_planets.end(); ++it )
	{
		( *it )->UpdatePlanetSyncronous( updateContext, m_planetScale );
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

	Matrix lastPlanetViewMatrix = CreatePlanetViewMatrix( m_viewLast );
	Tr2Renderer::SetViewTransform( CreatePlanetViewMatrix( Tr2Renderer::GetViewTransform() ));
	
	Matrix planetProjection = EveCamera::ModifyClipPlanes( m_projection, 0.01f, 1e5f ) * m_jitterMatrix;
	Matrix lastPlanetProjection = EveCamera::ModifyClipPlanes( m_projectionLast, 0.01f, 1e5f ) * m_jitterMatrix; //apply the same transformations and jitter to both so that they all cancel out.
	
	Tr2Renderer::SetProjectionTransform( planetProjection );

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
	PopulatePerFramePSData( m_perFramePS, renderContext );
	PopulatePerFrameVSData( m_perFrameVS, renderContext );
	m_perFrameVS.ViewProjectionLast = Transpose( lastPlanetViewMatrix * lastPlanetProjection );
	ApplyPerFrameData( renderContext );

	Tr2RenderableSortList renderablesWithTransparencies;
	GetAllBatchesFromRenderables( planetRenderables, renderablesWithTransparencies, m_secondaryBatches );
	PrepareTransparentBatch( renderablesWithTransparencies, m_secondaryBatches );
	FinalizeBatches( m_secondaryBatches );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_DEPTH_ONLY );
	renderContext.RenderBatches( m_secondaryBatches[TRIBATCHTYPE_DEPTH], BlueSharedString( "Depth" ) );
	RenderOpaqueBatches( m_secondaryBatches, renderContext );

	RenderTransparentBatches( m_secondaryBatches, renderContext );
	ClearBatches( m_secondaryBatches );

	// Put view/projection back to normal
	Tr2Renderer::PopProjection();
	guardPopProjection.Dismiss();

	Tr2Renderer::PopViewTransform();
	guardPopViewTransform.Dismiss();

	PopulatePerFramePSData( m_perFramePS, renderContext );
	PopulatePerFrameVSData( m_perFrameVS, renderContext );
	ApplyPerFrameData( renderContext );
}

Matrix EveSpaceScene::CreatePlanetViewMatrix( const Matrix& original )
{
	Matrix planetViewMatrix = original;
	const float planetScale = 1.0f / m_planetCameraScale;
	Vector3& viewPos = planetViewMatrix.GetTranslation();
	viewPos *= planetScale;

	return planetViewMatrix;
}

void EveSpaceScene::ClearRenderTargetIfNoBatches( Tr2RenderTarget* rt, uint32_t slot, Tr2RenderContext& renderContext, size_t batchCount )
{
#if TRINITY_PLATFORM_SUPPORTS_RENDER_PASS_HINTS
	if( batchCount == 0 )
	{
		renderContext.RenderPassHint( {}, { Tr2LoadAction::CLEAR, Tr2StoreAction::STORE }, {} );
		Tr2PushPopRT pprt( *rt, renderContext, slot );
		renderContext.Clear( CLEARFLAGS_TARGET, 0, 0, 0, 1 );
	}
#endif
}

void EveSpaceScene::SetupPlanetsAsShadowCaster( CcpMath::Sphere* planets, size_t maxPlanets )
{
	std::vector<PlanetInfo> visiblePlanets;
	for( EvePlanet* obj : m_planets )
	{
		float pixelSize = obj->GetEstimatedPixelDiameter();
		if( pixelSize > 50.0f )
		{
			// we don't want the sun to be a shadow caster
			if( obj->GetTranslationCurve() != nullptr && obj->GetTranslationCurve() == m_sunBall )
			{
				continue;
			}
			Vector3 planetWorldPos = obj->GetWorldPosition();
			float radius = obj->GetRadius();
			PlanetInfo p = { { Vector4( planetWorldPos, radius ) }, pixelSize };
			visiblePlanets.emplace_back( p );
		}
	}

	// sort the visiblePlanet list by the pixelSize so we get the 2 largest ones
	std::sort( visiblePlanets.begin(), visiblePlanets.end(), []( const PlanetInfo& a, const PlanetInfo& b ) {
		return a.pixelSize > b.pixelSize;
	} );

	// cut unnecessary objects from the list, or if we only have 1 planet the other index is just filled with 0 values
	visiblePlanets.resize( maxPlanets );

	for( size_t i = 0 ; i < maxPlanets ; i++ )
	{
		planets[i] = CcpMath::Sphere( visiblePlanets[i].sphere );
	}
}

void EveSpaceScene::SetupPlanetsAsShadowCaster( Tr2RenderContext& renderContext )
{
	CcpMath::Sphere planets[2];
	SetupPlanetsAsShadowCaster( planets, 2 );

	m_planetPerObjData.planetSphere[0] = Vector4( planets[0].center, planets[0].radius );
	m_planetPerObjData.planetSphere[1] = Vector4( planets[1].center, planets[1].radius );
	m_planetPerObjBuffer->SetData( (void*)&m_planetPerObjData, sizeof( m_planetPerObjData ) );
	m_planetPerObjBuffer->ApplyBuffer( renderContext );
}

void EveSpaceScene::GetPickingResults( Tr2PickBuffer& pickBuffer, Tr2RenderContext& renderContext, unsigned short& objId, unsigned short& areaId, float& depth )
{
	const void* data;
	uint32_t pitch;
	if( pickBuffer.PrepareGetResults( data, pitch, renderContext ) )
	{
		DecodeBufferPixel( data, objId, areaId, depth );
		pickBuffer.UnlockBuffer( renderContext );
	}
}

void EveSpaceScene::DecodeBufferPixel( const void* pBuffer, unsigned short& objId, unsigned short& areaId, float& depth ) const
{
	// helpers: get each channel
	unsigned int a = (unsigned int)( *( (unsigned char*)pBuffer + 3 ) );
	unsigned int r = (unsigned int)( *( (unsigned char*)pBuffer + 2 ) );
	unsigned int g = (unsigned int)( *( (unsigned char*)pBuffer + 1 ) );
	unsigned int b = (unsigned int)( *( (unsigned char*)pBuffer + 0 ) );
	// put it "together"
	objId = (unsigned short)( ( ( r & 0xff ) << 8 ) | ( g & 0xff ) );
	objId--;
	areaId = (unsigned short)( ( ( b & 0xff ) << 8 ) | ( a & 0xff ) );
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
			( *it )->RenderDebugInfo( *m_debugRenderer );
		}

		for( auto it = m_distanceFields.begin(); it != m_distanceFields.end(); ++it )
		{
			( *it )->RenderDebugInfo( *m_debugRenderer );
		}

		if( m_virtualCameraSystem )
		{
			m_virtualCameraSystem->RenderDebugInfo( *m_debugRenderer );
		}

		if( m_freezeFrustum && ( m_shadowQuality == SHADOW_HIGH || m_shadowQuality == SHADOW_LOW ) )
		{
			SetupCascadedShadows( renderContext );
		}

		m_debugRenderer->EndRender( renderContext );

		Tr2Renderer::RenderDebugInfo( renderContext );
	}
}

Vector3 EveSpaceScene::PickInfinity( int x, int y, Matrix proj, Matrix view )
{
	float xp, yp;
	gTriDev->ScreenToProjection( x, y, &xp, &yp );

	// Get the startpoint in world coordinates and pick ray
	Vector3 dir;
	Vector3 startWorld;
	ConvertProjectionCoordToWorldPickRay( xp, yp, &proj, &view, &startWorld, &dir );

	return dir;
}

void EveSpaceScene::UpdateSceneFromScript( Be::Time time )
{
	Update( time, time );
}

bool EveSpaceScene::GetPredicate( const char* name ) const
{
	if( strcmp( name, "hasBackgroundDistortionBatches" ) == 0 )
	{
		return m_hasBackgroundDistortionBatches;
	}
	else if( strcmp( name, "hasForegroundDistortionBatches" ) == 0 )
	{
		return m_hasForegroundDistortionBatches;
	}
	return false;
}

// Checks if we have a reflection probe
bool EveSpaceScene::HasReflectionProbe() const
{
	return m_reflectionProbe && m_reflectionProbe->IsValid();
}


void EveSpaceScene::ReregisterEntities()
{
	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		if( EveEntityPtr entity = BlueCastPtr( *it ) )
		{
			m_componentRegistry->ReRegister( entity );
		}
	}
}


void EveSpaceScene::ClearComponentRegistry()
{
	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		if( EveEntityPtr entity = BlueCastPtr( *it ) )
		{
			entity->UnRegister( m_componentRegistry );
		}
	}
	m_componentRegistry = nullptr;
}

Tr2DepthStencil* EveSpaceScene::GetDepth()
{
	return m_depthMap;
}

Matrix EveSpaceScene::GetReprojectionMatrix() const
{
	return m_reprojectionMatrix;
}
