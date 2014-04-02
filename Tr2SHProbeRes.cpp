#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2SHProbeRes.h"
#include "TriEnlightenStream.h"


const Geo::u32 Tr2SHProbeRes::s_versionNumber = 20110118;

Tr2SHProbeRes::Tr2SHProbeRes( IRoot* lockobj ):
	m_data( NULL ),	
	m_dataSize( 0 ),
	m_probeSize( 0 )
{
}

Tr2SHProbeRes::~Tr2SHProbeRes()
{
	ReleaseProbeSet();
}

int Tr2SHProbeRes::GetXResolution()
{
	if ( m_probeSets.size() == 0 )
	{
		return 0;
	}
	return m_probeSets[0].m_xRes;
}

int Tr2SHProbeRes::GetYResolution()
{
	if ( m_probeSets.size() == 0 )
	{
		return 0;
	}
	return m_probeSets[0].m_yRes;
}

int Tr2SHProbeRes::GetZResolution()
{
	if ( m_probeSets.size() == 0 )
	{
		return 0;
	}
	return m_probeSets[0].m_zRes;
}

void Tr2SHProbeRes::ReleaseProbeSet()
{
	for( std::vector<ProbeSet>::iterator it = m_probeSets.begin(); it != m_probeSets.end(); ++it )
	{
		using namespace Enlighten;
		DeleteRadProbeSetCore( it->m_probeSet );
	}	
	m_probeSets.clear();
	m_probeSize = 0;
}

bool Tr2SHProbeRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2SHProbeRes::GetMemoryUsage()
{
	return m_dataSize + m_probeSize;
}

bool Tr2SHProbeRes::DoOpenStream()
{
	BePaths->GetStreamFromPathW( GetPath(), &m_dataStream );

	if( !m_dataStream )
	{
		return false;
	}
	return true;
}

void Tr2SHProbeRes::DoCloseStream()
{
	if( m_dataStream )
	{
		m_dataStream->UnlockData();
		m_data = NULL;
		m_dataSize = 0;
		m_dataStream = nullptr;
	}
}

Enlighten::RadProbeSetCore* ProtectedReadRadProbeSetCore( GeoMemoryStream& stream, const wchar_t* path )
{
	Enlighten::RadProbeSetCore* data = nullptr;
	__try
	{
		data = Enlighten::ReadRadProbeSetCore( stream );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{ 
		CCP_LOGERR( "Files might be corrupt - try running the repair tool" );
		CCP_LOGERR( "Exception caught while reading RadProbeSetCore from file %S", path );
	}

	return data;
}

BlueAsyncRes::LoadingResult Tr2SHProbeRes::DoLoad()
{
	if( !m_dataStream->LockData( &m_data, 0 ) )
	{
		return LR_FAILED;
	}

	m_dataSize = (unsigned int)m_dataStream->GetSize();

	ReleaseProbeSet();

	m_dataSize = (unsigned int)m_dataStream->GetSize();
	GeoMemoryStream stream = GeoMemoryStream( m_dataSize, (BYTE*)m_data );

	Geo::u32 version = 0;
	stream.Read( &version, sizeof( version ), 1 );
	if( version != s_versionNumber )
	{
		CCP_LOGERR( "SH resource %S has incorrect version number %u (current version is %u)", GetPath(), version, s_versionNumber );
		return LR_FAILED;
	}

	m_probeSize = 0;

	ProbeSet probeSet;
	stream.Read( &probeSet.m_xRes, sizeof(int), 1 );
	stream.Read( &probeSet.m_yRes, sizeof(int), 1 );
	stream.Read( &probeSet.m_zRes, sizeof(int), 1 );

	probeSet.m_probeSet = ProtectedReadRadProbeSetCore( stream, m_path.c_str() );
	if( !probeSet.m_probeSet )
	{
		return LR_FAILED;
	}

	m_probeSize += sizeof( probeSet ) + probeSet.m_probeSet->m_ProbeSetPrecomp.m_Length;

	m_probeSets.push_back(probeSet);

	while ( !stream.IsEof())
	{
		Geo::u32 version = 0;
		ProbeSet probeSet;
		stream.Read( &probeSet.m_transform, sizeof(Matrix), 1 );
		stream.Read( &version, sizeof( version ), 1 );
		stream.Read( &probeSet.m_xRes, sizeof(int), 1 );
		stream.Read( &probeSet.m_yRes, sizeof(int), 1 );
		stream.Read( &probeSet.m_zRes, sizeof(int), 1 );

		probeSet.m_probeSet = ProtectedReadRadProbeSetCore( stream, m_path.c_str() );
		if( !probeSet.m_probeSet )
		{
			return LR_FAILED;
		}

		m_probeSets.push_back(probeSet);

		m_probeSize += sizeof( probeSet ) + probeSet.m_probeSet->m_ProbeSetPrecomp.m_Length;
	}

	return LR_SUCCESS;
}

bool Tr2SHProbeRes::DoPrepare()
{
	return true;
}

Enlighten::RadProbeSetCore* Tr2SHProbeRes::GetSHLightProbes()
{
	if( m_probeSets.size() == 0 )
	{
		return NULL;
	}
	return m_probeSets[0].m_probeSet; 
}

size_t Tr2SHProbeRes::GetAdditionalProbeSetCount() const
{
	if( m_probeSets.size() == 0 )
	{
		return 0;
	}
	return m_probeSets.size()-1;
}

bool Tr2SHProbeRes::GetAdditionalProbeSet( size_t index, Enlighten::RadProbeSetCore* &set, int &resolutionX, int &resolutionY, int &resolutionZ, Matrix &transform )
{
	if( m_probeSets.size() <= index + 1 )
	{
		return false;
	}
	set = m_probeSets[index + 1].m_probeSet;
	resolutionX = m_probeSets[index + 1].m_xRes;
	resolutionY = m_probeSets[index + 1].m_yRes;
	resolutionZ = m_probeSets[index + 1].m_zRes;
	transform = m_probeSets[index + 1].m_transform;
	return true;
}

bool Tr2SHProbeRes::WriteSHProbeSetToDisk( const Enlighten::RadProbeSetCore* probeSet, Geo::IGeoStream& stream, int xCount, int yCount, int zCount )
{ 
	stream.Write( &s_versionNumber, sizeof( s_versionNumber ), 1 );
	stream.Write( &xCount, sizeof(int), 1 );
	stream.Write( &yCount, sizeof(int), 1 );
	stream.Write( &zCount, sizeof(int), 1 );

	return Enlighten::WriteRadProbeSetCore( probeSet, stream, Geo::GEO_PLATFORM_WINDOWS );
}

#endif
