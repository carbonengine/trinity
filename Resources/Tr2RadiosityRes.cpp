#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2RadiosityRes.h"
#include "TriEnlightenStream.h"
#include "TriGeometryRes.h"

const Geo::u32 Tr2RadiosityRes::s_versionNumber = 20110118;

Tr2RadiosityRes::Tr2RadiosityRes( IRoot* lockobj ) :
	m_data( NULL ),
	m_dataSize( 0 ),
	m_systemSize( 0 ),
	m_radSystem( NULL ),
	m_inputWorkspace( NULL ),
	m_albedoWorkspaceData( NULL ),
	m_debugChartMap( NULL ),
	m_debugChartMapWidth( 0 ),
	m_debugChartMapHeight( 0 )
{
}

Tr2RadiosityRes::~Tr2RadiosityRes()
{
	Enlighten::DeleteRadSystemCore( m_radSystem );
	Enlighten::DeleteInputWorkspace( m_inputWorkspace );
	Enlighten::DeleteClusterAlbedoWorkspaceMaterialData( m_albedoWorkspaceData );
	CCP_DELETE[] m_debugChartMap;
}

bool Tr2RadiosityRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2RadiosityRes::GetMemoryUsage()
{
	return m_systemSize;
}

bool Tr2RadiosityRes::DoOpenStream()
{
	BePaths->GetStreamFromPathW( GetPath(), &m_dataStream );

	if( !m_dataStream )
	{
		return false;
	}

	return true;
}

void Tr2RadiosityRes::DoCloseStream()
{
	if( m_dataStream )
	{
		m_dataStream->UnlockData();
		m_data = NULL;
		m_dataSize = 0;
		m_dataStream = nullptr;
	}
}


Enlighten::RadSystemCore* ProtectedReadRadSystemCore( GeoMemoryStream& stream, const wchar_t* path )
{
	Enlighten::RadSystemCore* radSystem = nullptr;
	__try
	{
		radSystem = Enlighten::ReadRadSystemCore( stream );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{ 
		CCP_LOGERR( "Files might be corrupt - try running the repair tool" );
		CCP_LOGERR( "Exception caught while reading RadSystemCore from file %S", path );
	}

	return radSystem;
}

Enlighten::InputWorkspace* ProtectedReadInputWorkspace( GeoMemoryStream& stream, const wchar_t* path )
{
	Enlighten::InputWorkspace* inputWorkspace = nullptr;
	__try
	{
		inputWorkspace = Enlighten::ReadInputWorkspace( stream );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{ 
		CCP_LOGERR( "Files might be corrupt - try running the repair tool" );
		CCP_LOGERR( "Exception caught while reading InputWorkspace from file %S", path );
	}

	return inputWorkspace;
}

Enlighten::ClusterAlbedoWorkspaceMaterialData* ProtectedReadClusterAlbedoWorkspaceMaterialData( GeoMemoryStream& stream, const wchar_t* path )
{
	Enlighten::ClusterAlbedoWorkspaceMaterialData* albedoWorkspaceData = nullptr;
	__try
	{
		albedoWorkspaceData = Enlighten::ReadClusterAlbedoWorkspaceMaterialData( stream );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{ 
		CCP_LOGERR( "Files might be corrupt - try running the repair tool" );
		CCP_LOGERR( "Exception caught while reading ClusterAlbedoWorkspaceMaterialData from file %S", path );
	}

	return albedoWorkspaceData;
}

BlueAsyncRes::LoadingResult Tr2RadiosityRes::DoLoad()
{
	if( !m_dataStream->LockData( &m_data, 0 ) )
	{
		return LR_FAILED;
	}

	m_dataSize = (unsigned int)m_dataStream->GetSize();


	GeoMemoryStream stream =  GeoMemoryStream( m_dataSize, (uint8_t*)m_data );

	Geo::u32 version = 0;
	stream.Read( &version, sizeof( version ), 1 );
	if( version != s_versionNumber )
	{
		CCP_LOGERR( "Radiosity resource %S has incorrect version number %u (current version is %u)", GetPath(), version, s_versionNumber );
		return LR_FAILED;
	}

	m_systemSize = 0;
	
	Enlighten::DeleteRadSystemCore( m_radSystem );
	m_radSystem = NULL;
	Enlighten::DeleteInputWorkspace( m_inputWorkspace );
	m_inputWorkspace = NULL;
	Enlighten::DeleteClusterAlbedoWorkspaceMaterialData( m_albedoWorkspaceData );
	m_albedoWorkspaceData = NULL;
	CCP_DELETE[] m_debugChartMap;
	m_debugChartMap = NULL;
	m_debugChartMapWidth = 0;
	m_debugChartMapHeight = 0;

	m_radSystem = ProtectedReadRadSystemCore( stream, m_path.c_str() );
	m_inputWorkspace = ProtectedReadInputWorkspace( stream, m_path.c_str() );
	m_albedoWorkspaceData = ProtectedReadClusterAlbedoWorkspaceMaterialData( stream, m_path.c_str() );

	if ( !m_radSystem || !m_inputWorkspace || !m_albedoWorkspaceData )
	{
		Enlighten::DeleteRadSystemCore( m_radSystem );
		m_radSystem = NULL;
		Enlighten::DeleteInputWorkspace( m_inputWorkspace );
		m_inputWorkspace = NULL;
		Enlighten::DeleteClusterAlbedoWorkspaceMaterialData( m_albedoWorkspaceData );
		m_albedoWorkspaceData = NULL;
		return LR_FAILED;
	}

	m_systemSize = sizeof( m_radSystem ) + m_radSystem->m_RadiosityPrecomp.m_Length;
	m_systemSize += Enlighten::GetInputWorkspaceSize( m_inputWorkspace );
	m_systemSize += Enlighten::GetClusterAlbedoWorkspaceMaterialDataSize( m_albedoWorkspaceData );

	unsigned int geometryCount;
	if( !stream.Read( &geometryCount, sizeof(unsigned int), 1 ) )
	{
		return LR_FAILED;
	}

	for( unsigned int i = 0; i < geometryCount; ++i )
	{
		unsigned int geometryHash1, geometryHash2, pathLength;
		stream.Read( &geometryHash1, sizeof(unsigned int), 1 );
		stream.Read( &geometryHash2, sizeof(unsigned int), 1 );
		stream.Read( &pathLength, sizeof(unsigned int), 1 );

		wchar_t path[MAX_PATH];
		stream.Read( &path, sizeof(wchar_t), min(pathLength,(unsigned int)MAX_PATH) );
		
		// Construct the geometry hashes, which will be passes to python to check if the system needs to be rebuilt
		m_geometryHashes.push_back( std::make_pair( path, std::make_pair(geometryHash1,geometryHash2) ) );
	}

	m_systemSize += sizeof( unsigned ) * 3 * geometryCount;

	// Debug chart information is optional and should be last thing in the stream
	if( stream.Read( &m_debugChartMapWidth, sizeof(int), 1 ) == 0 
		|| stream.Read( &m_debugChartMapHeight, sizeof(int), 1 ) == 0 )
	{
		//no debug chart data present
		m_debugChartMapWidth = m_debugChartMapHeight = 0;
	}
	else
	{
		if( m_debugChartMapWidth > 0 && m_debugChartMapHeight > 0 )
		{
			m_debugChartMap = CCP_NEW( "Tr2RadiosityRes/m_debugChartMap" ) int[ m_debugChartMapWidth * m_debugChartMapHeight ];
			if( stream.Read( m_debugChartMap, sizeof(int), m_debugChartMapWidth * m_debugChartMapHeight ) == 0 )
			{
				CCP_LOGWARN( "Debug chart information not present, should be %d x %d bitmap", m_debugChartMapWidth, m_debugChartMapHeight );
				CCP_DELETE[] m_debugChartMap;
				m_debugChartMap = NULL;
				m_debugChartMapWidth = m_debugChartMapHeight = 0;
			}
			m_systemSize += sizeof( int ) * m_debugChartMapWidth * m_debugChartMapHeight;
		}
	}

	return LR_SUCCESS;
}

bool Tr2RadiosityRes::DoPrepare()
{
	return true;
}

Enlighten::RadSystemCore* Tr2RadiosityRes::GetSystemRadiosity()
{ 
	return m_radSystem; 
}

Enlighten::InputWorkspace* Tr2RadiosityRes::GetInputWorkspace() const
{
	return m_inputWorkspace;
}

Enlighten::ClusterAlbedoWorkspaceMaterialData* Tr2RadiosityRes::GetAlbedoWorkspaceData() const
{
	return m_albedoWorkspaceData;
}

// -------------------------------------------------------------------
// Description:
//   Returns chart map array along with its dimensions. The array
//   is allocated and filled with chart map data.
// Arguments:
//   chartMap - (out) Pointer to the newly allocated chart map
//              array containing a copy of the chart map or
//              zero if the chart is not loaded.
//   width - Width of the chart map or 0 if the chart is not loaded
//   height - Height of the chart map or 0 if the chart is not loaded
// -------------------------------------------------------------------
void Tr2RadiosityRes::GetDebugChartMap( int*& chartMap, int& width, int& height ) const
{
	if( m_debugChartMap == NULL )
	{
		chartMap = NULL;
		width = 0;
		height = 0;
	}
	else
	{
		chartMap = CCP_NEW( "Tr2RadiosityRes/m_debugChartMap copy" ) int[m_debugChartMapWidth * m_debugChartMapHeight];
		memcpy( chartMap, m_debugChartMap, sizeof( int ) * m_debugChartMapWidth * m_debugChartMapHeight );
		width = m_debugChartMapWidth;
		height = m_debugChartMapHeight;
	}
}

bool Tr2RadiosityRes::WriteWorkspaceResourceHashesToDisk( const std::string& filename, 
														  Enlighten::RadSystemCore* sysCore,
														  Enlighten::InputWorkspace* inputWorkspace, 
														  Enlighten::ClusterAlbedoWorkspaceMaterialData* albedoWorkspaceData, 
														  const std::vector<const TriGeometryRes*>& geometries,
														  const int chartWidth,
														  const int chartHeight,
														  const int *const chartData )
{ 

	Geo::GeoFileStream stream = Geo::GeoFileStream();
	if ( !stream.Open( CA2W( filename.c_str() ), Geo::GeoFileStream::esmWrite ))
	{
		CCP_LOGERR( "Tr2RadiosityRes::WriteSystemDustersResourceHashesToDisk Could not open %s for writing\n", filename.c_str() );	
	}
	else
	{
		bool success = true;
		// Use the enlighten stream writing functions to write out the base data
		success &= stream.Write( &s_versionNumber, sizeof( Geo::u32 ), 1 ) == 1;
		success &= Enlighten::WriteRadSystemCore( sysCore, stream, Geo::GEO_PLATFORM_WINDOWS );
		success &= Enlighten::WriteInputWorkspace( inputWorkspace, stream, Geo::GEO_PLATFORM_WINDOWS );
		success &= Enlighten::WriteClusterAlbedoWorkspaceMaterialData( albedoWorkspaceData, stream, Geo::GEO_PLATFORM_WINDOWS );

		unsigned int geometryCount = (unsigned int)geometries.size();
		stream.Write( &geometryCount, sizeof(unsigned int), 1 );

		for( unsigned int i = 0; i < geometryCount; ++i )
		{
			const TriGeometryRes* geometry = geometries[i];

			const wchar_t* path = geometry->GetPath();
			// add one to include \0
			unsigned int pathLength = (unsigned int)wcslen( path ) + 1;

			// Geometry hashes are constructed at the time that the geometry is built, 
			// from the date (down to minute resolution), and the guid used to build it
			// We're mainly interested in detecting if the geometry has been rebuilt after the system was
			// All of that logic is handled by python, rather than in C++ though
			unsigned int geometryHash1, geometryHash2;
			geometry->GetEnlightenGeometryHashes( geometryHash1, geometryHash2 );

			stream.Write( &geometryHash1, sizeof(unsigned int), 1 );
			stream.Write( &geometryHash2, sizeof(unsigned int), 1 );
			stream.Write( &pathLength, sizeof(unsigned int), 1 );
			stream.Write( path, sizeof(wchar_t), pathLength );
		}

		// Write out optional chart data, this should be the last thing written 
		//  due to lack of versioning on the resource files.
		stream.Write( &chartWidth, sizeof(int), 1 );
		stream.Write( &chartHeight, sizeof(int), 1 );
		stream.Write( (const void*)chartData, sizeof(int), chartWidth * chartHeight );
		CCP_LOG( "Appended %.02f kb of data to radiosity resource for debug chart mapping", (chartWidth*chartHeight*sizeof(int)) / 1024.f );

		return success;
	}
	return false;
}

#endif
