#include "StdAfx.h"

#include "Tr2ClothingRes.h"

#if APEX_ENABLED

#include "Apex.h"

#include "TriSettingsRegistrar.h"

bool g_clothingResAsyncLoad = false;
TRI_REGISTER_SETTING( "clothingResAsyncLoad", g_clothingResAsyncLoad );

Tr2ClothingRes::Tr2ClothingRes( IRoot* lockobj ) :
	m_dataSize(0)
	,m_asset( NULL )
{
}

Tr2ClothingRes::~Tr2ClothingRes()
{
	if( m_asset )
	{
		if( g_Tr2Apex )
		{
			g_Tr2Apex->OnApexAssetReleased( m_asset );
		}
		m_asset->release();
		m_asset = NULL;
	}
}

bool Tr2ClothingRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2ClothingRes::GetMemoryUsage()
{
	return m_dataSize;
}

bool Tr2ClothingRes::DoOpenStream()
{
	CCP_ASSERT( !m_dataStream );
	return BePaths->GetStreamFromPathW( m_path.c_str(), &m_dataStream );
}

void Tr2ClothingRes::DoCloseStream()
{
	if( m_dataStream )
	{
		m_dataStream->UnlockData();
		m_dataStream = nullptr;
	}
}

BlueAsyncRes::LoadingResult Tr2ClothingRes::InternalLoadFunction()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	BlueAsyncRes::LoadingResult ret = LR_FAILED; // default return code
	if( !g_Tr2Apex )
	{
		return ret;
	}

	CCP_ASSERT( g_Tr2Apex->GetApexSDK() );
	if( !g_Tr2Apex->GetApexSDK() )
	{
		CCP_LOGERR( "g_apexSDK not set - call trinity.InitializeApex");
		return ret;
	}

	if( m_asset )
	{
		m_asset->release();
		m_asset = NULL;
	}

	if( !m_dataStream )
	{
		CCP_LOGERR( "Tr2ClothingRes - no valid data stream");
		return ret;
	}

	m_dataSize = (uint32_t)m_dataStream->GetSize();

	void *data = CCP_MALLOC("temp_data",m_dataSize);
	if( !data )
	{
		CCP_LOGERR( "Tr2ClothingRes - failed to allocate data");
		return ret;
	}
	ON_BLOCK_EXIT( CCPFree, data );

	m_dataStream->Read(data,m_dataSize);

	physx::general_PxIOStream2::PxFileBuf *fb = g_Tr2Apex->GetApexSDK()->createMemoryReadStream(data,m_dataSize);
	if( !fb )
	{
		CCP_LOGERR( "Tr2ClothingRes - no valid data stream");
		return ret;
	}

	CW2A name( m_path.c_str() );
	m_apexName = name;

	NxParameterized::Serializer::SerializeType serType = NxParameterized::Serializer::NST_BINARY;
	NxParameterized::Serializer* serializer = g_Tr2Apex->GetApexSDK()->createSerializer( serType ); // create a binary deserializer
	if( !serializer )
	{
		CCP_LOGERR( "Tr2ClothingRes - createSerializer failed");
		g_Tr2Apex->GetApexSDK()->releaseMemoryReadStream(*fb);
		return ret;
	}
	

	NxParameterized::Serializer::DeserializedData deserializedData;

	NxParameterized::Serializer::ErrorType serError = serializer->deserialize(*fb, deserializedData );
	g_Tr2Apex->GetApexSDK()->releaseMemoryReadStream(*fb);

	if( serError != NxParameterized::ERROR_NONE )
	{
		CCP_LOGERR( "Error %d while deserializing '%s'", serError, m_apexName.c_str() );
	}
	else if( deserializedData.size() == 0 || deserializedData[0] == NULL )
	{
		CCP_LOGERR( "'%s' does not contain any asset", m_apexName.c_str() );
	}
	else
	{
		NxParameterized::Interface* data = deserializedData[0];
		if( strcmp( data->className(), "ClothingAssetParameters") != 0 )
		{
			CCP_LOGWARN( "Expected asset type 'ClothingAssetParameters', found asset of type '%s'", data->className() );
		}
		else
		{
			if( deserializedData.size() > 1 )
			{
				CCP_LOGWARN( "'%s' contains multiple assets - only using the first one", m_apexName.c_str() );
			}

			physx::apex::NxApexAsset* asset = g_Tr2Apex->GetApexSDK()->createAsset( data, m_apexName.c_str() );

			if( !asset )
			{
				CCP_LOGERR( "'%s' is not a valid clothing asset", m_apexName.c_str() );
			}
			else
			{
				ret = LR_SUCCESS;
				m_asset = static_cast<physx::apex::NxClothingAsset*>( asset );

				unsigned int n = m_asset->getNumBones();

				for( unsigned int i = 0; i < n; ++i )
				{
					const char* name = m_asset->getBoneName( i );
					if( name && strlen(name) > 0 )
					{
						m_asset->remapBoneIndex( name, i );
					}
				}
			}
		}
	}
	return ret;
}

BlueAsyncRes::LoadingResult Tr2ClothingRes::DoLoad()
{
	if( g_clothingResAsyncLoad )
	{
		return InternalLoadFunction();
	}
	return LR_SUCCESS;
}

bool Tr2ClothingRes::DoPrepare()
{
	if( !g_clothingResAsyncLoad )
	{
		return InternalLoadFunction() == LR_SUCCESS;
	}
	return true;
}

int Tr2ClothingRes::GetMaximumSimulationBudget() const
{
	if( m_asset )
	{
		//return m_asset->getMaximumSimulationBudget();
	}

	return 0;
}

int Tr2ClothingRes::GetNumGraphicalLodLevels() const
{
	if( m_asset )
	{
		return m_asset->getNumGraphicalLodLevels();
	}

	return 0;
}

int Tr2ClothingRes::GetGraphicalLodValue( int lodLevel ) const
{
	if( m_asset )
	{
		return m_asset->getGraphicalLodValue( lodLevel );
	}

	return 0;
}

float Tr2ClothingRes::GetBiggestMaxDistance() const
{
	if( m_asset )
	{
		return m_asset->getBiggestMaxDistance();
	}

	return 0.0f;
}

int Tr2ClothingRes::GetNumBones() const
{
	if( m_asset )
	{
		return m_asset->getNumBones();
	}

	return 0;
}

physx::apex::NxClothingAsset* Tr2ClothingRes::GetAsset() const
{
	return m_asset;
}

void Tr2ClothingRes::SafeReload()
{
	if( !g_Tr2Apex )
	{
		return;
	}

	ForceSynchronousLoad();		// minimize flickering when sculpting
	if( g_Tr2Apex->IsClothSimInProgress() )
	{
		g_Tr2Apex->ApexDelayReload( this );
	}
	else
	{
		Reload();
	}
}

const unsigned int INVALID_BONE_INDEX = 0xffffffff;

static unsigned int FindBone( physx::apex::NxClothingAsset* asset, const char* name )
{
	if( asset && name )
	{
		unsigned int n = asset->getNumUsedBones();

		for( unsigned int i = 0; i < n; ++i )
		{
			const char* candidate = asset->getBoneName( i );
			if( candidate != NULL && strcmp( name, candidate ) == 0 )
			{
				return i;
			}
		}
	}

	return INVALID_BONE_INDEX;
}

void Tr2ClothingRes::BindToRig( const std::string* boneList, unsigned int numBones, BoneMap_t& boneMap )
{
	if( !m_asset || !boneList )
	{
		return;
	}
	unsigned int usedBones = m_asset->getNumUsedBones();
	boneMap.clear();
	boneMap.resize( usedBones );
//	CCP_LOG("Building BoneBinding from %d input bones to %d used bones.\r\n", numBones, usedBones );
	for( unsigned int i = 0; i < numBones; ++i )
	{
		unsigned int ix = FindBone( m_asset, boneList[i].c_str() );
		if( ix != INVALID_BONE_INDEX )
		{
			boneMap[ix] = i;
//			CCP_LOG("Bound Bone:(%s) from %d to %d\r\n", boneList[i].c_str(), i, ix );
		}
	}
}

std::string Tr2ClothingRes::GetBoneName( unsigned int ix ) const
{
	if( !m_asset )
	{
		return "";
	}

	if( ix >= m_asset->getNumBones() )
	{
		return "";
	}

	return m_asset->getBoneName( ix );
}

#endif
