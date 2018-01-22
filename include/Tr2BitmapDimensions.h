#pragma once
#ifndef Tr2BitmapDimensions_H
#define Tr2BitmapDimensions_H

#include <cstdint>
#include "Tr2PixelFormat.h"
#include "Tr2TextureType.h"

// -------------------------------------------------------------
// Description:
// Helper to keep track of variables that usually go together, 
// and functions to do some repeated math around them (computing
// size of a given miplevel, etc).
// -------------------------------------------------------------
struct Tr2BitmapDimensions
{	
	Tr2BitmapDimensions();

	Tr2BitmapDimensions( uint32_t width,
						 uint32_t height,
						 uint32_t mipCount,
						 Tr2RenderContextEnum::PixelFormat format );

	Tr2BitmapDimensions( Tr2RenderContextEnum::TextureType type,
						 Tr2RenderContextEnum::PixelFormat format,
						 uint32_t width,
						 uint32_t height,
						 uint32_t depth,
						 uint32_t mipCount );

	Tr2BitmapDimensions( Tr2RenderContextEnum::TextureType type,
						 Tr2RenderContextEnum::PixelFormat format,
						 uint32_t width,
						 uint32_t height,
						 uint32_t depth,
						 uint32_t mipCount,
						 uint32_t arraySize );

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetDepth() const;
	Tr2RenderContextEnum::PixelFormat GetFormat() const;
	uint32_t GetMipCount() const;
	uint32_t GetTrueMipCount() const;
	bool IsCompressed() const;
	uint32_t HasMipmap() const;

	uint32_t GetMipWidth( uint32_t level ) const;
	uint32_t GetMipHeight( uint32_t level ) const;
	uint32_t GetMipDepth( uint32_t level ) const;
	uint32_t GetMipPitch( uint32_t level ) const;	
	uint32_t GetMipSize( uint32_t level ) const;

	uint32_t GetArraySize() const;

	// Number of rows to copy in a mip. For non-compressed formats this is the same as GetMipHeight.
	// For compressed formats it's that number / 4.
	uint32_t GetMipNumRows( uint32_t level ) const;

	Tr2RenderContextEnum::TextureType GetType() const;

	bool operator==( const Tr2BitmapDimensions& other ) const;

protected:
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_volumeDepth;
	uint32_t m_mipCount;
	uint32_t m_arraySize;
	Tr2RenderContextEnum::TextureType m_type;
	Tr2RenderContextEnum::PixelFormat m_format;

	void Destroy()
	{
		// m_width	= m_height = m_volumeDepth = m_mipCount = 0;
		m_type = Tr2RenderContextEnum::TEX_TYPE_INVALID;
		m_format = Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN;
	}
};

inline Tr2BitmapDimensions::Tr2BitmapDimensions()
:	m_width( 0 ),
	m_height( 0 ),
	m_volumeDepth( 0 ),
	m_mipCount( 0 ),
	m_arraySize( 1 ),
	m_type( Tr2RenderContextEnum::TEX_TYPE_INVALID ),
	m_format( Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
{
}

inline Tr2BitmapDimensions::Tr2BitmapDimensions( 
	uint32_t width,
	uint32_t height,
	uint32_t mipCount,
	Tr2RenderContextEnum::PixelFormat format )
:	m_width( width ),
	m_height( height ),
	m_volumeDepth( 1 ),
	m_mipCount( mipCount ),
	m_arraySize( 1 ),
	m_type( Tr2RenderContextEnum::TEX_TYPE_2D ),
	m_format( format )
{
}

inline Tr2BitmapDimensions::Tr2BitmapDimensions( 
	Tr2RenderContextEnum::TextureType type,
	Tr2RenderContextEnum::PixelFormat format,
	uint32_t width,
	uint32_t height,
	uint32_t depth,
	uint32_t mipCount )
:	m_width( width ),
	m_height( height ),
	m_volumeDepth( depth ),
	m_mipCount( mipCount ),
	m_arraySize( type == Tr2RenderContextEnum::TEX_TYPE_CUBE ? 6 : 1 ),
	m_type( type ),
	m_format( format )
{
}

inline Tr2BitmapDimensions::Tr2BitmapDimensions( 
	Tr2RenderContextEnum::TextureType type,
	Tr2RenderContextEnum::PixelFormat format,
	uint32_t width,
	uint32_t height,
	uint32_t depth,
	uint32_t mipCount,
	uint32_t arraySize )
:	m_width( width ),
	m_height( height ),
	m_volumeDepth( depth ),
	m_mipCount( mipCount ),
	m_arraySize( arraySize ),
	m_type( type ),
	m_format( format )
{
}

inline uint32_t Tr2BitmapDimensions::GetWidth() const 
{ 
	return m_width; 
}

inline uint32_t Tr2BitmapDimensions::GetHeight() const 
{ 
	return m_height; 
}

inline uint32_t Tr2BitmapDimensions::GetDepth() const 
{ 
	return m_volumeDepth; 
}

inline Tr2RenderContextEnum::PixelFormat Tr2BitmapDimensions::GetFormat() const 
{ 
	return m_format; 
}

inline uint32_t Tr2BitmapDimensions::GetMipCount() const 
{ 
	return m_mipCount; 
}

inline uint32_t Tr2BitmapDimensions::GetTrueMipCount() const
{
	if( m_mipCount > 0 )
	{
		return m_mipCount;
	}
	uint32_t size = std::max( m_width, m_height );
	uint32_t count = 0;
	while( size )
	{
		++count;
		size >>= 1;
	}
	return count;
}

inline bool Tr2BitmapDimensions::IsCompressed() const 
{ 
	return IsCompressedFormat( m_format ); 
}

inline uint32_t Tr2BitmapDimensions::HasMipmap() const 
{ 
	return m_mipCount != 1; 
}

inline uint32_t Tr2BitmapDimensions::GetMipWidth( uint32_t level ) const
{
	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	if( IsCompressed() )
	{
		return std::max( ( ( m_width >> level ) + 3u ) & ~3u, 4u );
	}

	return std::max( m_width >> level, 1u );
}

inline uint32_t Tr2BitmapDimensions::GetMipHeight( uint32_t level ) const
{
	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	if( IsCompressed() )
	{
		return std::max( ( ( m_height >> level ) + 3u ) & ~3u, 4u );
	}

	return std::max( m_height >> level, 1u );
}

inline uint32_t Tr2BitmapDimensions::GetMipDepth( uint32_t level ) const
{
	if( m_type != Tr2RenderContextEnum::TEX_TYPE_3D )
	{
		return 1;
	}

	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	return std::max( m_volumeDepth >> level, 1u );
}

inline uint32_t Tr2BitmapDimensions::GetMipPitch( uint32_t level ) const
{
	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	if( IsCompressed() )
	{
		return GetMipWidth( level ) / 4u * GetBlockByteSize( m_format );
	}

	return GetMipWidth( level ) * GetBytesPerPixel( m_format );
}

inline uint32_t Tr2BitmapDimensions::GetMipSize( uint32_t level ) const
{
	if( IsCompressed() )
	{
		return GetMipWidth( level ) * GetMipHeight( level ) * GetMipDepth( level ) / 16 * GetBlockByteSize( m_format );
	}
	
	return GetMipWidth( level ) * GetMipHeight( level ) * GetMipDepth( level ) * GetBytesPerPixel( m_format );
}

inline uint32_t Tr2BitmapDimensions::GetArraySize() const
{
	return m_arraySize;
}

inline uint32_t Tr2BitmapDimensions::GetMipNumRows( uint32_t level ) const
{ 
	return IsCompressed() ? GetMipHeight( level ) / 4 : GetMipHeight( level ); 
}

inline Tr2RenderContextEnum::TextureType Tr2BitmapDimensions::GetType() const 
{ 
	return m_type; 
}

inline bool Tr2BitmapDimensions::operator==( const Tr2BitmapDimensions& other ) const
{
	return
		m_width == other.m_width &&
		m_height == other.m_height &&
		m_volumeDepth == other.m_volumeDepth &&
		m_mipCount == other.m_mipCount &&
		m_arraySize == other.m_arraySize &&
		m_type == other.m_type &&
		m_format == other.m_format;

}

#endif