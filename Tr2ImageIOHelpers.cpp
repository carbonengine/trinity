#include "StdAfx.h"
#include "Tr2ImageIOHelpers.h"

using namespace Tr2RenderContextEnum;

namespace
{

bool CreateCubeTexture(	Tr2ImageHandler& ih, Tr2TextureAL &out, 
											uint32_t &memoryUse, 
											Tr2PrimaryRenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	const unsigned trueMipLevelCount = std::max( ih.GetMipLevelCount(), 1u );
	
	std::vector<Tr2SubresourceData> initData;

	for( unsigned face = 0; face != 6 ; ++face )
	{		
		for( unsigned i = 0; i != trueMipLevelCount; ++i )
		{
			Tr2SubresourceData srd;
			srd.m_sysMem			= ih.GetMipBytes( i, face );			
			srd.m_height			= ih.GetMipLevelHeight( i );
			srd.m_sysMemSlicePitch	= ih.GetMipLevelSize( i );
			srd.m_sysMemPitch		= ih.GetPitch( i );	

			if( !srd.m_sysMem )
			{
				return false;
			}

			initData.push_back( srd );

			memoryUse += ih.GetMipLevelSize( i );
		}
	}

	if( FAILED( out.CreateCube( ih.GetWidth(), ih.GetHeight(), ih.GetMipLevelCount(), ih.GetFormat(), USAGE_IMMUTABLE, &initData[0], renderContext ) ) )
	{
		CCP_LOGWARN( "Tr2ImageHandler::CreateCubeTexture - Failed for %S", ih.GetSourceName() );
		return false;
	}

	return true;
}

bool CreateVolumeTexture( Tr2ImageHandler& ih, Tr2TextureAL &out, 
											uint32_t &memoryUse, 
											Tr2PrimaryRenderContext &renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	BeTimer create;

	const unsigned trueMipLevelCount = std::max( ih.GetMipLevelCount(), 1u );

	std::vector<Tr2SubresourceData> initData;

	//
	// Copy the pixels into the locked D3D surface (one copy per mip)
	//
	for( unsigned i = 0; i != trueMipLevelCount; ++i )
	{
		Tr2SubresourceData srd;

		srd.m_sysMem			= ih.GetMipBytes( i );			
		srd.m_height			= ih.GetMipLevelHeight( i );
		srd.m_sysMemSlicePitch	= ih.GetMipLevelSize( i );
		srd.m_sysMemPitch		= ih.GetPitch( i );	

		if( !srd.m_sysMem )
		{
			return false;
		}

		const unsigned mipDepth = std::max( ih.GetDepth() >> i, 1u );
		srd.m_sysMemSlicePitch	/= mipDepth;

		initData.push_back( srd );

		memoryUse += ih.GetMipLevelSize( i );
	}

	if( FAILED( out.CreateVolume( ih.GetWidth(), ih.GetHeight(), ih.GetDepth(), ih.GetMipLevelCount(), ih.GetFormat(), USAGE_IMMUTABLE, &initData[0], renderContext ) ) )
	{
		CCP_LOGWARN( "Tr2ImageHandler::CreateVolumeTexture - Failed for %S", ih.GetSourceName() );
		return false;
	}

	return true;
}

}

namespace Tr2ImageIOHelpers
{

bool Create2DTexture(	Tr2ImageHandler& ih, Tr2TextureAL &out, 
										uint32_t &memoryUse, 
										Tr2PrimaryRenderContext &renderContext, 
										Tr2RenderContextEnum::BufferUsage usage ) 
{
	CCP_STATS_ZONE( __FUNCTION__ );

	BeTimer create;

	const unsigned trueMipLevelCount = std::max( ih.GetMipLevelCount(), 1u );

	std::vector<Tr2SubresourceData> initData( trueMipLevelCount );

	for( unsigned i = 0; i != trueMipLevelCount; ++i )
	{
		Tr2SubresourceData& srd = initData[i];

		srd.m_sysMem			= ih.GetMipBytes( i );			
		srd.m_height			= ih.GetMipLevelHeight( i );
		srd.m_sysMemSlicePitch	= ih.GetMipLevelSize( i );
		srd.m_sysMemPitch		= ih.GetPitch( i );	

		if( !srd.m_sysMem )
		{
			return false;
		}

		memoryUse += srd.m_sysMemSlicePitch;

		//REPORT_TIME( "COPY: %5.5f sec '%s' - %d\n", t, m_sourceName.c_str(), i );
	}

	if( FAILED( out.Create2D(	ih.GetWidth(), 
								ih.GetHeight(), 
								ih.GetMipLevelCount(), 
								ih.GetFormat(), 
								usage, 
								&initData[0], 
								renderContext ) ) )
	{
		CCP_LOGWARN( "Tr2ImageHandler::Create2DTexture - Failed for %S", ih.GetSourceName() );
		return false;
	}

	return true;
}

bool CreateTexture(	Tr2ImageHandler& ih, Tr2TextureAL &out, 
										uint32_t &memoryUse, 
										Tr2PrimaryRenderContext &renderContext, 
										Tr2RenderContextEnum::BufferUsage usage )
{
	if( ih.IsCubeTexture() )
	{
		return CreateCubeTexture( ih, out, memoryUse, renderContext );
	}
	else if( ih.IsVolumeTexture() )
	{
		return CreateVolumeTexture( ih, out, memoryUse, renderContext );
	}
	else
	{
		return Create2DTexture( ih, out, memoryUse, renderContext, usage );
	}
	

	return true;
}

bool CopyToTexture( Tr2ImageHandler& ih, Tr2TextureAL& texture, unsigned int x, unsigned int y, unsigned int margin, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( texture.GetFormat() != ih.GetFormat() )
	{
		CCP_LOGERR( "Tr2ImageHandler::CopyToTexture - formats don't match" );
		return false;
	}

	if( x >= texture.GetWidth() || y >= texture.GetHeight() )
	{
		CCP_LOGERR( "Tr2ImageHandler::CopyToTexture - out of bounds" );
		return false;
	}

	if( x + ih.GetWidth() > texture.GetWidth() || y + ih.GetHeight() > texture.GetHeight() )
	{
		CCP_LOGERR( "Tr2ImageHandler::CopyToTexture - out of bounds" );
		return false;
	}

	if( !margin )
	{
		const auto result = texture.UpdateSubresource( x, y, x + ih.GetWidth(), y + ih.GetHeight(), ih.GetMipBytes( 0 ), ih.GetPitch( 0 ), renderContext );
		if( FAILED( result ) )
		{
			CCP_LOGERR( "Tr2ImageHandler::CopyToTexture - UpdateSubresource failed [no margin]: %08x", result );
		}
		return SUCCEEDED(result);
	}

	// Can't expect the Hal to support updating a subresource with automatic replication of border pixels, so do this ourselves in a chunk
	// of temporary memory, then send that off to the backend.

	std::vector<unsigned char> pixels;
	unsigned pitch = 0;
	ih.AddMargin( ih.GetFormat(), ih.GetMipBytes( 0 ), ih.GetWidth(), ih.GetHeight(), margin, pixels, pitch );

	const auto result = texture.UpdateSubresource( x, y, x + ih.GetWidth() + 2 * margin, y + ih.GetHeight() + 2 * margin, &pixels[0], pitch, renderContext );
	if( FAILED( result ) )
	{
		CCP_LOGERR( "Tr2ImageHandler::CopyToTexture - UpdateSubresource failed [margin]: %08x", result );
	}
	return SUCCEEDED(result);
}
}
