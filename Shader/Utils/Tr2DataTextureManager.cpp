////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "Tr2DataTextureManager.h"

#include "Tr2VariableStore.h"

Tr2DataTextureManager::Tr2DataTextureManager( IRoot* lockobj ) :
	m_maxDataSize( 512 ),
	m_textureHeight( 4 ),
	m_blockDataNextIdx( 1 )
{
	GlobalStore().RegisterVariable( "ImpactShieldDataMap", &m_dataTexture );

	PrepareResources();
}

Tr2DataTextureManager::~Tr2DataTextureManager()
{
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating resources
// --------------------------------------------------------------------------------
bool Tr2DataTextureManager::Initialize()
{
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Release all device resources here
// --------------------------------------------------------------------------------
void Tr2DataTextureManager::ReleaseResources( TriStorage s )
{
	// get rid of data texture
	m_dataTexture.Destroy();
}

// --------------------------------------------------------------------------------
// Description:
//   Debug/dev helper strings to show up in some tools
// --------------------------------------------------------------------------------
#ifdef TRINITYDEV
void Tr2DataTextureManager::GetDescription( std::string& desc )
{
	desc = std::string( "Tr2DataTextureManager" );
}
#endif

// --------------------------------------------------------------------------------
// Description:
//   Allocate all device resources here
// --------------------------------------------------------------------------------
bool Tr2DataTextureManager::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	// create the data texture here, prefill it with zeros
	std::vector<Vector4> t( m_maxDataSize * m_textureHeight, Vector4( 0.f, 0.f, 0.f, 0.f ) );
	Tr2SubresourceData init = { &t[0], m_maxDataSize * uint32_t(sizeof(Vector4)), m_maxDataSize * m_textureHeight * uint32_t(sizeof(Vector4)) };
	if( FAILED( m_dataTexture.Create2D( m_maxDataSize, m_textureHeight, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT, Tr2RenderContextEnum::USAGE_CPU_WRITE, &init, renderContext ) ) )
	{
		return false;
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Syncronous update
// --------------------------------------------------------------------------------
void Tr2DataTextureManager::Update( EveUpdateContext& updateContext )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	// do not update anything if data is totally empty
	if( m_blockData.empty() )
	{
		return;
	}

	// update data texture
	void* data = nullptr;
	uint32_t pitch = 0;
	if( SUCCEEDED( m_dataTexture.Lock( 0, data, pitch, Tr2RenderContextEnum::LOCK_WRITEONLY, renderContext ) ) )
	{
		uint8_t* mem = (uint8_t*)data;

		// encode all the blocks in the texture
		for( auto it = m_blockData.begin(); it != m_blockData.end(); ++it )
		{
			BlockData* block = &it->second;

			// header
			for( uint32_t y = 0; y < m_textureHeight; ++y )
			{
				memcpy( &mem[ y * pitch ], &block->header[ y ], sizeof( Vector4 ) );
			}

			// data
			mem += sizeof( Vector4 );
			for( uint32_t y = 0; y < m_textureHeight; ++y )
			{
				for( uint32_t x = 0; x < block->blockLength; ++x )
				{
					memcpy( &mem[ y * pitch + x * sizeof( Vector4 )], &block->data[ x * m_textureHeight + y ], sizeof( Vector4 ) );
				}
			}
		}
		m_dataTexture.Unlock( renderContext );
	}

	// ok, the texture and the per-block offsets are done, so we don't need the data blocks anymore!
	m_blockData.clear();

	GlobalStore().RegisterVariable( "ImpactShieldDataMap", &m_dataTexture );
}

// --------------------------------------------------------------------------------
// Description:
//   Request a block of data hand back an ID for further queries on this block
// Arguments:
//   headerData - pointer to Vec4's memory buffer, as long as the requested row number
//   blockLength - the number of columns in the data texture (NOT including the header!)
//   blockData - the block data, column-wise (column0[n], column1[n], column2[n], ...)
// --------------------------------------------------------------------------------
int32_t Tr2DataTextureManager::requestBlockData( const Vector4* headerData, size_t blockLength, const Vector4* blockData )
{
	// set the data
	BlockData bd;
	bd.header.resize( m_textureHeight );
	memcpy( &bd.header[0], headerData, m_textureHeight * sizeof( Vector4 ) );

	bd.blockLength = blockLength;

	if( blockLength > 0 )
	{
		bd.data.resize( blockLength * m_textureHeight );
		memcpy( &bd.data[0], blockData, blockLength * m_textureHeight * sizeof( Vector4 ) );
	}

	// insert it into the map
	m_blockData[ m_blockDataNextIdx ] = bd;
	return m_blockDataNextIdx++;
}






