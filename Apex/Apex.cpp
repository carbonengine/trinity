#include "StdAfx.h"

#if APEX_ENABLED

#include "Apex.h"

#include "Tr2ApexScene.h"
#include "Interior/Tr2InteriorSHLightingSolver.h"


#include "Tr2ApexRenderer.h"
#include "Tr2ClothingActor.h"
#include "include/ITr2DebugRenderer.h"
#include "TriSettingsRegistrar.h"
#include "TriRenderBatch.h"

#define USE_PLATFORM_ANALYZER CCP_STATS_ENABLED

#if USE_PLATFORM_ANALYZER

#include "PlatformAnalyzer.h"

namespace PLATFORM_ANALYZER
{
	static PLATFORM_ANALYZER::PlatformAnalyzer *gPlatformAnalyzer=NULL;
};

#endif



TriPoolAllocator* g_apexDebugRenderAllocator = NULL;
ITriRenderBatchAccumulator* g_apexDebugRenderBatches = NULL;

extern ITr2DebugRendererPtr g_debugRenderer;

bool g_apexVisualizeOnly = false;
TRI_REGISTER_SETTING( "apexVisualizeOnly", g_apexVisualizeOnly );
bool							g_apexClothSimParallel=false;
bool							g_apexParallelPhysXMeshSkinning=false;
bool							g_apexParallelMeshMeshSkinning=false;
bool							g_apexParallelCpuSkinning=true;

TRI_REGISTER_SETTING( "apexClothSimParallel",			g_apexClothSimParallel );
TRI_REGISTER_SETTING( "apexParallelPhysXMeshSkinning",	g_apexParallelPhysXMeshSkinning );
TRI_REGISTER_SETTING( "apexParallelMeshMeshSkinning",	g_apexParallelMeshMeshSkinning );
TRI_REGISTER_SETTING( "apexParallelCpuSkinning",		g_apexParallelCpuSkinning );

std::vector<physx::apex::NxApexActor* > g_apexDelayRelease;
std::vector<BlueAsyncRes* >				g_apexDelayReload;


physx::apex::NxModule* Tr2Apex::LoadModule(const char* moduleName)
{
	physx::apex::NxModule* module = NULL;
	PX_ASSERT(m_apexSDK);
	if (m_apexSDK)
	{
		module = m_apexSDK->createModule(moduleName);
		PX_ASSERT(module);
		if (module)
		{
			if( NxParameterized::Interface* params = module->getDefaultModuleDesc() )
			{
				NxParameterized::setParamBool( *params, "allowAsyncCooking", false );
				module->init( *params );
			}
		}
	}
	return module;
}

void Tr2Apex::ApexDelayReleaseActor( physx::apex::NxApexActor* actor )
{
	if( actor )
	{
		g_apexDelayRelease.push_back( actor );
	}
}

void Tr2Apex::ApexDelayReload( BlueAsyncRes* resource )
{
	if( resource )
	{
		g_apexDelayReload.push_back( resource );
	}
}

void Tr2Apex::OnApexAssetReleased( physx::apex::NxApexAsset* asset )
{
	if( asset )
	{
		for( size_t i = 0; i != g_apexDelayRelease.size(); ++i )
		{
			if( g_apexDelayRelease[i] && g_apexDelayRelease[i]->getOwner() == asset )
			{
				g_apexDelayRelease[i] = NULL;
			}
		}
	}
}

void Tr2Apex::PerformDelayedActions()
{
	for( size_t i = 0; i != g_apexDelayRelease.size(); ++i )
	{
		if( g_apexDelayRelease[i] != NULL )
		{
			g_apexDelayRelease[i]->release();
		}
	}
	g_apexDelayRelease.resize(0);

	for( size_t i = 0; i != g_apexDelayReload.size(); ++i )
	{
		if( g_apexDelayReload[i] != NULL )
		{
			g_apexDelayReload[i]->Reload();
		}
	}
	g_apexDelayReload.resize(0);
}

Tr2Apex *g_Tr2Apex=NULL;

Tr2Apex::Tr2Apex(  IRoot* lockobj /* = 0 */ ) : 
	m_apexModuleClothing(NULL)
	, m_physXSDK(NULL)
	, m_apexSDK(NULL)
	, m_frameworkLegacy(NULL)
	, m_clothingLegacy(NULL)
	, s_isClothSimInProgress(false)
	, m_debugVisualize(false)
	, m_fallbackSkinning(false)
	, m_enableLod(true)
	, m_lodBenefitValue(200)
	,m_apexViewMatrixId(0)
	,m_apexProjectionMatrixId(0)
{
	g_Tr2Apex = this;
}

bool Tr2Apex::GetIsVisualizationEnabled()
{
	if( m_physXSDK )
	{
		return 1.0f == m_physXSDK->getParameter( NX_VISUALIZATION_SCALE );
	}

	return false;
}

void Tr2Apex::SetIsVisualizationEnabled( bool val )
{
	m_debugVisualize = val;
	if( m_physXSDK )
	{
		m_physXSDK->setParameter( NX_VISUALIZATION_SCALE, val ? 1.0f : 0.0f );
	}
}

bool Tr2Apex::GetVisualizeCollisionShapes()
{
	if( m_physXSDK )
	{
		return 1.0f == m_physXSDK->getParameter( NX_VISUALIZE_COLLISION_SHAPES );
	}

	return false;
}

void Tr2Apex::SetVisualizeCollisionShapes( bool val )
{
	if( m_physXSDK )
	{
		m_physXSDK->setParameter( NX_VISUALIZE_COLLISION_SHAPES, val ? 1.0f : 0.0f );
	}
}

bool Tr2Apex::GetRendererVerbose()
{
	extern bool g_apexRendererVerbose;
	return g_apexRendererVerbose;
}

void Tr2Apex::SetRendererVerbose( bool val )
{
	extern bool g_apexRendererVerbose;
	g_apexRendererVerbose = val;
}

bool Tr2Apex::GetVisualizeOnly()
{
	extern bool g_apexVisualizeOnly;
	return g_apexVisualizeOnly;
}

void Tr2Apex::SetVisualizeOnly( bool val )
{
	extern bool g_apexVisualizeOnly;
	g_apexVisualizeOnly = val;
}

bool Tr2Apex::GetClothSimParallel()
{
	return g_apexClothSimParallel;
}

void Tr2Apex::SetClothSimParallel( bool val )
{
	g_apexClothSimParallel = val;
}

bool Tr2Apex::GetParallelPhysxMeshSkinning()
{
	return g_apexParallelPhysXMeshSkinning;
}

void Tr2Apex::SetParallelPhysxMeshSkinning( bool val )
{
	g_apexParallelPhysXMeshSkinning = val;
}

bool Tr2Apex::GetParallelMeshMeshSkinning()
{
	return g_apexParallelMeshMeshSkinning;
}

void Tr2Apex::SetParallelMeshMeshSkinning( bool val )
{
	g_apexParallelMeshMeshSkinning = val;
}

bool Tr2Apex::GetParallelCpuSkinning()
{
	return g_apexParallelCpuSkinning;
}

void Tr2Apex::SetParallelCpuSkinning( bool val )
{
	g_apexParallelCpuSkinning = val;
}

class ApexResourceCallback : public physx::apex::NxResourceCallback
{
public:
	ApexResourceCallback() 
	{
	}

	void *requestResource(const char *nameSpace, const char *name)
	{
		CCP_LOG("APEX: requestResource(%s,%s)\r\n",nameSpace,name);
		return NULL;
	}

	void releaseResource(const char *nameSpace, const char *name, void *resource)
	{

	}

protected:
};


ApexResourceCallback g_apexResourceCallback;

class ApexAllocator : public physx::PxAllocatorCallback
{
public:
	inline const char * lastSlash(const char *str)
	{
		const char *lastSlash = str;
		while ( *str )
		{
			if ( *str == '/' || *str == '\\' )
			{
				lastSlash = str+1;
			}
			str++;
		}
		return lastSlash;
	}

		/**
	\brief Allocates size bytes of memory, which must be 16-byte aligned.

	This method hould never return NULL.  If you run out of memory, then
	you should terminate the app or take some other appropriate action.

	<b>Threading:</b> This function should be thread safe as it can be called in the context of the user thread 
	and physics processing thread(s).

	\param size			Number of bytes to allocate.
	\param typeName		Name of the datatype that is being allocated
	\param filename		The source file which allocated the memory
	\param line			The source line which allocated the memory
	\return				The allocated block of memory.
	*/
	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line)
	{
		void* p = CCPAlignedMallocWithTracking(size, 16, "Apex", filename, line);

#if USE_PLATFORM_ANALYZER
		if ( PLATFORM_ANALYZER::gPlatformAnalyzer )
		{
			if ( typeName == NULL || strlen(typeName) == 0 )
			{
				typeName = lastSlash(filename);
			}
			PLATFORM_ANALYZER::gPlatformAnalyzer->trackAlloc(p,size,PLATFORM_ANALYZER::PlatformAnalyzer::MT_MALLOC,"APEX",typeName,filename,line);
		}
#endif
		return p;
	}

	/**
	\brief Frees memory previously allocated by allocate().

	<b>Threading:</b> This function should be thread safe as it can be called in the context of the user thread 
	and physics processing thread(s).

	\param ptr Memory to free.
	*/
	virtual void deallocate(void* ptr)
	{
		if ( ptr )
		{
#if USE_PLATFORM_ANALYZER
			if ( PLATFORM_ANALYZER::gPlatformAnalyzer )
			{
				PLATFORM_ANALYZER::gPlatformAnalyzer->trackFree(ptr,PLATFORM_ANALYZER::PlatformAnalyzer::MT_FREE,"APEX",__FILE__,__LINE__);
			}
#endif
			CCPAlignedFreeWithTracking( ptr );
		}
	}


};

ApexAllocator g_apexAllocator;

class ApexErrorStream : public physx::PxErrorCallback
{
public:
	void reportError( physx::PxErrorCode::Enum code, const char * message, const char *file, int line )
	{
		switch( code )
		{
			case physx::PxErrorCode::eDEBUG_INFO:
				CCP_LOG( "PhysX info (%d): %s - (%s line: %d)", code, message, file, line );
				break;
			case physx::PxErrorCode::eDEBUG_WARNING:
				CCP_LOGWARN( "PhysX info (%d): %s - (%s line: %d)", code, message, file, line );
				break;

			default:
				CCP_LOGERR( "PhysX error (%d): %s - (%s line: %d)", code, message, file, line );
				break;
		}
	}
};

ApexErrorStream g_apexErrorStream;

void Tr2Apex::InitializeApex( NxPhysicsSDK* sdk,PLATFORM_ANALYZER::PlatformAnalyzer *platformAnalyzer)
{
#ifdef _WIN64
	// TODO: Sort out Apex config issues in 64bit
	return;
#else
	if( !sdk )
	{
		return;
	}

	m_physXSDK = sdk;

	physx::apex::NxApexSDKDesc apexSdkDesc;
	apexSdkDesc.allocator = &g_apexAllocator;
	apexSdkDesc.outputStream = &g_apexErrorStream;
	apexSdkDesc.physXSDK = sdk;
	apexSdkDesc.cooking = NxGetCookingLib( NX_PHYSICS_SDK_VERSION );
	apexSdkDesc.renderResourceManager = g_apexRenderResourceManager;
	apexSdkDesc.resourceCallback = &g_apexResourceCallback;
#if USE_PLATFORM_ANALYZER
	PLATFORM_ANALYZER::gPlatformAnalyzer = platformAnalyzer;
	apexSdkDesc.platformAnalyzer = platformAnalyzer;
#endif

	physx::apex::NxApexCreateError errorCode;
	m_apexSDK = NxCreateApexSDK( apexSdkDesc, &errorCode, NX_APEX_SDK_VERSION );

	if( !m_apexSDK )
	{
		CCP_LOGERR( "Failed to initialize Apex SDK: %d", errorCode );
		return;
	}

	if( apexSdkDesc.cooking )
	{
		apexSdkDesc.cooking->NxInitCooking( &sdk->getFoundationSDK().getAllocator(), sdk->getFoundationSDK().getErrorStream() );
	}

	physx::apex::NxModule* tmp = m_apexSDK->createModule( "Clothing" );
	m_apexModuleClothing = static_cast<physx::apex::NxModuleClothing*>( tmp );
	if( !m_apexModuleClothing )
	{
		return;
	}

	m_frameworkLegacy = LoadModule("Framework_Legacy");
	m_clothingLegacy = LoadModule("Clothing_Legacy");

	g_apexDebugRenderAllocator = CCP_NEW( "g_apexDebugRenderAllocator" ) TriPoolAllocator();
	g_apexDebugRenderBatches = CCP_NEW( "g_apexDebugRenderBatches" ) TriRenderBatchAccumulator<>( g_apexDebugRenderAllocator );

	NxParameterized::Interface* moduleDesc = m_apexModuleClothing->getDefaultModuleDesc();

	if( moduleDesc )
	{
		NxParameterized::setParamBool( *moduleDesc, "allowAsyncCooking", false );	
	}

	m_apexModuleClothing->init( *moduleDesc );
	m_apexModuleClothing->setLODEnabled(m_enableLod);
	m_apexModuleClothing->setLODBenefitValue(m_lodBenefitValue);
#endif
}

namespace
{

void InitializeApex( IPhysXSdk* p )
{
	if( !g_Tr2Apex )
	{
		return;
	}

	if( g_Tr2Apex->GetApexSDK() )
	{
		// Already been initialized
		return;
	}

	if( !p )
	{
		return;
	}

	NxPhysicsSDK* sdk = p->GetSdk();
	PLATFORM_ANALYZER::PlatformAnalyzer *platformAnalyzer = p->GetPlatformAnalyzer();
	g_Tr2Apex->InitializeApex( sdk, platformAnalyzer );
}

}

MAP_FUNCTION_AND_WRAP( "InitializeApex", InitializeApex, "Initialize the Apex SDK (for cloth simulation)");

void ShutdownApex()
{
	if ( g_Tr2Apex )
	{
		delete g_Tr2Apex;
		g_Tr2Apex = NULL;
	}
}

#define APEX_RELEASE(x) if ( x ) { x->release(); x = NULL; }

Tr2Apex::~Tr2Apex(void)
{
	ShutdownApex();
}

void Tr2Apex::ShutdownApex(void)
{
	APEX_RELEASE(m_apexModuleClothing);
	APEX_RELEASE(m_clothingLegacy);
	APEX_RELEASE(m_frameworkLegacy);
	APEX_RELEASE(m_apexSDK);

	CCP_DELETE g_apexDebugRenderBatches;
	g_apexDebugRenderBatches = NULL;

	CCP_DELETE g_apexDebugRenderAllocator;
	g_apexDebugRenderAllocator = NULL;
}

namespace
{

void ConnectRemoteDebugger()
{
	NxPhysicsSDK *physXSDK = g_Tr2Apex->GetPhysXSDK();
	if( physXSDK )
	{
		if (!physXSDK->getFoundationSDK().getRemoteDebugger()->isConnected())
		{
			physXSDK->getFoundationSDK().getRemoteDebugger()->connect("localhost");
		}
	}
	else
	{
		CCP_LOGERR( "PhysXConnectRemoteDebugger: No PhysX SDK");
	}
}

}

MAP_FUNCTION_AND_WRAP( "PhysXConnectRemoteDebugger", ConnectRemoteDebugger, "" );

void Tr2Apex::RenderDebugInfo()
{
}

void RenderDebugRenderables( const NxDebugRenderable* dr )
{
	unsigned int lineCount = dr->getNbLines();
	const NxDebugLine* lines = dr->getLines();
	for( unsigned int i = 0; i < lineCount; ++i )
	{
		const NxDebugLine& l = lines[i];
		Color color( l.color );
		color.a = 1.0f;
		Vector3 from( l.p0.x, l.p0.y, l.p0.z );
		Vector3 to( l.p1.x, l.p1.y, l.p1.z );
		g_debugRenderer->DrawLine( from, to, color );
	}

	unsigned int triCount = dr->getNbTriangles();
	const NxDebugTriangle* triangles = dr->getTriangles();
	for( unsigned int i = 0; i < triCount; ++i )
	{
		const NxDebugTriangle& tri = triangles[i];

		Color color( tri.color );
		color.a = 1.0f;

		Vector3 p0( tri.p0.x, tri.p0.y, tri.p0.z );
		Vector3 p1( tri.p1.x, tri.p1.y, tri.p1.z );
		Vector3 p2( tri.p2.x, tri.p2.y, tri.p2.z );

		g_debugRenderer->DrawLine( p0, p1, color );
		g_debugRenderer->DrawLine( p1, p2, color );
		g_debugRenderer->DrawLine( p2, p0, color );
	}

	unsigned int pointCount = dr->getNbPoints();
	const NxDebugPoint* points = dr->getPoints();
	g_debugRenderer->DrawPointCloud( pointCount, (float*)points, sizeof( NxDebugPoint ) );

}

bool Tr2Apex::ApexSceneIsParallel()
{
	return g_apexClothSimParallel;
}

unsigned int Tr2Apex::ApexGetFrameDelay()
{
	unsigned int frameDelay = 0;

#if APEX_ENABLED
	if( g_apexClothSimParallel )
	{
		++frameDelay;
	}
	if( g_apexParallelPhysXMeshSkinning )
	{
		++frameDelay;
	}
	if( g_apexParallelMeshMeshSkinning )
	{
		++frameDelay;
	}
#endif

	return frameDelay;
}

bool Tr2Apex::ApexParallelPhysxMeshSkinning()
{
	return g_apexParallelPhysXMeshSkinning;
}

bool Tr2Apex::ApexGetParallelMeshMeshSkinning()
{
	return g_apexParallelMeshMeshSkinning;
}

bool Tr2Apex::ApexGetParallelCpuSkinning()
{
	return g_apexParallelCpuSkinning;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gathers rendering batches for a vector of Apex meshes. 
// Arguments:
//   clothMeshes - A vector of Apex meshes
//   batchType - A TriBatchType enum value: type of batches to render
//   perObjectData - Per-object data
//   perObjectSHData - Per-object data for SH lighting (optional, used only for transparent
//                     meshes that require SH lighting).
//   depth - Depth value used for batch sorting (optional)
// --------------------------------------------------------------------------------------
void Tr2Apex::ApexGatherBatches( Tr2ClothingActorVector& clothMeshes, 
					    ITriRenderBatchAccumulator* batches, 
						int batchType, 
						const Tr2PerObjectData* perObjectData, 
						ITr2InteriorSHLightingSolver* shSolver,
						unsigned int depth )
{
	g_apexRenderer.SetAccumulator( batches );
	g_apexRenderer.SetPerObjectData( perObjectData );
	g_apexRenderer.SetDepth( depth );
	for( Tr2ClothingActorVector::iterator it = clothMeshes.begin(); it != clothMeshes.end(); ++it )
	{
		if( batchType == TRIBATCHTYPE_DEPTHNORMAL )
		{
			// Skip depth normal batches for hair
			if( (*it)->GetDepthNormalEffect() == NULL || (*it)->GetDepthNormalEffect()->GetShaderStateInterface() == NULL )
			{
				continue;
			}
			g_apexRenderer.SetEffect( (*it)->GetDepthNormalEffect() );
			g_apexRenderer.SetReversedEffect( (*it)->GetDepthNormalEffectReversed() );
		}
		else if( batchType == TRIBATCHTYPE_DEPTH )
		{
			// Skip depth normal batches for hair
			if( (*it)->GetDepthEffect() == NULL || (*it)->GetDepthEffect()->GetShaderStateInterface() == NULL )
			{
				continue;
			}
			g_apexRenderer.SetEffect( (*it)->GetDepthEffect() );
			g_apexRenderer.SetReversedEffect( (*it)->GetDepthEffectReversed() );
		}
		else
		{
			if( ( batchType == TRIBATCHTYPE_DECAL || batchType == TRIBATCHTYPE_DECAL_PREPASS ) && (*it)->GetUseTransparentBatches() )
			{
				continue;
			}
			if( batchType == TRIBATCHTYPE_TRANSPARENT && !(*it)->GetUseTransparentBatches() )
			{
				continue;
			}
			g_apexRenderer.SetPerObjectData( perObjectData );
			if( shSolver && ( *it )->GetUseTransparentBatches() && ( *it )->GetUseSHLighting() )
			{
				Tr2PerAreaSHLightingData* shAreaData = batches->Allocate<Tr2PerAreaSHLightingData>();
				if( shAreaData )
				{
					AxisAlignedBoundingBox aabb = ( *it )->GetWorldBoundingBox();

					shAreaData->SetPerObjectData( perObjectData );
					shSolver->AddVolume( aabb.m_min, aabb.m_max, Tr2Renderer::GetIdentityTransform(), shAreaData );

					g_apexRenderer.SetPerObjectData( shAreaData );
				}
			}

			g_apexRenderer.SetEffect( (*it)->GetEffect() );
			g_apexRenderer.SetReversedEffect( (*it)->GetEffectReversed() );
		}

		if( !g_apexVisualizeOnly )
		{
			physx::apex::NxApexRenderable* r = (*it)->GetApexRenderable();
			if( r )
			{
				r->lockRenderResources();
				r->updateRenderResources();
				r->unlockRenderResources();
				r->dispatchRenderResources( g_apexRenderer );
			}
		}
	}
	g_apexRenderer.SetAccumulator( NULL );
	g_apexRenderer.SetEffect( NULL );
	g_apexRenderer.SetReversedEffect( NULL );
	g_apexRenderer.SetDepth( 0xFFFFFFFF );
}

void Tr2Apex::CoreDumpScene(void)
{

}

void Tr2Apex::MemoryUsageReport(void)
{
#if USE_PLATFORM_ANALYZER
	if ( PLATFORM_ANALYZER::gPlatformAnalyzer )
	{
		PLATFORM_ANALYZER::gPlatformAnalyzer->memoryReport("CCP_PhysX_Apex_MemoryReport.html",false);
	}
#endif
}

bool Tr2Apex::GetFallbackSkinning(void)
{
	return m_fallbackSkinning;
}

void Tr2Apex::SetFallbackSkinning(bool val)
{
	m_fallbackSkinning = val;
}

bool Tr2Apex::GetEnableLOD(void)
{
	return m_enableLod;
}

void Tr2Apex::SetEnableLOD(bool val)
{
	m_enableLod = val;
	if ( m_apexModuleClothing )
	{
		m_apexModuleClothing->setLODEnabled(val);
	}
}

float Tr2Apex::GetLodBenefitValue(void)
{
	return m_lodBenefitValue;
}

void Tr2Apex::SetLodBenefitValue(float v)
{
	m_lodBenefitValue = v;
	if ( m_apexModuleClothing )
	{
		m_apexModuleClothing->setLODBenefitValue(v);
	}
}

//******************


namespace
{

void LaunchApexParamEditor( IPhysXSdk* )
{
	g_Tr2Apex->LaunchApexParamEditor(NULL);
}

}

MAP_FUNCTION_AND_WRAP( "LaunchApexParamEditor", LaunchApexParamEditor, "Launch the APEX parameter editor");



void	Tr2Apex::LaunchApexParamEditor(Tr2ApexEmitterActor *emitter)
{

}

bool Tr2Apex::IsClothSimInProgress()
{
	return Tr2ApexScene::s_numClothSimInProgress != 0;
}

#endif // APEX_ENABLED

