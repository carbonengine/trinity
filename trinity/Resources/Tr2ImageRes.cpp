#include "StdAfx.h"
#include "Tr2ImageRes.h"

Tr2ImageRes::Tr2ImageRes( IRoot* lockobj )
{
}

Tr2ImageRes::~Tr2ImageRes()
{
}

bool Tr2ImageRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2ImageRes::GetMemoryUsage()
{
	if( m_bitmap.IsValid() )
	{
		return m_bitmap.GetRawDataSize();
	}
	else
	{
		return 1024;
	}
}

BlueAsyncRes::LoadingResult Tr2ImageRes::DoLoad()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_dataStream )
	{
		return LR_FAILED;
	}

	auto result = ImageIO::ReadImage( *m_dataStream, ImageIO::LoadParameters( m_path.c_str() ), m_bitmap );
	if( !result )
	{
		CCP_LOGWARN( "Tr2ImageRes: error reading '%S' - %s", GetPath(), result.GetErrorMessage().c_str() );
		return LR_FAILED;
	}

	return LR_SUCCESS;
}

bool Tr2ImageRes::DoPrepare()
{
	return true;
}

int Tr2ImageRes::GetWidth() const
{
	return m_bitmap.GetWidth();
}

int Tr2ImageRes::GetHeight() const
{
	return m_bitmap.GetHeight();
}

bool Tr2ImageRes::IsPixelOpaque( int x, int y ) const
{
	// TODO: Support different formats
	if( m_bitmap.GetFormat() != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM )
	{
		CCP_LOGERR( "Tr2ImageRes::IsPixelOpaque currently only supports PIXEL_FORMAT_B8G8R8A8_UNORM" );
		return false;
	}

	if( !m_bitmap.GetRawData() )
	{
		return false;
	}

	if( ( x < 0 ) || ( y < 0 ) || ( x >= int( m_bitmap.GetWidth() ) ) || ( y >= int( m_bitmap.GetHeight() ) ) )
	{
		return false;
	}

	const unsigned char* p = reinterpret_cast<const unsigned char*>( m_bitmap.GetRawData() );

	const int bytesPerPixel = 4; // only works for PIXEL_FORMAT_B8G8R8A8_UNORM, etc.
	p += y * m_bitmap.GetWidth() * bytesPerPixel + x * bytesPerPixel;

	return p[3] > 0x7f;
}

Color Tr2ImageRes::GetPixelColor( int x, int y ) const
{
	if( !m_bitmap.IsValid() )
	{
		return Color( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	// TODO: Support different formats
	Tr2RenderContextEnum::PixelFormat format = m_bitmap.GetFormat();
	if( format != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM && 
		format != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM )
	{
		CCP_LOGERR( "Tr2ImageRes::GetPixelColor currently only supports PIXEL_FORMAT_B8G8R8A8_UNORM or PIXEL_FORMAT_B8G8R8X8_UNORM" );
		return Color( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	if( !m_bitmap.GetRawData() )
	{
		if( format == Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM )
		{
			return Color( 0.0f, 0.0f, 0.0f, 0.0f );
		}
		else
		{
			return Color( 0.0f, 0.0f, 0.0f, 1.0f );
		}
	}

	if( ( x < 0 ) || ( y < 0 ) || ( x >= int( m_bitmap.GetWidth() ) ) || ( y >= int( m_bitmap.GetHeight() ) ) )
	{
		return Color( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	const unsigned char* p = reinterpret_cast<const unsigned char*>( m_bitmap.GetRawData() );

	const int bytesPerPixel = 4; // only works for PIXEL_FORMAT_B8G8R8A8_UNORM, etc.
	p += y * m_bitmap.GetWidth() * bytesPerPixel + x * bytesPerPixel;

	Color color( *reinterpret_cast<const uint32_t*>( p ) );
	if( format == Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM )
	{
		color.a = 1.0f;
	}
	return color;
}

const ImageIO::HostBitmap& Tr2ImageRes::GetBitmap() const
{
	return m_bitmap;
}
