#include "StdAfx.h"
#include "Tr2HalHelperStructures.h"

#include "include/Tr2ShaderAL.h"
#include "include/Tr2ConstantBufferAL.h"
#include "include/Tr2VertexLayoutAL.h"
#include "include/Tr2ShaderProgramAL.h"
#include "include/Tr2ShaderProgramAL.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2TextureSubresource default constructor: construct a subresource range containing
//   the entire resource.
// --------------------------------------------------------------------------------------
Tr2TextureSubresource::Tr2TextureSubresource() :
	m_startFace( 0 ), 
	m_endFace( std::numeric_limits<uint32_t>::max() ), 
	m_startMipLevel( 0 ), 
	m_endMipLevel( 0xffffffff ), 
	m_box{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
{
}

Tr2TextureSubresource::Tr2TextureSubresource( uint32_t mipLevel )
	:m_startFace( 0 ),
	m_endFace( 1 ),
	m_startMipLevel( mipLevel ),
	m_endMipLevel( mipLevel + 1 ),
	m_box{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2TextureSubresource default constructor: construct a subresource range containing
//   the a single mip level for a single cubemap face / array slice.
// --------------------------------------------------------------------------------------
Tr2TextureSubresource::Tr2TextureSubresource( uint32_t face, uint32_t mipLevel )
	:m_startFace( face ),
	m_endFace( face + 1 ),
	m_startMipLevel( mipLevel ),
	m_endMipLevel( mipLevel + 1 ),
	m_box{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Clamps subresource values to a specific texture dimensions.  
// Arguments:
//   texture - Texture which dimensions are used to clamp subresource data
// --------------------------------------------------------------------------------------
void Tr2TextureSubresource::ClampToTexture( const Tr2BitmapDimensions& texture )
{
	uint32_t arraySize = std::max( texture.GetArraySize(), 1u );
	m_startFace = std::min( m_startFace, arraySize - 1 );
	m_endFace = std::min( m_endFace, arraySize );

	m_startMipLevel = std::min( m_startMipLevel, texture.GetTrueMipCount() - 1 );
	m_endMipLevel = std::min( m_endMipLevel, texture.GetTrueMipCount() );

	uint32_t mipWidth  = texture.GetMipWidth( m_startMipLevel );
	uint32_t mipHeight = texture.GetMipHeight( m_startMipLevel );
	uint32_t mipDepth  = std::max( texture.GetDepth() >> m_startMipLevel, 1u );

	if( HasBox() )
	{
		m_box.left = std::min( m_box.left, mipWidth - 1 );
		m_box.right = std::min( m_box.right, mipWidth );
		m_box.top = std::min( m_box.top, mipHeight - 1 );
		m_box.bottom = std::min( m_box.bottom, mipHeight );
		if( texture.GetType() == TEX_TYPE_3D )
		{
			m_box.front = std::min( m_box.front, mipDepth - 1 );
			m_box.back = std::min( m_box.back, mipDepth );
		}
		else
		{
			m_box.front = 0;
			m_box.back = 1;
		}
	}
	else
	{
		m_box = { 0, 0, 0, mipWidth, mipHeight, mipDepth };
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Check if subresource covers the entire textures (all slices and all mip levels).  
// Arguments:
//   texture - Texture to check against
// Return Value:
//   true If subresource data covers the entire texture
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2TextureSubresource::IsSubresourceFull( const Tr2BitmapDimensions& texture ) const
{
	if( m_startFace > 0 || m_endFace < texture.GetArraySize() )
	{
		return false;
	}
	if( m_startMipLevel > 0 )
	{
		return false;
	}
	if( m_endMipLevel < texture.GetTrueMipCount() )
	{
		return false;
	}
	if( HasBox() )
	{
		if( m_box.left > 0 || m_box.top > 0 || m_box.front > 0 )
		{
			return false;
		}
		if( m_box.right < texture.GetWidth() || m_box.bottom < texture.GetHeight() || m_box.back < texture.GetDepth() )
		{
			return false;
		}
	}

	return true;
}

bool Tr2TextureSubresource::IsValidForBitmap( const Tr2BitmapDimensions& bitmap ) const
{
	if( m_endFace > bitmap.GetArraySize() )
	{
		return false;
	}
	if( m_endMipLevel > bitmap.GetTrueMipCount() )
	{
		return false;
	}
	if( HasBox() )
	{
		if( m_box.right > bitmap.GetMipWidth( m_startMipLevel ) )
		{
			return false;
		}
		if( m_box.bottom > bitmap.GetMipHeight( m_startMipLevel ) )
		{
			return false;
		}
		if( bitmap.GetType() == TEX_TYPE_3D && m_box.back > bitmap.GetMipDepth( m_startMipLevel ) )
		{
			return false;
		}
	}
	return true;
}

bool Tr2TextureSubresource::HasBox() const
{
	return m_box.left != 0xffffffff || m_box.top != 0xffffffff || m_box.front != 0xffffffff || m_box.right != 0xffffffff || m_box.bottom != 0xffffffff || m_box.back != 0xffffffff;
}

bool Tr2TextureSubresource::IsSingleSubresource() const
{
	return m_endFace == m_startFace + 1 && m_endMipLevel == m_startMipLevel + 1;
}

Tr2TextureSubresource& Tr2TextureSubresource::SetBox( const uint32_t* ltfrbb )
{
	m_box.left = ltfrbb[0];
	m_box.top = ltfrbb[1];
	m_box.front = ltfrbb[2];
	m_box.right = ltfrbb[3];
	m_box.bottom = ltfrbb[4];
	m_box.back = ltfrbb[5];
	return *this;
}

Tr2TextureSubresource& Tr2TextureSubresource::SetRect( const uint32_t* ltrb )
{
	m_box.left = ltrb[0];
	m_box.top = ltrb[1];
	m_box.front = 0;
	m_box.right = ltrb[2];
	m_box.bottom = ltrb[3];
	m_box.back = 1;
	return *this;
}

Tr2TextureSubresource& Tr2TextureSubresource::SetRect( uint32_t left, uint32_t top, uint32_t right, uint32_t bottom )
{
	m_box.left = left;
	m_box.top = top;
	m_box.front = 0;
	m_box.right = right;
	m_box.bottom = bottom;
	m_box.back = 1;
	return *this;
}

bool Tr2TextureSubresource::operator==( const Tr2TextureSubresource& other ) const
{
	return	m_startFace		== other.m_startFace		&&
			m_endFace		== other.m_endFace			&&
			m_startMipLevel	== other.m_startMipLevel	&&
			m_endMipLevel	== other.m_endMipLevel		&&
			m_box			== other.m_box;
}

bool Tr2TextureSubresource::IsValid() const
{
	if( HasBox() )
	{
		if( m_box.left >= m_box.right || m_box.top >= m_box.bottom || m_box.front >= m_box.back )
		{
			return false;
		}
	}
	return	m_startFace < m_endFace && m_startMipLevel < m_endMipLevel;
}

// Crop both subresources to the given bitmaps as well as each others dimensions;
// and run some basic checks on them (IsValid, formats matching, ...).
// Returns true if all that passed and a copy would make sense.
bool Crop(	Tr2TextureSubresource& sourceSR,
			const Tr2BitmapDimensions& sourceBD, 
			Tr2TextureSubresource& destSR,
			const Tr2BitmapDimensions& destBD )
{
	if( destSR.GetFaceCount()	!= sourceSR.GetFaceCount() )
	{
		return false;
	}

	if( sourceSR.HasBox() && destSR.HasBox() && ( destSR.GetDepth() != sourceSR.GetDepth() ) )
	{
		return false;
	}

	sourceSR.ClampToTexture( sourceBD );
	destSR  .ClampToTexture( destBD   );

	if( sourceSR.GetWidth() < destSR.GetWidth() )
	{
		destSR.m_box.right = destSR.m_box.left + sourceSR.GetWidth();
	}
	else
	{
		sourceSR.m_box.right = sourceSR.m_box.left + destSR.GetWidth();
	}

	if( sourceSR.GetHeight() < destSR.GetHeight() )
	{
		destSR.m_box.bottom = destSR.m_box.top + sourceSR.GetHeight();
	}
	else
	{
		sourceSR.m_box.bottom = sourceSR.m_box.top + destSR.GetHeight();
	}

	sourceSR.ClampToTexture( sourceBD );
	destSR  .ClampToTexture( destBD   );

	if( !sourceSR.IsValid() || !destSR.IsValid() )
	{
		return false;
	}

	return true;
}

// Check if we're moving to a smaller mip, and if so, shrink the rectangle pointed at by
// sub in half.
void AdvanceMip( Tr2TextureSubresource& sub, const Tr2BitmapDimensions& bd, uint32_t mip )
{
	if( bd.GetMipWidth( sub.m_startMipLevel + mip + 1 ) < bd.GetMipWidth( sub.m_startMipLevel + mip ) )
	{
		sub.m_box.left /= 2;
		sub.m_box.right /= 2;

		if( bd.IsCompressed() && sub.GetWidth() < 4 )
		{
			sub.m_box.right = sub.m_box.left + 4;
		}
	}

	if( bd.GetMipHeight( sub.m_startMipLevel + mip + 1 ) < bd.GetMipHeight( sub.m_startMipLevel + mip ) )
	{
		sub.m_box.top /= 2;
		sub.m_box.bottom /= 2;

		if( bd.IsCompressed() && sub.GetHeight() < 4 )
		{
			sub.m_box.bottom = sub.m_box.top + 4;
		}
	}

	if( bd.GetMipDepth( sub.m_startMipLevel + mip + 1 ) < bd.GetMipDepth( sub.m_startMipLevel + mip ) )
	{
		sub.m_box.front /= 2;
		sub.m_box.back /= 2;

		if( bd.IsCompressed() && sub.GetDepth() < 4 )
		{
			sub.m_box.bottom = sub.m_box.top + 4;
		}
	}
}
