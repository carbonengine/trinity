#include "StdAfx.h"

#if APEX_ENABLED

#include "Tr2ApexRes.h"
#include "Apex.h"

Tr2ApexRes::Tr2ApexRes( IRoot* lockobj ) :
	m_dataSize(0)
	,m_asset( NULL )
{
}

Tr2ApexRes::~Tr2ApexRes()
{
	if( m_asset )
	{
		m_asset->release();
		m_asset = NULL;
	}
}

bool Tr2ApexRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2ApexRes::GetMemoryUsage()
{
	return m_dataSize;
}

bool Tr2ApexRes::DoOpenStream()
{
	CCP_ASSERT( !m_dataStream );
	return BePaths->GetStreamFromPathW( m_path.c_str(), &m_dataStream );
}

void Tr2ApexRes::DoCloseStream()
{
	if( m_dataStream )
	{
		m_dataStream->UnlockData();
		m_dataStream.Unlock();
	}
}

BlueAsyncRes::LoadingResult Tr2ApexRes::DoLoad()
{
	// Disabling async load as we suspect threading issues with Apex.
	// All the work is now done in DoPrepare
	return LR_SUCCESS;
}

bool Tr2ApexRes::DoPrepare()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	bool ret = false; // default return code
	CCP_ASSERT( g_Tr2Apex->GetApexSDK() );
	if( !g_Tr2Apex->GetApexSDK() )
	{
		CCP_LOGERR( "g_apexSDK not set - call trinity.InitializeApex");
		return false;
	}

	if( m_asset )
	{
		m_asset->release();
		m_asset = NULL;
	}

	m_dataSize = (uint32_t)m_dataStream->GetSize();

	void *data = CCP_MALLOC("temp_data",m_dataSize);
	m_dataStream->Read(data,m_dataSize);

	physx::general_PxIOStream2::PxFileBuf *fb = g_Tr2Apex->GetApexSDK()->createMemoryReadStream(data,m_dataSize);

	CW2A name( m_path.c_str() );
	m_apexName = name;
	NxParameterized::Serializer::SerializeType serType = g_Tr2Apex->GetApexSDK()->getSerializeType(*fb);
	NxParameterized::Serializer* serializer = g_Tr2Apex->GetApexSDK()->createSerializer( serType ); // create a binary deserializer

	NxParameterized::Serializer::DeserializedData deserializedData;

	NxParameterized::Serializer::ErrorType serError = serializer->deserialize(*fb, deserializedData );

	if( serError != NxParameterized::ERROR_NONE )
	{
		CCP_LOGERR( "Error %d while deserializing '%s'", serError, m_apexName.c_str() );
	}
	else if( deserializedData.size() == 0 )
	{
		CCP_LOGERR( "'%s' does not contain any asset", m_apexName.c_str() );
	}
	else
	{
		NxParameterized::Interface* data = deserializedData[0];
		{
			if( deserializedData.size() > 1 )
			{
				CCP_LOGWARN( "'%s' contains multiple assets - only using the first one", m_apexName.c_str() );
			}
			physx::apex::NxApexAsset* asset = g_Tr2Apex->GetApexSDK()->createAsset( data, m_apexName.c_str() );
			if( !asset )
			{
				CCP_LOGERR( "'%s' is not a valid apex asset", m_apexName.c_str() );
			}
			else
			{
				ret = true; // success!
				m_asset = static_cast<physx::apex::NxApexAsset*>( asset );
			}
		}
	}
	CCP_FREE(data); // free temporary data used by the stream read
	g_Tr2Apex->GetApexSDK()->releaseMemoryReadStream(*fb);

	return ret;
}

physx::apex::NxApexAsset* Tr2ApexRes::GetAsset() const
{
	return m_asset;
}

#endif

