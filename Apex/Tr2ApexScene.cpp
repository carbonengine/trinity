#include "StdAfx.h"

#if APEX_ENABLED

#include "Tr2ApexScene.h"

#include "Apex.h"
#include "Tr2ApexRenderer.h"
#include "include/ITr2DebugRenderer.h"
#include "TriSettingsRegistrar.h"
#include "TriDebugResourceHelper.h"
#include "TriRenderBatch.h"
#include "Tr2Effect.h"

#define USE_PLATFORM_ANALYZER CCP_STATS_ENABLED

#if USE_PLATFORM_ANALYZER

#include "PlatformAnalyzer.h"

namespace PLATFORM_ANALYZER
{
	static PLATFORM_ANALYZER::PlatformAnalyzer *gPlatformAnalyzer=NULL;
};

extern bool							g_apexClothSimParallel;
extern bool							g_apexParallelPhysXMeshSkinning;
extern bool							g_apexParallelMeshMeshSkinning;
extern bool							g_apexParallelCpuSkinning;

extern TriPoolAllocator* g_apexDebugRenderAllocator;
extern ITriRenderBatchAccumulator* g_apexDebugRenderBatches;

#endif

extern ITr2DebugRendererPtr g_debugRenderer;

unsigned Tr2ApexScene::s_numClothSimInProgress = 0;

Tr2ApexScene::Tr2ApexScene(  IRoot* lockobj /* = 0 */ )
	: m_physXScene( NULL )
	, m_apexScene( NULL )
	, m_apexModuleClothingDbgParams( NULL )
	, m_debugVisualize( false )
	, m_apexViewMatrixId( 0 )
	, m_apexProjectionMatrixId( 0 )
	, m_hasSimulated( false )
	, m_isClothSimInProgress( false )
	, m_lastUpdateTime( 0 )
{
}

bool Tr2ApexScene::GetIsVisualizationEnabled()
{
	return m_apexScene && m_debugVisualize;
}


void Tr2ApexScene::SetIsVisualizationEnabled( bool val )
{
	m_debugVisualize = val;
	if( val && g_Tr2Apex && !g_Tr2Apex->GetIsVisualizationEnabled() )
	{
		g_Tr2Apex->SetIsVisualizationEnabled( val );
	}
	if( m_apexScene )
	{
		NxParameterized::Interface* debugRenderParams = m_apexScene->getDebugRenderParams();
		NxParameterized::setParamBool( *debugRenderParams, "VISUALIZATION_ENABLE", val );
		NxParameterized::setParamF32( *debugRenderParams, "VISUALIZATION_SCALE", 1.0f );
	}
}

unsigned int Tr2ApexScene::GetNbActorsInPhysxScene()
{
	if( m_physXScene )
	{
		return m_physXScene->getNbActors();
	}

	return 0;
}

unsigned int Tr2ApexScene::GetNbStaticShapesInPhysxScene()
{
	if( m_physXScene )
	{
		return m_physXScene->getNbStaticShapes();
	}

	return 0;
}

unsigned int Tr2ApexScene::GetNbDynamicShapesInPhysxScene()
{
	if( m_physXScene )
	{
		return m_physXScene->getNbDynamicShapes();
	}

	return 0;
}

unsigned int Tr2ApexScene::GetNbJointsInPhysxScene()
{
	if( m_physXScene )
	{
		return m_physXScene->getNbJoints();
	}

	return 0;
}

unsigned int Tr2ApexScene::GetNbEffectorsInPhysxScene()
{
	if( m_physXScene )
	{
		return m_physXScene->getNbEffectors();
	}

	return 0;
}

void Tr2ApexScene::DumpStats()
{
	if( m_apexScene )
	{
		const physx::apex::NxApexSceneStats* stats = m_apexScene->getStats();
		unsigned int n = stats->numApexStats;
		physx::apex::ApexStatsInfo* p = stats->ApexStatsInfoPtr;
		for( unsigned int i = 0; i < n; ++i )
		{
			switch( p->StatType )
			{
				case physx::apex::STATS_TYPE_STRING:
				case physx::apex::STATS_TYPE_ENUM:
					CCP_LOG( "%s: %s", p->StatName, p->StatCurrentValue.String );
					break;

				case physx::apex::STATS_TYPE_INT:
					CCP_LOG( "%s: %d", p->StatName, p->StatCurrentValue.Int );
					break;

				case physx::apex::STATS_TYPE_FLOAT:
					CCP_LOG( "%s: %f", p->StatName, p->StatCurrentValue.Float );
					break;

				case physx::apex::STATS_TYPE_BOOL:
					CCP_LOG( "%s: %s", p->StatName, p->StatCurrentValue.Bool ? "True" : "False" );
					break;

				default:
					CCP_LOG( "%s: <Unknown type>", p->StatName );

			}

			++p;
		}
	}
}

void Tr2ApexScene::CreateScene()
{
	if( !g_Tr2Apex || !g_Tr2Apex->GetApexSDK() || !g_Tr2Apex->GetPhysXSDK() )
	{
		return;
	}

	NxSceneDesc physxSceneDesc;
	physxSceneDesc.flags &= ~NX_SF_SIMULATE_SEPARATE_THREAD;
	physxSceneDesc.maxIter = 1;
	physxSceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	physxSceneDesc.gravity.set( 0.0f, -9.8f, 0.0f );
	m_physXScene = g_Tr2Apex->GetPhysXSDK()->createScene( physxSceneDesc );

	physx::apex::NxApexSceneDesc sceneDesc;
	sceneDesc.scene = m_physXScene;
	m_apexScene = g_Tr2Apex->GetApexSDK()->createScene( sceneDesc );
	m_apexScene->setUseDebugRenderable( true );	// properly propagates the flag; sceneDesc doesn't, apex bug?
	m_apexViewMatrixId = m_apexScene->allocViewMatrix( physx::apex::ViewMatrixType::LOOK_AT_RH );
	m_apexProjectionMatrixId = m_apexScene->allocProjMatrix( physx::apex::ProjMatrixType::USER_CUSTOMIZED );

	m_apexModuleClothingDbgParams = m_apexScene->getModuleDebugRenderParams( "Clothing" );
}

void Tr2ApexScene::DeleteScene()
{
	float tmp;
	PreUpdate( 0, 0.0f, tmp );

	if( m_apexScene )
	{
		m_apexScene->release();
		m_apexScene = NULL;
	}

	if( m_physXScene && g_Tr2Apex && g_Tr2Apex->GetPhysXSDK()  )
	{
		g_Tr2Apex->GetPhysXSDK()->releaseScene( *m_physXScene );
		m_physXScene = NULL;
	}
}

Tr2ApexScene::~Tr2ApexScene(void)
{
	DeleteScene();
}

void RenderDebugRenderables( const NxDebugRenderable* dr );


void Tr2ApexScene::FetchResults( float& lodResourceBudgetConsumed )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_apexScene )
	{
		NxU32 errorState = 0;
		if( m_hasSimulated )
		{
			m_apexScene->fetchResults( true, &errorState );
			m_hasSimulated = false;
		}
		lodResourceBudgetConsumed = m_apexScene->getLODResourceConsumed();

		if( errorState != 0 )
		{
			CCP_LOGERR( "Apex errorState: %d", errorState );
		}

		if ( m_debugVisualize ) // only if debug visualization is enabled.
		{
			const NxDebugRenderable* apexDebugRenderable = m_apexScene->getDebugRenderable();
			if( g_debugRenderer )
			{
				RenderDebugRenderables( m_physXScene->getDebugRenderable() );
				RenderDebugRenderables( m_apexScene->getDebugRenderable() );
			}
		}
	}
}

void Tr2ApexScene::Simulate( Be::Time time, float lodResourceBudget )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_lastUpdateTime == 0 )
	{
		m_lastUpdateTime = time;
	}
	Be::Time delta = time - m_lastUpdateTime;
	m_lastUpdateTime = time;

	if( m_apexScene && delta )
	{
		float fDelta = TimeAsFloat( delta );

		// This is registered as a trinity setting in Tr2SkinnedObject.
		extern float g_clothTimeDeltaResetThreshold;

		if( fDelta > g_clothTimeDeltaResetThreshold )
		{
			fDelta = 0.0166f;
		}

		if( fDelta > 0.0f )
		{
			m_apexScene->setLODResourceBudget( lodResourceBudget );
			m_apexScene->setViewMatrix( *(const physx::PxMat44*)&Tr2Renderer::GetViewTransform(), m_apexViewMatrixId );
			m_apexScene->setProjMatrix( *(const physx::PxMat44*)&Tr2Renderer::GetProjectionTransform(), m_apexViewMatrixId );
			m_apexScene->setUseViewProjMatrix( m_apexViewMatrixId, m_apexProjectionMatrixId );
			m_apexScene->simulate( fDelta );
			m_hasSimulated = true;
		}
	}
}

void Tr2ApexScene::PreUpdate( Be::Time time, float lodResourceBudget, float& lodResourceBudgetConsumed )
{
	if( m_isClothSimInProgress )
	{
		CCP_STATS_ZONE( __FUNCTION__ );
		// Fetch results from last frame. Simulation is kicked off after rendering
		// has finished so we store the update time here and call ApexSceneSimulate
		// at the end of Render. This gives Apex the maximum chance of doing things
		// on a background thread or GPU so we don't have to wait for results.
		FetchResults( lodResourceBudgetConsumed );
		m_isClothSimInProgress = false;
		--s_numClothSimInProgress;

		if( g_Tr2Apex != NULL )
		{
			g_Tr2Apex->PerformDelayedActions();
		}
	}
}

void Tr2ApexScene::PostUpdate( Be::Time time, float lodResourceBudget, float& lodResourceBudgetConsumed )
{
	if( !g_apexClothSimParallel )
	{
		CCP_STATS_ZONE( __FUNCTION__ );
		Simulate( time, lodResourceBudget );
		m_isClothSimInProgress = true;
		++s_numClothSimInProgress;
	}
}

void Tr2ApexScene::PreRender( Be::Time time, float lodResourceBudget, float& lodResourceBudgetConsumed )
{
	if( m_isClothSimInProgress )
	{
		FetchResults( lodResourceBudgetConsumed );
		m_isClothSimInProgress = false;
		--s_numClothSimInProgress;
	}
}

void Tr2ApexScene::PostRender( Be::Time time, float lodResourceBudget, float& lodResourceBudgetConsumed )
{
	if( g_apexClothSimParallel )
	{
		// Kick off Apex simulation - any results that were required for rendering
		// should be consumed now.
		Simulate( time, lodResourceBudget );
		m_isClothSimInProgress = true;
		++s_numClothSimInProgress;
	}
}

void Tr2ApexScene::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	if( m_apexScene && g_apexDebugRenderBatches )
	{
		g_apexRenderer.SetPerObjectData( NULL );
		g_apexRenderer.SetEffect( g_debugResourceHelper.GetEffect() );
		g_apexRenderer.SetAccumulator( g_apexDebugRenderBatches );

		m_apexScene->lockRenderResources();
		m_apexScene->updateRenderResources();
		m_apexScene->dispatchRenderResources( g_apexRenderer );
		m_apexScene->unlockRenderResources();

		g_apexDebugRenderBatches->Finalize();

		renderContext.m_esm.RenderBatches( g_apexDebugRenderBatches );
		g_apexDebugRenderBatches->Clear();
		g_apexDebugRenderAllocator->Clear();
	}
}

bool Tr2ApexScene::CreatePlane( Vector3 position, Vector3 normal, float distance )
{
	if( NxScene *physXScene = GetPhysXScene() )
	{
		NxPlaneShapeDesc planeDesc;
		planeDesc.d = distance;
		planeDesc.normal = NxVec3( normal.x, normal.y, normal.z );
		planeDesc.localPose = NxMat34( true );

		NxActorDesc actorDesc;
		actorDesc.shapes.pushBack( &planeDesc );
		actorDesc.globalPose.t = NxVec3( position.x, position.y, position.z );
		actorDesc.globalPose.M.fromQuat( NxQuat( NxVec3(0, 0, 0), 1.0f ) );
		actorDesc.globalPose = NxMat34( true );

		// No rigid body or density for static shapes
		actorDesc.body = 0;
		actorDesc.density = 0;

		NxActor* actor = physXScene->createActor( actorDesc );
		return actor != NULL;
	}
	return false;
}

void Tr2ApexScene::SetLODResourceBudget(float val)
{
	if ( m_apexScene )
	{
		m_apexScene->setLODResourceBudget(val);
	}
}

bool Tr2ApexScene::GetApexClothingModuleSetting( const char* name )
{
	float val = 0.0f;
	if( m_apexScene )
	{
		NxParameterized::Interface* clothingDebugRenderParams = m_apexScene->getModuleDebugRenderParams("Clothing");
		if( !NxParameterized::getParamF32( *clothingDebugRenderParams, name, val ) )
		{
			CCP_LOGWARN( "Parameter '%s' does not exist in clothing module", name );
		}
	}

	return val != 0.0f;
}

void Tr2ApexScene::SetApexClothingModuleSetting( const char* name, bool val,float distance)
{
	if( m_apexScene )
	{
		NxParameterized::Interface* clothingDebugRenderParams = m_apexScene->getModuleDebugRenderParams("Clothing");
		if( !NxParameterized::setParamF32( *clothingDebugRenderParams, name, val ? distance : 0.0f ) )
		{
			CCP_LOGWARN( "Parameter '%s' does not exist in clothing module", name );
		}
	}
}

bool Tr2ApexScene::GetVisualizeClothingSkinnedPositions()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_SKINNED_POSITIONS" );
}

void Tr2ApexScene::SetVisualizeClothingSkinnedPositions( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_SKINNED_POSITIONS", val );
}

bool Tr2ApexScene::GetVisualizeClothingSphereCollisions()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_SPHERE_COLLISION" );
}

void Tr2ApexScene::SetVisualizeClothingSphereCollisions( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_SPHERE_COLLISION", val );
}

bool Tr2ApexScene::GetVisualizeClothingMaxDistanceOut()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_MAX_DISTANCE_OUT" );
}

void Tr2ApexScene::SetVisualizeClothingMaxDistanceOut( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_MAX_DISTANCE_OUT", val );
}

bool Tr2ApexScene::GetVisualizeClothingMaxDistanceIn()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_MAX_DISTANCE_IN" );
}

void Tr2ApexScene::SetVisualizeClothingMaxDistanceIn( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_MAX_DISTANCE_IN", val );
}

bool Tr2ApexScene::GetVisualizeClothingSkinMap()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_SKIN_MAP" );
}

void Tr2ApexScene::SetVisualizeClothingSkinMap( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_SKIN_MAP", val );
}

bool Tr2ApexScene::GetVisualizeClothingRenderNormals()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_RENDER_NORMALS" );
}

void Tr2ApexScene::SetVisualizeClothingRenderNormals( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_RENDER_NORMALS", val, 0.02f );
}

bool Tr2ApexScene::GetVisualizeClothingRenderTangents()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_RENDER_TANGENTS" );
}

void Tr2ApexScene::SetVisualizeClothingRenderTangents( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_RENDER_TANGENTS", val, 0.02f );
}

bool Tr2ApexScene::GetVisualizeClothingPhysicsMeshWire()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_PHYSICS_MESH_WIRE" );
}

void Tr2ApexScene::SetVisualizeClothingPhysicsMeshWire( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_PHYSICS_MESH_WIRE", val );
}

bool Tr2ApexScene::GetVisualizeClothingPhysicsMeshSolid()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_PHYSICS_MESH_SOLID" );
}

void Tr2ApexScene::SetVisualizeClothingPhysicsMeshSolid( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_PHYSICS_MESH_SOLID", val );
}

bool Tr2ApexScene::GetVisualizeClothingSkeleton()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_SKELETON" );
}

void Tr2ApexScene::SetVisualizeClothingSkeleton( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_SKELETON", val );
}

bool Tr2ApexScene::GetVisualizeClothingVelocities()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_VELOCITIES" );
}

void Tr2ApexScene::SetVisualizeClothingVelocities( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_VELOCITIES", val );
}

bool Tr2ApexScene::GetVisualizeClothingGraphicalVertexBones()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_GRAPHICAL_VERTEX_BONES" );
}

void Tr2ApexScene::SetVisualizeClothingGraphicalVertexBones( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_GRAPHICAL_VERTEX_BONES", val );
}

bool Tr2ApexScene::GetVisualizeClothingPhysicalVertexBones()
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_PHYSICAL_VERTEX_BONES" );
}

void Tr2ApexScene::SetVisualizeClothingPhysicalVertexBones( bool val )
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_PHYSICAL_VERTEX_BONES", val );
}

bool Tr2ApexScene::GetVisualizeClothingAllSkinMap(void)
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_ALL_SKIN_MAP" );
}

void Tr2ApexScene::SetVisualizeClothingAllSkinMap(bool val)
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_ALL_SKIN_MAP", val );
}

bool Tr2ApexScene::GetVisualizeClothingBadSkinMap(void)
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_BAD_SKIN_MAP" );
}

void Tr2ApexScene::SetVisualizeClothingBadSkinMap(bool val)
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_BAD_SKIN_MAP", val );
}

bool Tr2ApexScene::GetVisualizeClothingBoneFrames(void)
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_BONE_FRAMES" );
}

void Tr2ApexScene::SetVisualizeClothingBoneFrames(bool val)
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_BONE_FRAMES", val, 0.1f );
}

bool Tr2ApexScene::GetVisualizeClothingBoneNames(void)
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_BONE_NAMES" );
}

void Tr2ApexScene::SetVisualizeClothingBoneNames(bool val)
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_BONE_NAMES", val, 0.05f );
}

bool Tr2ApexScene::GetVisualizeClothingAcollisionUmbrellas(void)
{
	return GetApexClothingModuleSetting( "VISUALIZE_CLOTHING_ACOLLISION_UMBRELLAS" );
}

void Tr2ApexScene::SetVisualizeClothingAcollisionUmbrellas(bool val)
{
	SetApexClothingModuleSetting( "VISUALIZE_CLOTHING_ACOLLISION_UMBRELLAS", val );
}

#endif // APEX_ENABLED
