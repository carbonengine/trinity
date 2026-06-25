// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "Tr2DataTextureManager.h"
#include "Tr2TextureReference.h"
#include "Tr2VariableStore.h"

Tr2DataTextureManager::Tr2DataTextureManager( IRoot* lockobj ) :
	m_textureWidth( 256 ),
	m_textureHeight( 4 ),
	m_blockDataNextIdx( 1 ),
	m_maxBlockCount( 0 ),
	m_maxPixelCount( 0 )
{
	m_dataTexture.CreateInstance();

	GlobalStore().RegisterVariable( "ImpactShieldDataMap", m_dataTexture );

	PrepareResources();
}

Tr2DataTextureManager::~Tr2DataTextureManager()
{
	GlobalStore().RegisterVariable( "ImpactShieldDataMap", static_cast<ITr2TextureProvider*>( nullptr ) );
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
	*m_dataTexture->GetTexture() = Tr2TextureAL();
	m_dataTexture->OnTextureChange().Broadcast();
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
	std::vector<Vector4> t( m_textureWidth * m_textureHeight, Vector4( 0.f, 0.f, 0.f, 0.f ) );
	Tr2SubresourceData init = { &t[0], m_textureWidth * uint32_t( sizeof( Vector4 ) ), m_textureWidth * m_textureHeight * uint32_t( sizeof( Vector4 ) ) };
	if( FAILED( m_dataTexture->GetTexture()->Create( Tr2BitmapDimensions( m_textureWidth, m_textureHeight, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ | Tr2CpuUsage::WRITE, &init, renderContext ) ) )
	{
		m_dataTexture->OnTextureChange().Broadcast();
		return false;
	}
	m_dataTexture->OnTextureChange().Broadcast();

	return true;
}


// --------------------------------------------------------------------------------
// Description:
//   Set global variables
// --------------------------------------------------------------------------------
void Tr2DataTextureManager::SetVariables()
{
	GlobalStore().RegisterVariable( "ImpactShieldDataMap", m_dataTexture );
}

// --------------------------------------------------------------------------------
// Description:
//   Syncronous update
// --------------------------------------------------------------------------------
void Tr2DataTextureManager::Update( EveUpdateContext& updateContext )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	// 0
	m_maxPixelCount = m_maxBlockCount = 0;

	// do not update anything if data is totally empty
	if( m_blockData.empty() )
	{
		return;
	}

	// update data texture and keep track of the pixel offsets!
	void* data = nullptr;
	uint32_t pitch = 0;
	int32_t pixelOffset = 0;
	m_dataTextureOffsets.clear();
	if( SUCCEEDED( m_dataTexture->GetTexture()->MapForWriting( Tr2TextureSubresource( 0 ), data, pitch, renderContext ) ) )
	{
		uint8_t* mem = (uint8_t*)data;

		// encode all the blocks in the texture, go through them by priority
		for( auto it = m_blockPriority.rbegin(); it != m_blockPriority.rend(); ++it )
		{
			auto finder = m_blockData.find( it->second );
			if( finder != m_blockData.end() )
			{
				int32_t blockID = finder->first;
				BlockData* block = &finder->second;

				// check if we still have room for one more block
				if( pixelOffset + block->blockLength + 1 >= m_textureWidth )
				{
					break;
				}

				// save pixeloffset
				m_dataTextureOffsets[blockID] = pixelOffset;

				// header
				for( uint32_t y = 0; y < m_textureHeight; ++y )
				{
					memcpy( &mem[y * pitch], &block->header[y], sizeof( Vector4 ) );
				}

				// data
				mem += sizeof( Vector4 );
				for( uint32_t y = 0; y < m_textureHeight; ++y )
				{
					for( uint32_t x = 0; x < block->blockLength; ++x )
					{
						memcpy( &mem[y * pitch + x * sizeof( Vector4 )], &block->data[x * m_textureHeight + y], sizeof( Vector4 ) );
					}
				}

				// next
				mem += block->blockLength * sizeof( Vector4 );
				pixelOffset += block->blockLength + 1;
			}
		}
		m_dataTexture->GetTexture()->UnmapForWriting( renderContext );
	}

	// keep track of some numbers, just for debugging
	m_maxPixelCount = pixelOffset;
	m_maxBlockCount = (uint32_t)m_blockData.size();

	// ok, the texture and the per-block offsets are done, so we don't need the data blocks anymore!
	m_blockData.clear();
	m_blockPriority.clear();
}

// --------------------------------------------------------------------------------
// Description:
//   Request a block of data hand back an ID for further queries on this block
// Arguments:
//   headerData - pointer to Vec4's memory buffer, as long as the requested row number
//   blockLength - the number of columns in the data texture (NOT including the header!)
//   blockData - the block data, column-wise (column0[n], column1[n], column2[n], ...)
//   priority - the block's priority, the higher the more important
// --------------------------------------------------------------------------------
int32_t Tr2DataTextureManager::RequestBlockData( const Vector4* headerData, uint32_t blockLength, const Vector4* blockData, float priority )
{
	// reject zero priority calls
	if( priority <= 0.f )
	{
		return -1;
	}

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
	m_blockData[m_blockDataNextIdx] = bd;

	// and insert it into the priority map, keeps it sorted!
	m_blockPriority[priority] = m_blockDataNextIdx;

	return m_blockDataNextIdx++;
}

// --------------------------------------------------------------------------------
// Description:
//   Query the actual in-texture offset in pixel for each data block
// Arguments:
//   blockID - the block ID previously returned by the update function
// --------------------------------------------------------------------------------
int32_t Tr2DataTextureManager::GetTextureOffset( int32_t blockID ) const
{
	// find it!
	auto finder = m_dataTextureOffsets.find( blockID );
	if( finder == m_dataTextureOffsets.end() )
	{
		return -1;
	}
	return finder->second;
}
