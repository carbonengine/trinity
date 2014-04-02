#include "StdAfx.h"
#include "Tr2ImageRes.h"

Tr2ImageRes::Tr2ImageRes( IRoot* lockobj ) :
	m_imageHandler( nullptr )
{
}

Tr2ImageRes::~Tr2ImageRes()
{
	CCP_DELETE m_imageHandler;
	m_imageHandler = NULL;
}

bool Tr2ImageRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2ImageRes::GetMemoryUsage()
{
	if( m_imageHandler )
	{
		return m_imageHandler->GetTotalDataSize();
	}
	else
	{
		return 1024;
	}
}

bool Tr2ImageRes::DoOpenStream()
{
	m_reservedMemory = 0;

	// make sure the path string is not empty
	if( !GetPath()[0] )
	{
		return false;
	}

	if( BePaths->GetStreamFromPathW( GetPath(), &m_dataStream ) )
	{
		m_reservedMemory = m_dataStream->GetSize();
		BeResMan->ReserveBackgroundLoadMemory( m_reservedMemory );
		return true;
	}

	return false;
}

void Tr2ImageRes::DoCloseStream()
{
	if( m_dataStream )
	{
		m_dataStream->UnlockData();
		m_dataSize = 0;
		m_dataStream = 0;
	}

	BeResMan->ReleaseBackgroundLoadMemory( m_reservedMemory );
	m_reservedMemory = 0;
}

BlueAsyncRes::LoadingResult Tr2ImageRes::DoLoad()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_dataStream )
	{
		return LR_FAILED;
	}

	m_imageHandler = CreateImageHandler( m_path );
	CCP_ASSERT( m_imageHandler != NULL );

	bool isOK = m_imageHandler->ReadHeader( m_dataStream );
	if(isOK)
	{
		isOK = m_imageHandler->ReadImage( m_dataStream );
	}
	else
	{
		CCP_LOGWARN( "Texture '%S' - couldn't read header", GetPath() );
	}

	return isOK ? LR_SUCCESS : LR_FAILED;
}

bool Tr2ImageRes::DoPrepare()
{
	return true;
}

int Tr2ImageRes::GetWidth() const
{
	if( m_imageHandler )
	{
		return m_imageHandler->GetWidth();
	}

	return 0;
}

int Tr2ImageRes::GetHeight() const
{
	if( m_imageHandler )
	{
		return m_imageHandler->GetHeight();
	}

	return 0;
}

bool Tr2ImageRes::IsPixelOpaque( int x, int y ) const
{
	if( !m_imageHandler )
	{
		return false;
	}

	// TODO: Support different formats
	if( m_imageHandler->GetFormat() != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM )
	{
		CCP_LOGERR( "Tr2ImageRes::IsPixelOpaque currently only supports PIXEL_FORMAT_B8G8R8A8_UNORM" );
		return false;
	}

	if( !m_imageHandler->GetMipBytes( 0, 0 ) )
	{
		return false;
	}

	if( ( x >= int( m_imageHandler->GetWidth() ) ) || ( y >= int( m_imageHandler->GetHeight() ) ) )
	{
		return false;
	}

	unsigned char* p = m_imageHandler->GetMipBytes( 0, 0 );

	const int bytesPerPixel = 4; // only works for PIXEL_FORMAT_B8G8R8A8_UNORM, etc.
	p += y * m_imageHandler->GetWidth() * bytesPerPixel + x * bytesPerPixel;

	return p[3] > 0x7f;
}

Color Tr2ImageRes::GetPixelColor( int x, int y ) const
{
	if( !m_imageHandler )
	{
		return Color( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	// TODO: Support different formats
	Tr2RenderContextEnum::PixelFormat format = m_imageHandler->GetFormat();
	if( format != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM && 
		format != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM )
	{
		CCP_LOGERR( "Tr2ImageRes::GetPixelColor currently only supports PIXEL_FORMAT_B8G8R8A8_UNORM or PIXEL_FORMAT_B8G8R8X8_UNORM" );
		return Color( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	if( !m_imageHandler->GetMipBytes( 0, 0 ) )
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

	if( ( x >= int( m_imageHandler->GetWidth() ) ) || ( y >= int( m_imageHandler->GetHeight() ) ) )
	{
		return Color( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	unsigned char* p = m_imageHandler->GetMipBytes( 0, 0 );

	const int bytesPerPixel = 4; // only works for PIXEL_FORMAT_B8G8R8A8_UNORM, etc.
	p += y * m_imageHandler->GetWidth() * bytesPerPixel + x * bytesPerPixel;

	Color color( *reinterpret_cast<uint32_t*>( p ) );
	if( format == Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM )
	{
		color.a = 1.0f;
	}
	return color;
}
