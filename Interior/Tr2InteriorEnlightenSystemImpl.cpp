////////////////////////////////////////////////////////////
//
//    Created:   May 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorEnlightenSystemImpl.h"

#include "TriEnlightenUtils.h"
#include "Tr2Renderer.h"
#include "Resources/Tr2RadiosityRes.h"

extern CcpLogChannel_t g_enlightenBuildChannel;

CCP_STATS_DECLARED_ELSEWHERE( triEnlightenObjectMemory );

Tr2InteriorEnlightenSystemImpl::Tr2InteriorEnlightenSystemImpl()
:	m_radResPath( "" ),
	m_systemID( 0 ),
	m_systemInCellIdx( 0 ),
	m_enlightenPixelSize( 1.0f ),
	m_radBuiltInternally( false ),
	m_previewBuild( false),
	m_albedoWorkspace( NULL ),
	m_inputWorkspace( NULL ),
	m_albedoWorkspaceData( NULL ),
	m_radSystem( NULL ),
	m_inputLightingCurrentFrame( NULL ),
	m_inputLightingNextFrame( NULL ),
	m_inputLightingList( NULL ),
	m_notificationTarget( NULL ),
	m_debugChartMap( NULL ),
	m_debugChartMapWidth( 0 ),
	m_debugChartMapHeight( 0 ),
	m_bounceData( NULL ),
	m_albedoTexture( NULL ),
	m_emissiveTexture( NULL )
{
}

Tr2InteriorEnlightenSystemImpl::~Tr2InteriorEnlightenSystemImpl()
{
	ReleaseWorkspace();
	ReleaseRadCore();

	if( m_radSystemResource )
	{
		m_radSystemResource->RemoveNotifyTarget( this );
		m_radSystemResource.Unlock();
	}
}

// -------------------------------------------------------------
// Description:
//   Initializes the system (tries to load .rad file).
//   The owner must call this method in its own Initialize method.
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::Initialize()
{
	SetRadiosityResource();
}

// -------------------------------------------------------------
// Description:
//   Watches for .rad file path variable modification.
//   The owner must call this method in its own OnModified method
//   if it exposes m_radResPath to Blue.
// Arguments:
//   value - Variable that was changed
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_radResPath ) )
	{		
		SetRadiosityResource( );	
	}
}

// -------------------------------------------------------------
// Description:
//   Implements IAsyncLoadedResNotifyTarget method for 
//   m_radSystemResource resource.
// Arguments:
//   p - Resource requesting data release
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::ReleaseCachedData( BlueAsyncRes* p )
{
	if( p == m_radSystemResource )
	{
		if( m_notificationTarget )
		{
			m_notificationTarget->ReleaseCachedData( p );
		}

		ReleaseWorkspace();
		ReleaseRadCore();
	}
}

// -------------------------------------------------------------
// Description:
//   Implements IAsyncLoadedResNotifyTarget method for 
//   m_radSystemResource resource. Re-creates Enlighten data
//   for loaded .rad file.
// Arguments:
//   p - Resource requesting data rebuild
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::RebuildCachedData( BlueAsyncRes* p )
{
	if ( p == m_radSystemResource && p->IsGood() )
	{
		if( m_notificationTarget )
		{
			m_notificationTarget->ReleaseCachedData( p );
		}

		ReleaseWorkspace();
		ReleaseRadCore();

		if( m_radSystemResource->GetSystemRadiosity() && m_radSystemResource->GetInputWorkspace() && m_radSystemResource->GetAlbedoWorkspaceData() )
		{
			m_radSystem = m_radSystemResource->GetSystemRadiosity();
			m_inputWorkspace = m_radSystemResource->GetInputWorkspace();
			m_albedoWorkspaceData = m_radSystemResource->GetAlbedoWorkspaceData();

			m_radBuiltInternally = false;
			m_previewBuild = false;

			CreateEnlightenWorkspaces();
		}

		// Set up the debug chart map
		m_radSystemResource->GetDebugChartMap( m_debugChartMap, m_debugChartMapWidth, m_debugChartMapHeight );

		if( m_notificationTarget )
		{
			m_notificationTarget->RebuildCachedData( p );
		}
	}
}

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// -------------------------------------------------------------
// Description:
//   Saves .rad file. Implements IAsyncLoadedResNotifyTarget method for 
//   m_radSystemResource resource. Re-creates Enlighten data
//   for loaded .rad file.
// Arguments:
//   geometries - List of geometries in the system. Their IDs
//                are saved into the file to have an ability
//				  to check if the file is in sync with loaded
//				  geometry.
// Return Value:
//   true If successfully saved .rad file
//   false If there was an error saving .rad file or the path 
//		   to the file is empty
// -------------------------------------------------------------
bool Tr2InteriorEnlightenSystemImpl::SaveEnlightenSystem( const std::vector<const TriGeometryRes*>& geometries )
{
	if( m_previewBuild )
	{
		CCP_LOGERR( "Can not save preview build" );		
		return false;
	}

	if ( !m_radSystem || !m_inputWorkspace || !m_albedoWorkspaceData )
	{
		CCP_LOGERR( "No radiosity data to write" );		
		return false;
	}

	if ( !m_radResPath.empty() )
	{
		std::wstring radResPathW = CA2W( m_radResPath.c_str() );
		std::wstring filepathW = BePaths->ResolvePathW( radResPathW );
		std::string filepath = CW2A( filepathW.c_str() );

		// TODO: Change signature of this function to take in a wide string
		return Tr2RadiosityRes::WriteWorkspaceResourceHashesToDisk(
			filepath,
			m_radSystem,
			m_inputWorkspace,
			m_albedoWorkspaceData,
			geometries,
			m_debugChartMapWidth, 
			m_debugChartMapHeight, 
			m_debugChartMap
			);
	}

	return false;
}

// -------------------------------------------------------------
// Description:
//   Packs geometries into Enlighten system. This is the first
//   step in building the system.
// Arguments:
//   quality - quality level for the built
//   pPrecompute - pointer to Enlighten precompute object
//   prog - progress log object
//   systemInCellIdx - index of the system in the cell
//   geometryProvider - pointer to the geometry provider object:
//						it is accessed to build a list of
//						packed geometries for the system
// Return Value:
//   true If successfully packed the system
//   false If there was an error or there were no geometries to pack
// -------------------------------------------------------------
bool Tr2InteriorEnlightenSystemImpl::PackEnlightenSystem( Quality quality, Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog, 
													  unsigned int systemInCellIdx, ITr2InteriorEnlightenGeometryProvider* geometryProvider )
{
	m_systemInCellIdx = systemInCellIdx;
	m_previewBuild = quality != QUALITY_RELEASE;

	float clusterSize;
	int samplesPerCluster;
	int irradianceQuality;
	int shQuality;

	switch( quality )
	{
	case QUALITY_PREVIEW:
		clusterSize = m_enlightenPixelSize * 1250.0f;
		samplesPerCluster = 32;
		irradianceQuality = Enlighten::PCQ_DRAFT;
		shQuality = Enlighten::PCQ_DRAFT;
		break;
	default: // QUALITY_RELEASE
		clusterSize = m_enlightenPixelSize * 250.0f;
		samplesPerCluster = 128;
		irradianceQuality = Enlighten::PCQ_MEDIUM;
		shQuality = Enlighten::PCQ_MEDIUM;
	}


	std::vector<Enlighten::IPrecompPackedGeometry*> geometries(0);

	Enlighten::IPrecompInputSystem* inputSystem = Enlighten::IPrecompInputSystem::Create();
	CCP_ASSERT( inputSystem != NULL );
	ON_BLOCK_EXIT( &Enlighten::IPrecompInputSystem::Release, inputSystem );// Can't use CComPtr

	Enlighten::IPrecompBuildParameters* buildParameters = inputSystem->GetBuildParameters();
	// See: http://core/wiki/Enlighten_Precompute_Parameters

	// The unit scale for the entire precompute. We're working in meters, not cm.
	const float enlightenUnitScale = 0.01f;
	buildParameters->SetUnitScale( enlightenUnitScale );

	// The size of an input cluster, in cm. Using 2.5*the pixel size as a starting value
	// Should be the same order of magnitude as the pixels
	// This doesn't have any benefit in being smaller than a pixel
	// Smaller sizes increase pre-compute (probably non-linear), but allow you to catch finer detail
	// Does NOT affect the runtime performance (within that order of magnitude)
	buildParameters->SetClusterSize( clusterSize );
	
	// Number of samples, per 2.5m square area
	// This determines the number of dusters in a cluster,
	// for which the size is determined above.
	// Decreasing this, the quality will go down, but both the pre-compute and runtime speed
	// would increase as well as memory
	buildParameters->SetSamplesPerCluster( samplesPerCluster );

	// Irradiance quality
	// Generally affects the quality of the result, but not so much the size of the result
	// We could crank this up, if we want a higher quality build, but this will have a large effect on the build time
	buildParameters->SetIrradianceQuality( irradianceQuality );

	// How much information to use for the radiosity computation - think of this like the JPEG compression level
	// Must be a multiple of 16 - affects the quality of the result, as well as the size and speed of the runtime (linearly)
	// Increasing this value increases the quality. Minimum is perhaps ~48 if irradiance is used
	// Makes very little difference to the build time
	buildParameters->SetIrradianceBudget( 64 );

	// Spherical Harmonics Quality (we're not using this at the moment)
	// Works similarly to the irradiance quality above
	buildParameters->SetSphericalQuality( shQuality );

	// Similar to the irradiance budget above
	// Must be a multiple of 4
	// Increasing this value increases the quality. Minimum is perhaps ~40 if SH is used
	buildParameters->SetSphericalBudget( 64 );

	// Effectively controls blending across buckets
	// If this is too big, it will cause over-blurring
	// Decreasing this will increase the build time
	// Decreasing this will increase the runtime memory cost (a little) and slow things down (as it performs more blending)
	buildParameters->SetViewVoxelSize( m_enlightenPixelSize * 400.0f );

	// A limit on how much of the cluster must be visible before it is taken into account
	// 1.0 = only use a cluster if the whole thing is visible
	// This might need to be adjusted if light is leaking through walls due to cluster sizes
	buildParameters->SetVisibilityThreshold( 0.0 );

	// This is currently (20/10/2010) bugged, and non-zero values will incorrectly
	//  remove the 'naughty' flag from (almost?) all overhanging pixels.
	buildParameters->SetValidOverhangDistance( 0 );

	inputSystem->SetSystemId( Geo::GeoGuid::Create( 1, m_systemID ) );	

	if( !geometryProvider->GetEnlightenPackedGeometry( pPrecompute, inputSystem, prog, m_enlightenPixelSize, geometries ) || geometries.size() == 0 )
	{
		return false;
	}

	Enlighten::IPrecompPackedSystem* packedSystem = NULL;
	//Geo::GeoAutoPtr<Enlighten::IPrecompPackedSystem> packedSystem = NULL;
	CCP_LOG_CH( g_enlightenBuildChannel, "Packing System %u,%u using %i geometry objects", inputSystem->GetId().High(), inputSystem->GetId().Low(), geometries.size() );
	pPrecompute->PackSystem( inputSystem, &geometries[0], (Geo::s32)geometries.size(), &prog, packedSystem );

	if( !packedSystem )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Failed to pack system. Enlighten returned NULL from Packsystem().");
		CCP_LOGERR_CH( g_enlightenBuildChannel, Tr2Renderer::GetEnlightenErrorBuffer() );
		Enlighten::ClearErrorBuffer();
		return false;
	}

	m_packedSystem.Delete();
	m_packedSystem = Geo::GeoAutoReleasePtr<Enlighten::IPrecompPackedSystem>( packedSystem );

	return true;
}

// -------------------------------------------------------------
// Description:
//   Creates pre clustering for the system. This is a second step in
//   building Enlighten.
// Arguments:
//   pPrecompute - pointer to Enlighten precompute object
//   neighbours - list of neighbour Enlighten systems (including this one)
//   prog - progress log object
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::BuildPreClustering( Enlighten::IPrecompute* pPrecompute, 
													 std::vector< Enlighten::IPrecompPackedSystem* >& neighbours, 
													 TriEnlightenProgressBar& prog )
{
	CCP_LOG_CH( g_enlightenBuildChannel, "Creating System Pre Clustering" );

	// This should not be able to happen, because this system should be its own neighbour
	CCP_ASSERT( !neighbours.empty() );
	CCP_ASSERT( m_packedSystem );

	Enlighten::IPrecompSystemPreClustering* systemPreClustering = NULL;
	pPrecompute->CreatePreClustering( m_packedSystem.GetPtr(), &neighbours[0], (Geo::s32)neighbours.size(), &prog, systemPreClustering );
	if( systemPreClustering == NULL )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Failed to build system pre-clustering for the system.");
		CCP_LOGERR_CH( g_enlightenBuildChannel, Tr2Renderer::GetEnlightenErrorBuffer() );
		Enlighten::ClearErrorBuffer();
	}
	m_systemPreClustering.Delete();
	m_systemPreClustering = Geo::GeoAutoReleasePtr<Enlighten::IPrecompSystemPreClustering>( systemPreClustering );
}

// -------------------------------------------------------------
// Description:
//   Creates clustering for the system. This is a third step in
//   building Enlighten.
// Arguments:
//   pPrecompute - pointer to Enlighten precompute object
//   neighbours - list of neighbour Enlighten systems (including this one)
//   prog - progress log object
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::BuildClustering( Enlighten::IPrecompute* pPrecompute, 
												  std::vector< Enlighten::IPrecompSystemPreClustering* >& neighbours, 
												  TriEnlightenProgressBar& prog )
{
	CCP_LOG_CH( g_enlightenBuildChannel, "Creating System Clustering" );

	// This should not be able to happen, because this system should be its own neighbour
	CCP_ASSERT( !neighbours.empty() );
	CCP_ASSERT( m_systemPreClustering );

	Enlighten::IPrecompSystemClustering* systemClustering = NULL;
	pPrecompute->CreateClustering( m_systemPreClustering.GetPtr(), &neighbours[0], (Geo::s32)neighbours.size(), &prog, systemClustering );
	if( systemClustering == NULL )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Failed to build system clustering for the system.");
		CCP_LOGERR_CH( g_enlightenBuildChannel, Tr2Renderer::GetEnlightenErrorBuffer() );
		Enlighten::ClearErrorBuffer();
	}
	m_systemClustering.Delete();
	m_systemClustering = Geo::GeoAutoReleasePtr<Enlighten::IPrecompSystemClustering>( systemClustering );
}

// -------------------------------------------------------------
// Description:
//   Compiles radiosity solution for the system. This is the last step in
//   building Enlighten.
// Arguments:
//   pPrecompute - pointer to Enlighten precompute object
//   neighbours - list of neighbour Enlighten systems (including this one)
//   systemClusters - list of neighbour clusters (including the cluster of this system)
//   prog - progress log object
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::BuildEnlightenSystem( Enlighten::IPrecompute* pPrecompute, 
													   std::vector< Enlighten::IPrecompPackedSystem* >& neighbours, 
													   std::vector< Enlighten::IPrecompSystemClustering* >& systemClusters, 
													   TriEnlightenProgressBar& prog )
{
	CCP_ASSERT( m_packedSystem );

	ReleaseWorkspace();
	ReleaseRadCore();

	// This phase of the enlighten build process is mainly based around assembling multiple systems and their dependencies
	typedef std::vector< Enlighten::IPrecompPackedSystem* > packedSystemVector;

	const Geo::GeoGuid& thisSystemGuid = m_packedSystem->GetId();
	CCP_LOG_CH( g_enlightenBuildChannel, "Building System (%u,%u)", thisSystemGuid.High(), thisSystemGuid.Low() );
	for( packedSystemVector::iterator i = neighbours.begin(); i != neighbours.end(); ++i )
	{
		const Geo::GeoGuid& guid = (*i)->GetId();
		CCP_LOG_CH( g_enlightenBuildChannel, "    Using System: (%u,%u)", guid.High(), guid.Low() );
	}

	CCP_LOG_CH( g_enlightenBuildChannel, "Creating Dusters" );
	Enlighten::IPrecompSystemDuster* systemDuster = NULL;
	pPrecompute->CompileDusters( m_systemClustering.GetPtr(), Geo::GEO_PLATFORM_WINDOWS, &prog, systemDuster );
	if( systemDuster == NULL )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Failed to compile dusters for the system.");
		CCP_LOGERR_CH( g_enlightenBuildChannel, Tr2Renderer::GetEnlightenErrorBuffer() );
		Enlighten::ClearErrorBuffer();
	}
	CCP_ASSERT( systemDuster != NULL );
	ON_BLOCK_EXIT( &Enlighten::IPrecompSystemDuster::Release, systemDuster );// Can't use CComPtr
	
	CCP_LOG_CH( g_enlightenBuildChannel, "Creating Light Transport" );
	// This step onwards is all that really needs doing when you've changed a system next door
	Enlighten::IPrecompSystemLightTransport* lightTransport = NULL;
	pPrecompute->CreateLightTransport( m_packedSystem.GetPtr(), &neighbours[0], (Geo::s32)neighbours.size(), &systemClusters[0], (Geo::s32)systemClusters.size(), &prog, lightTransport );
	if( lightTransport == NULL )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Failed to create light transport for the system.");
		CCP_LOGERR_CH( g_enlightenBuildChannel, Tr2Renderer::GetEnlightenErrorBuffer() );
		Enlighten::ClearErrorBuffer();
	}
	CCP_ASSERT( lightTransport != NULL );
	ON_BLOCK_EXIT( &Enlighten::IPrecompSystemLightTransport::Release, lightTransport );// Can't use CComPtr

	CCP_LOG_CH( g_enlightenBuildChannel, "Compiling Radiosity");
	// Do the actual build
	Enlighten::IPrecompSystemRadiosity* systemRadiosity = NULL;
	pPrecompute->CompileRadiosity(lightTransport, Geo::GEO_PLATFORM_WINDOWS, &prog, systemRadiosity );
	if( systemRadiosity == NULL )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Failed to compile radiosity for the system.");
		CCP_LOGERR_CH( g_enlightenBuildChannel, Tr2Renderer::GetEnlightenErrorBuffer() );
		Enlighten::ClearErrorBuffer();
	}
	CCP_ASSERT( systemRadiosity != NULL );
	ON_BLOCK_EXIT( &Enlighten::IPrecompSystemRadiosity::Release, systemRadiosity );// Can't use CComPtr

	// Copy RadSystemCore
	m_radSystem = GEO_NEW( Enlighten::RadSystemCore );
	*m_radSystem = *systemRadiosity->GetRadSystemCore();
	m_radSystem->m_RadiosityPrecomp.m_DataStart = GEO_ALIGNED_MALLOC( m_radSystem->m_RadiosityPrecomp.m_Length, 16 );
	memcpy( m_radSystem->m_RadiosityPrecomp.m_DataStart, 
			systemRadiosity->GetRadSystemCore()->m_RadiosityPrecomp.m_DataStart, 
			m_radSystem->m_RadiosityPrecomp.m_Length );
	const Enlighten::RadSysDusterGeneric* dusters = systemDuster->GetRadSysDusterGeneric();

	pPrecompute->CompileInputWorkspace( dusters, Geo::GEO_PLATFORM_WINDOWS, &prog, m_inputWorkspace );
	CCP_STATS_ADD( triEnlightenObjectMemory, (int)Enlighten::GetInputWorkspaceSize( m_inputWorkspace ) );
	pPrecompute->CompileMaterialData( dusters, Geo::GEO_PLATFORM_WINDOWS, &prog, m_albedoWorkspaceData );
	CCP_STATS_ADD( triEnlightenObjectMemory, (int)Enlighten::GetClusterAlbedoWorkspaceMaterialDataSize( m_albedoWorkspaceData ) );

	CreateEnlightenWorkspaces();

	CCP_LOG_CH( g_enlightenBuildChannel, "Done creating Radiosity");

	m_radBuiltInternally = true;
}

// -------------------------------------------------------------
// Description:
//   Deletes temporary data used during Enlighten precompute.
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::DeletePrecompData()
{
	m_packedSystem.Delete();
	m_systemPreClustering.Delete();
	m_systemClustering.Delete();
}


// -------------------------------------------------------------
// Description:
//   Grabs chart mapping data.
// Return Value:
//   Whether the output parameters have been written to.
// -------------------------------------------------------------
bool Tr2InteriorEnlightenSystemImpl::CaptureChartMapping()
{
	CCP_DELETE[] m_debugChartMap;
	m_debugChartMap = 0;
	m_debugChartMapHeight = m_debugChartMapWidth = 0;

	if( !m_packedSystem.GetPtr() )
	{
		return false;
	}
	const Enlighten::IMeshSimpOutput *mso = m_packedSystem.GetPtr()->GetMeshSimpOutput();
	if( !mso )
	{
		return false;
	}

	const int h = mso->GetOutputAtlasHeight();
	const int w = mso->GetOutputAtlasWidth();
	m_debugChartMapWidth = w;
	m_debugChartMapHeight = h;
	m_debugChartMap = CCP_NEW( "Tr2InteriorEnlightenSystemImpl/m_debugChartMap" ) int[w * h];
	for( int y = 0; y < h; ++y )
	{
		for( int x = 0; x < w; ++x )
		{
			m_debugChartMap[ y * w + x ] = mso->GetOutputAtlasChart(x,y);
		}
	}
	return true;
}
#endif

int Tr2InteriorEnlightenSystemImpl::GetChartIndex(int x, int y) const
{	
	// Invalid data
	if( m_debugChartMapWidth <= 0 || m_debugChartMapHeight <= 0 || !m_debugChartMap )
	{
		return -1;
	}

	// Invalid texel
	if( x < 0 || x >= m_debugChartMapWidth || y < 0 || y >= m_debugChartMapHeight )
	{
		return -1;
	}

	return m_debugChartMap[ y * m_debugChartMapWidth + x ];
}

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// -------------------------------------------------------------
// Description:
//   Return packed system or NULL if the system was not built/loaded. 
// Return Value:
//   Packed system
// -------------------------------------------------------------
Enlighten::IPrecompPackedSystem* Tr2InteriorEnlightenSystemImpl::GetPackedSystem() const
{
	return m_packedSystem.GetPtr();
}

// -------------------------------------------------------------
// Description:
//   Return system pre-clustering or NULL if the system was not built. 
// Return Value:
//   System clustering
// -------------------------------------------------------------
Enlighten::IPrecompSystemPreClustering* Tr2InteriorEnlightenSystemImpl::GetSystemPreClustering() const
{
	return m_systemPreClustering.GetPtr();
}

// -------------------------------------------------------------
// Description:
//   Return system clustering or NULL if the system was not built/loaded. 
// Return Value:
//   System clustering
// -------------------------------------------------------------
Enlighten::IPrecompSystemClustering* Tr2InteriorEnlightenSystemImpl::GetSystemClustering() const
{
	return m_systemClustering.GetPtr();
}
#endif

// -------------------------------------------------------------
// Description:
//   Return Enlighten input workspace or NULL if the system was not built/loaded. 
// Return Value:
//   Enlighten input workspace
// -------------------------------------------------------------
Enlighten::InputWorkspace* Tr2InteriorEnlightenSystemImpl::GetEnlightenWorkspace() const
{
	return m_inputWorkspace;
}

// -------------------------------------------------------------
// Description:
//   Return albedo workspace or NULL if the system was not built/loaded. 
// Return Value:
//   Albedo workspace
// -------------------------------------------------------------
Enlighten::ClusterAlbedoWorkspace* Tr2InteriorEnlightenSystemImpl::GetAlbedoWorkspace() const
{
	return m_albedoWorkspace;
}

// -------------------------------------------------------------
// Description:
//   Return albedo workspace data associated with system's 
//   albedo workspace or NULL if the system was not built/loaded. 
// Return Value:
//   Albedo workspace data
// -------------------------------------------------------------
Enlighten::ClusterAlbedoWorkspaceMaterialData* Tr2InteriorEnlightenSystemImpl::GetAlbedoWorkspaceData() const
{
	return m_albedoWorkspaceData;
}

// -------------------------------------------------------------
// Description:
//   Return Enlighten core precomputed data or NULL if the system was not built/loaded. 
// Return Value:
//   Enlighten core precomputed data
// -------------------------------------------------------------
const Enlighten::RadSystemCore* Tr2InteriorEnlightenSystemImpl::GetRadSystem() const
{
	return m_radSystem;
}

// -------------------------------------------------------------
// Description:
//   Return current Enlighten input lighting buffer already filled with 
//   lighting data (and ready to be solved).
// Return Value:
//   Current Enlighten input lighting buffer
// -------------------------------------------------------------
Enlighten::InputLightingBuffer* Tr2InteriorEnlightenSystemImpl::GetCurrentInputLightingBuffer() const
{
	return m_inputLightingCurrentFrame;
}

// -------------------------------------------------------------
// Description:
//   Return next Enlighten input lighting buffer to be filled with 
//   lighting data.
// Return Value:
//   Next Enlighten input lighting buffer
// -------------------------------------------------------------
Enlighten::InputLightingBuffer* Tr2InteriorEnlightenSystemImpl::GetNextInputLightingBuffer() const
{
	return m_inputLightingNextFrame;
}

// -------------------------------------------------------------
// Description:
//   Returns a block of memory used by Enlighten to store 
//   bounce data.
// Return Value:
//   Bounce data memory block
// -------------------------------------------------------------
void* Tr2InteriorEnlightenSystemImpl::GetBounceData() const
{
	return m_bounceData;
}

// -------------------------------------------------------------
// Description:
//   Returns Enlighten albedo texture (A8B8G8R8 format)
// Return Value:
//   Enlighten albedo texture
// -------------------------------------------------------------
Geo::u32* Tr2InteriorEnlightenSystemImpl::GetAlbedoTexture() const
{
	return m_albedoTexture;
}

// -------------------------------------------------------------
// Description:
//   Returns Enlighten emissive texture (A8B8G8R8 format)
// Return Value:
//   Enlighten emissive texture
// -------------------------------------------------------------
Geo::u32* Tr2InteriorEnlightenSystemImpl::GetEmissiveTexture() const
{
	return m_emissiveTexture;
}

// -------------------------------------------------------------
// Description:
//   Returns Enlighten albedo/emissive texture size
// Arguments:
//   width - (output) width of the texture
//   height - (output) height of the texture
// Return Value:
//   true If the resulting dimensions are valid (the system is
//        built/loaded)
//   false Otherwise
// -------------------------------------------------------------
bool Tr2InteriorEnlightenSystemImpl::GetAlbedoTextureSize( int& width, int& height ) const
{
	if( m_radSystem == NULL )
	{
		return false;
	}

	int w, h;
	Enlighten::GetTextureAlbedoSize( m_radSystem, w, h );
	width = static_cast<unsigned int>( w );
	height = static_cast<unsigned int>( h );
	return true;
}

// -------------------------------------------------------------
// Description:
//   Tells Enlighten that input data generation for input/albedo 
//   workspaces is done and they are ready to be solved. Swaps 
//   current and next input lighting buffers.
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::EndInputWorkspace()
{
	Enlighten::EndClusterAlbedoWorkspace( GetAlbedoWorkspace(), GetAlbedoWorkspaceData() );

	Enlighten::InputLightingBuffer* temp = m_inputLightingCurrentFrame;
	m_inputLightingCurrentFrame = m_inputLightingNextFrame;
	m_inputLightingNextFrame = temp;
}

// -------------------------------------------------------------
// Description:
//   Return input lighting buffer list. Creates one if needed. 
// Return Value:
//   Input workspace list
// -------------------------------------------------------------
const Enlighten::InputLightingBuffer** Tr2InteriorEnlightenSystemImpl::GetInputLightingList()
{
	CCP_ASSERT( m_radSystem != NULL );

	if( !m_inputLightingList )
	{
		m_inputLightingList = GEO_NEW_ARRAY( const Enlighten::InputLightingBuffer*, Enlighten::GetInputWorkspaceListLength( m_radSystem ) );
	}
	return m_inputLightingList;
}

// -------------------------------------------------------------
// Description:
//   Releases all workspace resources.
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::ReleaseWorkspace()
{
	m_dusterCache.Clear();

	if( m_albedoWorkspace )
	{
		CCP_STATS_ADD( triEnlightenObjectMemory, -(int)Enlighten::CalcClusterAlbedoWorkspaceMemory( m_inputWorkspace ) );
		GEO_ALIGNED_FREE( m_albedoWorkspace );
		m_albedoWorkspace = NULL;
	}

	if( m_inputLightingList )
	{
		GEO_DELETE_ARRAY( const Enlighten::InputLightingBuffer*, m_inputLightingList );
		m_inputLightingList = NULL;
	}

	if( m_inputLightingCurrentFrame )
	{
		unsigned int inputLightingCacheSize = Enlighten::CalcInputLightingBufferSize( m_inputWorkspace );
		CCP_STATS_ADD( triEnlightenObjectMemory, -static_cast<int>( inputLightingCacheSize ) );
		Enlighten::DeleteInputLightingBuffer( m_inputLightingCurrentFrame );
		m_inputLightingCurrentFrame = NULL;
	}

	if( m_bounceData )
	{
		CCP_STATS_ADD( triEnlightenObjectMemory, -static_cast<int>( Enlighten::CalcRequiredBounceOutputSize( m_radSystem ) ) );
		GEO_ALIGNED_FREE( m_bounceData );
	}

	if( m_albedoTexture )
	{
		int width, height;
		Enlighten::GetTextureAlbedoSize( m_radSystem, width, height );
		CCP_STATS_ADD( triEnlightenObjectMemory, -width * height * sizeof( Geo::u32 ) );
		GEO_ALIGNED_FREE( m_albedoTexture );
	}

	if( m_emissiveTexture )
	{
		int width, height;
		Enlighten::GetTextureAlbedoSize( m_radSystem, width, height );
		CCP_STATS_ADD( triEnlightenObjectMemory, -width * height * sizeof( Geo::u32 ) );
		GEO_ALIGNED_FREE( m_emissiveTexture );
	}

	if( m_inputLightingNextFrame )
	{
		unsigned int inputLightingCacheSize = Enlighten::CalcInputLightingBufferSize( m_inputWorkspace );
		CCP_STATS_ADD( triEnlightenObjectMemory, -static_cast<int>( inputLightingCacheSize ) );
		Enlighten::DeleteInputLightingBuffer( m_inputLightingNextFrame );
		m_inputLightingNextFrame = NULL;
	}

	if( m_debugChartMap )
	{
		CCP_DELETE[] m_debugChartMap;
		m_debugChartMap = NULL;
		m_debugChartMapHeight = m_debugChartMapWidth = 0;
	}
}

// -------------------------------------------------------------
// Description:
//   Deletes Enlighten core precomputed data and dusters if they
//   were built (not loaded: for that case they are owned be Tr2RadiosityRes).
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::ReleaseRadCore()
{
	if( m_radBuiltInternally )
	{
		if( m_inputWorkspace )
		{
			CCP_STATS_ADD( triEnlightenObjectMemory, -(int)Enlighten::GetInputWorkspaceSize( m_inputWorkspace ) );
			Enlighten::DeleteInputWorkspace( m_inputWorkspace );
		}

		if( m_albedoWorkspaceData )
		{
			CCP_STATS_ADD( triEnlightenObjectMemory, -(int)Enlighten::GetClusterAlbedoWorkspaceMaterialDataSize( m_albedoWorkspaceData ) );
			Enlighten::DeleteClusterAlbedoWorkspaceMaterialData( m_albedoWorkspaceData );
		}
		
		if( m_radSystem )
		{
			Enlighten::DeleteRadSystemCore( m_radSystem );
		}

		m_radBuiltInternally = false;
		m_previewBuild = false;
	}
	m_inputWorkspace = NULL;
	m_albedoWorkspaceData = NULL;
	m_radSystem = NULL;
}

// -------------------------------------------------------------
// Description:
//   Starts loading .rad file. 
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::SetRadiosityResource()
{
	ReleaseCachedData( m_radSystemResource );
	if( m_radSystemResource )
	{
		m_radSystemResource->RemoveNotifyTarget( this );
		m_radSystemResource.Unlock();
	}

	if( !m_radResPath.empty() )
	{
		BeResMan->GetResource( m_radResPath.c_str(), "", m_radSystemResource );
	}	

	if( m_radSystemResource )
	{
		m_radSystemResource->AddNotifyTarget( this );
	}
}

// -------------------------------------------------------------
// Description:
//   Sets an additional notify target for m_radSystemResource.
// Arguments:
//   notificationTarget - new notify target
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::SetRadNotificationTarget( IBlueAsyncResNotifyTarget* notificationTarget )
{
	m_notificationTarget = notificationTarget;
}

// -------------------------------------------------------------
// Description:
//   Creates Enlighten workspaces and input lighting buffers. 
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystemImpl::CreateEnlightenWorkspaces()
{
	// Workspace for the albedo
	Geo::u32 albedoWorkspaceSize = Enlighten::CalcClusterAlbedoWorkspaceMemory( m_inputWorkspace );
	void* albedoWorkspaceBlock = GEO_ALIGNED_MALLOC( albedoWorkspaceSize, 16 );
	CCP_STATS_ADD( triEnlightenObjectMemory, albedoWorkspaceSize );
	m_albedoWorkspace = Enlighten::CreateClusterAlbedoWorkspace( albedoWorkspaceBlock, m_inputWorkspace );
	CCP_ASSERT( m_albedoWorkspace != NULL );

	// Lighting buffers
	unsigned int inputLightingCacheSize = Enlighten::CalcInputLightingBufferSize( m_inputWorkspace );
	void* dataBlock = GEO_ALIGNED_MALLOC( inputLightingCacheSize, 16 );
	m_inputLightingCurrentFrame = Enlighten::CreateInputLightingBuffer( dataBlock, m_inputWorkspace );
	dataBlock = GEO_ALIGNED_MALLOC( inputLightingCacheSize, 16 );
	m_inputLightingNextFrame = Enlighten::CreateInputLightingBuffer( dataBlock, m_inputWorkspace );
	CCP_STATS_ADD( triEnlightenObjectMemory, static_cast<int>( 2 * inputLightingCacheSize ) );

	int bounceDataSize = Enlighten::CalcRequiredBounceOutputSize( m_radSystem );
	CCP_STATS_ADD( triEnlightenObjectMemory, bounceDataSize );
	m_bounceData = GEO_ALIGNED_MALLOC( bounceDataSize, 16 );
	memset( m_bounceData, 0, bounceDataSize );

	int width, height;
	Enlighten::GetTextureAlbedoSize( m_radSystem, width, height );
	unsigned textureSize = width * height * sizeof( Geo::u32 );
	CCP_STATS_ADD( triEnlightenObjectMemory, 2 * textureSize );
	m_albedoTexture = static_cast<Geo::u32*>( GEO_ALIGNED_MALLOC( textureSize, 16 ) );
	memset( m_albedoTexture, 0, textureSize );
	m_emissiveTexture = static_cast<Geo::u32*>( GEO_ALIGNED_MALLOC( textureSize, 16 ) );
	memset( m_emissiveTexture, 0, textureSize );

	m_dusterCache.Initialize( m_inputWorkspace, m_inputLightingCurrentFrame );
}

Tr2InteriorDusterCache *Tr2InteriorEnlightenSystemImpl::GetDusterCache()
{
	return &m_dusterCache;
}

#endif
