////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2TexturePipelineStepPack.h"

using namespace Tr2RenderContextEnum;

namespace
{
	uint32_t SwapRedBlue( uint8_t channel )
	{
		switch( channel )
		{
		case 0:
			return 2;
		case 2:
			return 0;
		default:
			return channel;
		}
	}
}


Tr2TexturePackChannel::Tr2TexturePackChannel( IRoot* )
	:m_channel( 0 ),
	m_fill( 0 )
{

}

Tr2TexturePipelineStepPack::Tr2TexturePipelineStepPack( IRoot* lockobj ):
	m_format( PIXEL_FORMAT_B8G8R8A8_UNORM ),
	PARENTLOCK( m_r ),
	PARENTLOCK( m_g ),
	PARENTLOCK( m_b ),
	PARENTLOCK( m_a )
{


}

void Tr2TexturePipelineStepPack::GetResourceDependencies( std::set<std::wstring>& resources ) const
{
	if( !m_r.m_path.empty() )
	{
		resources.insert( m_r.m_path );
	}
	if( !m_g.m_path.empty() )
	{
		resources.insert( m_g.m_path );
	}
	if( !m_b.m_path.empty() )
	{
		resources.insert( m_b.m_path );
	}
	if( !m_a.m_path.empty() )
	{
		resources.insert( m_a.m_path );
	}
};

bool Tr2TexturePipelineStepPack::Execute( ImageIO::HostBitmap& bitmap, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>& inputs, const Tr2TexturePipelineParams& ) const
{
	if( bitmap.IsValid() )
	{
		CCP_LOGWARN_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepLoad: output bitmap is not empty" );
	}
	if( m_format != PIXEL_FORMAT_B8G8R8A8_UNORM && m_format != PIXEL_FORMAT_B8G8R8X8_UNORM && m_format != PIXEL_FORMAT_R8_UNORM )
	{
		CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepPack: only supports b8g8r8a8, b8g8r8 and r8 textures" );
		return false;
	}

	const Tr2TexturePackChannel* channels[] = { &m_b, &m_g, &m_r, &m_a };
	const ImageIO::HostBitmap* channelInputs[4];
	for( uint32_t i = 0; i < 4; ++i )
	{
		if( channels[i]->m_path.empty() )
		{
			channelInputs[i] = nullptr;
		}
		else
		{
			auto found = inputs.find( channels[i]->m_path );
			if( found == inputs.end() || !found->second )
			{
				CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepPack: failed to get input texture %S", channels[i]->m_path.c_str() );
				return false;
			}
			channelInputs[i] = found->second;
		}
	}

	uint32_t width = 4;
	uint32_t height = 4;
	uint32_t mips = 1;
	bool defaultSize = true;

	for( uint32_t i = 0; i < 4; ++i )
	{
		auto channelInput = channelInputs[i];
		if( channelInput )
		{
			if( channelInput->GetArraySize() > 1 )
			{
				CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepPack: no support for texture arrays" );
				return false;
			}
			if( channelInput->GetType() != TEX_TYPE_2D )
			{
				CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepPack: only supports 2D textures" );
				return false;
			}
			if( channelInput->GetFormat() != PIXEL_FORMAT_B8G8R8A8_UNORM && channelInput->GetFormat() != PIXEL_FORMAT_B8G8R8X8_UNORM && channelInput->GetFormat() != PIXEL_FORMAT_R8_UNORM )
			{
				CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepPack: only supports b8g8r8a8, b8g8r8 and r8 textures" );
				return false;
			}

			if( defaultSize )
			{
				width = channelInput->GetWidth();
				height = channelInput->GetHeight();
				mips = channelInput->GetTrueMipCount();
				defaultSize = false;
			}
			else
			{
				if( width != channelInput->GetWidth() || height != channelInput->GetHeight() )
				{
					CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepPack: inconsistent texture size for %S", channels[i]->m_path.c_str() );
					return false;
				}
				mips = std::min( mips, channelInput->GetTrueMipCount() );
			}
		}
	}

	if( !bitmap.Create( width, height, mips, m_format ) )
	{
		CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepPack: failed to create output" );
		return false;
	}

	for( uint32_t mip = 0; mip < mips; ++mip )
	{
		BitmapSource sources[4];
		for( uint32_t i = 0; i < 4; ++i )
		{
			auto& source = sources[i];
			auto channelInput = channelInputs[i];
			auto& channel = *channels[i];
			if( channelInput )
			{
				source.pixelStride = GetBytesPerPixel( channelInput->GetFormat() );
				source.rowStride = channelInput->GetMipPitch( mip );
				source.data = reinterpret_cast<const uint8_t*>( channelInput->GetMipRawData( mip ) ) + std::min( source.pixelStride - 1, SwapRedBlue( channel.m_channel ) );
			}
			else
			{
				source.pixelStride = 0;
				source.rowStride = 0;
				source.data = &channel.m_fill;
			}
		}
		uint32_t sourceCount;
		switch( m_format )
		{
		case PIXEL_FORMAT_R8_UINT:
			sourceCount = 1;
			std::swap( sources[0], sources[2] );
			break;
		case PIXEL_FORMAT_B8G8R8X8_UNORM:
			sourceCount = 3;
			break;
		default:
			sourceCount = 4;
		}
		Pack( bitmap, sources, sourceCount, mip );
	}
	return true;
}

void Tr2TexturePipelineStepPack::Pack( ImageIO::HostBitmap& bitmap, BitmapSource* channels, uint32_t channelCount, uint32_t mip )
{
	auto width = bitmap.GetMipWidth( mip );
	auto height = bitmap.GetMipHeight( mip );
	auto pitch = bitmap.GetMipPitch( mip );

	auto dstRow = bitmap.GetMipRawData( mip );
	const uint8_t* srcRows[4] = { channels[0].data, channels[1].data, channels[2].data, channels[3].data };

	for( uint32_t j = 0; j < height; ++j )
	{
		const uint8_t* src[4] = { srcRows[0], srcRows[1], srcRows[2], srcRows[3] };
		auto dst = dstRow;
		for( uint32_t i = 0; i < width; ++i )
		{
			for( uint32_t c = 0; c < channelCount; ++c )
			{
				*dst++ = *src[c];
				src[c] += channels[c].pixelStride;
			}
		}
		dstRow += pitch;
		srcRows[0] += channels[0].rowStride;
		srcRows[1] += channels[1].rowStride;
		srcRows[2] += channels[2].rowStride;
		srcRows[3] += channels[3].rowStride;
	}
}
