#include "StdAfx.h"
#include "Tr2TextureReference.h"
#include "Tr2HostBitmap.h"

Tr2TextureReference::Tr2TextureReference( IRoot* )
{
}

Tr2TextureAL* Tr2TextureReference::GetTexture()
{
	return &m_texture;
}

BeResultChoice<BlueStdResult, ImageIO::Result> Tr2TextureReference::Save( const std::wstring& resPath )
{
	if( !m_texture.IsValid() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Referenced texture is invalid" );
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	const void* src;
	uint32_t srcPitch;
	if( FAILED( m_texture.MapForReading( Tr2TextureSubresource(), src, srcPitch, renderContext ) ) )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "Failed to map referenced texture" );
	}

	ImageIO::HostBitmap bmp;
	bmp.CreateFromBitmapDimensions( m_texture.GetDesc() );

	uint32_t dstPitch = bmp.GetPitch();
	auto srcRow = static_cast<const uint8_t*>( src );
	auto dstRow = reinterpret_cast<uint8_t*>( bmp.GetRawData() );
	auto rowSize = std::min( dstPitch, srcPitch );
	for( uint32_t k = 0; k < std::max( m_texture.GetDesc().GetDepth(), 1u ); ++k )
	{
		for( uint32_t j = 0; j < m_texture.GetDesc().GetHeight(); ++j )
		{
			memcpy( dstRow, srcRow, rowSize );
			srcRow += srcPitch;
			dstRow += dstPitch;
		}
	}
	m_texture.UnmapForReading( renderContext );

	auto path = BePaths->ResolvePathForWritingW( resPath );
	Be::Clsid resFileClsid( "blue", "ResFile" );
	IResFilePtr stream( resFileClsid );
	if( !( stream->FileExistsW( path.c_str() ) ? stream->OpenW( path.c_str(), false ) : stream->CreateW( path.c_str() ) ) )
	{
		return BlueStdResult( BLUE_STD_RESULT_OS_ERROR, "Could not open the destination file" );
	}

	return ImageIO::SaveImage( path.c_str(), bmp, *stream );
}

uint32_t Tr2TextureReference::GetWidth() const
{
	return m_texture.GetDesc().GetWidth();
}

uint32_t Tr2TextureReference::GetHeight() const
{
	return m_texture.GetDesc().GetHeight();
}

uint32_t Tr2TextureReference::GetDepth() const
{
	return m_texture.GetDesc().GetDepth();
}

uint32_t Tr2TextureReference::GetType() const
{
	return m_texture.GetDesc().GetType();
}

uint32_t Tr2TextureReference::GetMipCount() const
{
	return m_texture.GetDesc().GetMipCount();
}

uint32_t Tr2TextureReference::GetArraySize() const
{
	return m_texture.GetDesc().GetArraySize();
}

Tr2RenderContextEnum::PixelFormat Tr2TextureReference::GetFormat() const
{
	return m_texture.GetDesc().GetFormat();
}



Tr2TransientTextureReference::Tr2TransientTextureReference( IRoot* )
	:m_texture( nullptr )
{
}

Tr2TextureAL* Tr2TransientTextureReference::GetTexture()
{
	return m_texture;
}

void Tr2TransientTextureReference::SetTexture( Tr2TextureAL* texture )
{
	m_texture = texture;
}