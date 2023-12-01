////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2TexturePipelineStepLimitSize.h"

Tr2TexturePipelineStepLimitSize::Tr2TexturePipelineStepLimitSize( IRoot* )
	:m_maxWidth( 0 ),
	m_maxHeight( 0 )
{
}

bool Tr2TexturePipelineStepLimitSize::Execute( ImageIO::HostBitmap& bitmap, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>&, const Tr2TexturePipelineParams& params ) const
{
	auto maxWidth = m_maxWidth;
	if( params.maxWidth )
	{
		if( maxWidth == 0 || maxWidth > params.maxWidth )
		{
			maxWidth = params.maxWidth;
		}
	}
	auto maxHeight = m_maxHeight;
	if( params.maxHeight )
	{
		if( maxHeight == 0 || maxHeight > params.maxHeight )
		{
			maxHeight = params.maxHeight;
		}
	}
	return LimitSize( bitmap, maxWidth, maxHeight );
}


bool Tr2TexturePipelineStepLimitSize::LimitSize( ImageIO::HostBitmap& bitmap, uint32_t maxWidth, uint32_t maxHeight )
{
	if( maxWidth == 0 && maxHeight == 0 )
	{
		return true;
	}

	if( !bitmap.IsValid() )
	{
		CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepLimitSize: invalid input bitmap" );
		return false;
	}

	uint32_t mip = 0;
	uint32_t width = bitmap.GetWidth();
	uint32_t height = bitmap.GetHeight();
	if( maxWidth > 0 )
	{
		while( width > maxWidth )
		{
			width /= 2;
			height /= 2;
			mip++;
		}
	}
	if( maxHeight > 0 )
	{
		while( height > maxHeight )
		{
			width /= 2;
			height /= 2;
			mip++;
		}
	}
	if( mip == 0 )
	{
		return true;
	}

	auto mipCount = bitmap.GetTrueMipCount() > mip + 1 ? bitmap.GetTrueMipCount() - ( mip + 1 ) : 1;
	if( mip + 1 > bitmap.GetTrueMipCount() )
	{
		bitmap.GenerateMipMaps();
	}

	ImageIO::HostBitmap result;
	if( !result.Create( bitmap.GetMipWidth( mip ), bitmap.GetMipHeight( mip ), mipCount, bitmap.GetFormat() ) )
	{
		CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepLimitSize: could not create resulting bitmap" );
		return false;
	}
	for( uint32_t m = mip; m < mipCount; ++m )
	{
		memcpy( result.GetMipRawData( m - mip ), bitmap.GetMipRawData( m ), bitmap.GetMipSize( m ) );
	}
	std::swap( bitmap, result );
	return true;
}