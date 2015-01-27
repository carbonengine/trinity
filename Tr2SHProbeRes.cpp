#include "StdAfx.h"

#include "Tr2SHProbeRes.h"


const uint32_t Tr2SHProbeRes::s_versionNumber = 2;

Tr2SHProbeRes::Tr2SHProbeRes( IRoot* lockobj )
{
}

bool Tr2SHProbeRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2SHProbeRes::GetMemoryUsage()
{
	return m_data.size();
}

BlueAsyncRes::LoadingResult Tr2SHProbeRes::DoLoad()
{
	m_probeSets.clear();

	void* data;
	if( !m_dataStream->LockData( &data, 0 ) )
	{
		return LR_FAILED;
	}

	auto dataSize = m_dataStream->GetSize();

	if( dataSize < 2 * sizeof( uint32_t ) || static_cast<uint32_t*>( data )[0] != s_versionNumber )
	{
		return LR_FAILED;
	}

	m_data.resize( "Tr2SHProbeRes::m_data", dataSize );
	memcpy( m_data.get(), data, dataSize );

	uint32_t count = static_cast<uint32_t*>( data )[1];
	auto* setData = m_data.get() + 2 * sizeof( uint32_t );
	for( uint32_t i = 0; i < count; ++i )
	{
		ProbeSet* set = reinterpret_cast<ProbeSet*>( setData );
		m_probeSets.push_back( set );
		setData += sizeof( ProbeSet ) + ( set->m_xRes * set->m_yRes * set->m_zRes - 1 ) * sizeof( Matrix );

	}

	return LR_SUCCESS;
}

bool Tr2SHProbeRes::DoPrepare()
{
	return true;
}

size_t Tr2SHProbeRes::GetProbeSetCount() const
{
	return m_probeSets.size();
}

bool Tr2SHProbeRes::GetProbeSet( size_t index, int &resolutionX, int &resolutionY, int &resolutionZ, Matrix &transform, const Matrix*& shData )
{
	if( m_probeSets.size() <= index )
	{
		return false;
	}
	auto set = m_probeSets[index];
	resolutionX = set->m_xRes;
	resolutionY = set->m_yRes;
	resolutionZ = set->m_zRes;
	transform = set->m_transform;
	shData = set->m_shData;
	return true;
}
