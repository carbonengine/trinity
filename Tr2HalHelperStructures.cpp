#include "StdAfx.h"
#include "Tr2HalHelperStructures.h"

#include "include/Tr2ShaderAL.h"
#include "include/Tr2ConstantBufferAL.h"
#include "include/Tr2VertexLayoutAL.h"
#include "include/Tr2ShaderProgramAL.h"
#include "include/Tr2ShaderProgramAL.h"

using namespace Tr2RenderContextEnum;

const Tr2ShaderAL		nullShader[SHADER_TYPE_COUNT];

namespace
{
	struct NullShaderInitializer
	{
		NullShaderInitializer()
		{
			for( int i = SHADER_TYPE_FIRST; i < SHADER_TYPE_COUNT; ++i )
			{
				const_cast<Tr2ShaderAL&>( nullShader[i] ).SetNullShaderType( ShaderType( i ) );
			}
		}
	};

	NullShaderInitializer	s_nullShaderInitializer;
}

const Tr2ConstantBufferAL	nullCB;
const Tr2VertexLayoutAL	nullVL;
const Tr2ShaderProgramAL nullSP;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2TextureSubresource default constructor: construct a subresource range containing
//   the entire resource.
// --------------------------------------------------------------------------------------
Tr2TextureSubresource::Tr2TextureSubresource()
	: m_startFace( 0 )
	, m_endFace( std::numeric_limits<uint32_t>::max() ),
	m_startMipLevel( 0 ),
	m_endMipLevel( 0xffffffff ),
	m_left( 0xffffffff ),
	m_top( 0xffffffff ),
	m_front( 0xffffffff ),
	m_right( 0xffffffff ),
	m_bottom( 0xffffffff ),
	m_back( 0xffffffff )
{
}

Tr2TextureSubresource::Tr2TextureSubresource( uint32_t mipLevel )
	:m_startFace( 0 ),
	m_endFace( 1 ),
	m_startMipLevel( mipLevel ),
	m_endMipLevel( mipLevel + 1 ),
	m_left( 0xffffffff ),
	m_top( 0xffffffff ),
	m_front( 0xffffffff ),
	m_right( 0xffffffff ),
	m_bottom( 0xffffffff ),
	m_back( 0xffffffff )
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
	m_left( 0xffffffff ),
	m_top( 0xffffffff ),
	m_front( 0xffffffff ),
	m_right( 0xffffffff ),
	m_bottom( 0xffffffff ),
	m_back( 0xffffffff )
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
		m_left = std::min( m_left, mipWidth - 1 );
		m_right = std::min( m_right, mipWidth );
		m_top = std::min( m_top, mipHeight - 1 );
		m_bottom = std::min( m_bottom, mipHeight );
		if( texture.GetType() == TEX_TYPE_3D )
		{
			m_front = std::min( m_front, mipDepth - 1 );
			m_back = std::min( m_back, mipDepth );
		}
		else
		{
			m_front = 0;
			m_back = 1;
		}
	}
	else
	{
		m_left = m_top = m_front = 0;
		m_right = mipWidth;
		m_bottom = mipHeight;
		m_back = mipDepth;
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
		if( m_left > 0 || m_top > 0 || m_front > 0 )
		{
			return false;
		}
		if( m_right < texture.GetWidth() || m_bottom < texture.GetHeight() || m_back < texture.GetDepth() )
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
		if( m_right > bitmap.GetMipWidth( m_startMipLevel ) )
		{
			return false;
		}
		if( m_bottom > bitmap.GetMipHeight( m_startMipLevel ) )
		{
			return false;
		}
		if( bitmap.GetType() == TEX_TYPE_3D && m_back > bitmap.GetMipDepth( m_startMipLevel ) )
		{
			return false;
		}
	}
	return true;
}

bool Tr2TextureSubresource::HasBox() const
{
	return m_left != 0xffffffff || m_top != 0xffffffff || m_front != 0xffffffff || m_right != 0xffffffff || m_bottom != 0xffffffff || m_back != 0xffffffff;
}

bool Tr2TextureSubresource::IsSingleSubresource() const
{
	return m_endFace == m_startFace + 1 && m_endMipLevel == m_startMipLevel + 1;
}

Tr2TextureSubresource& Tr2TextureSubresource::SetBox( const uint32_t* ltfrbb )
{
	m_left = ltfrbb[0];
	m_top = ltfrbb[1];
	m_front = ltfrbb[2];
	m_right = ltfrbb[3];
	m_bottom = ltfrbb[4];
	m_back = ltfrbb[5];
	return *this;
}

Tr2TextureSubresource& Tr2TextureSubresource::SetRect( const uint32_t* ltrb )
{
	m_left = ltrb[0];
	m_top = ltrb[1];
	m_front = 0;
	m_right = ltrb[2];
	m_bottom = ltrb[3];
	m_back = 1;
	return *this;
}

Tr2TextureSubresource& Tr2TextureSubresource::SetRect( uint32_t left, uint32_t top, uint32_t right, uint32_t bottom )
{
	m_left = left;
	m_top = top;
	m_front = 0;
	m_right = right;
	m_bottom = bottom;
	m_back = 1;
	return *this;
}

bool Tr2TextureSubresource::operator==( const Tr2TextureSubresource& other ) const
{
	return	m_startFace		== other.m_startFace		&&
			m_endFace		== other.m_endFace			&&
			m_startMipLevel	== other.m_startMipLevel	&&
			m_endMipLevel	== other.m_endMipLevel		&&
			m_left			== other.m_left				&&
			m_top			== other.m_top				&&
			m_front			== other.m_front			&&
			m_right			== other.m_right			&&
			m_bottom		== other.m_bottom			&&
			m_back			== other.m_back;
}

bool Tr2TextureSubresource::IsValid() const
{
	if( HasBox() )
	{
		if( m_left >= m_right || m_top >= m_bottom || m_front >= m_back )
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
		destSR.m_right = destSR.m_left + sourceSR.GetWidth();
	}
	else
	{
		sourceSR.m_right = sourceSR.m_left + destSR.GetWidth();
	}

	if( sourceSR.GetHeight() < destSR.GetHeight() )
	{
		destSR.m_bottom = destSR.m_top + sourceSR.GetHeight();
	}
	else
	{
		sourceSR.m_bottom = sourceSR.m_top + destSR.GetHeight();
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
		sub.m_left  /= 2;
		sub.m_right /= 2;

		if( bd.IsCompressed() && sub.GetWidth() < 4 )
		{
			sub.m_right = sub.m_left + 4;
		}
	}

	if( bd.GetMipHeight( sub.m_startMipLevel + mip + 1 ) < bd.GetMipHeight( sub.m_startMipLevel + mip ) )
	{
		sub.m_top    /= 2;
		sub.m_bottom /= 2;

		if( bd.IsCompressed() && sub.GetHeight() < 4 )
		{
			sub.m_bottom = sub.m_top + 4;
		}
	}

	if( bd.GetMipDepth( sub.m_startMipLevel + mip + 1 ) < bd.GetMipDepth( sub.m_startMipLevel + mip ) )
	{
		sub.m_front /= 2;
		sub.m_back  /= 2;

		if( bd.IsCompressed() && sub.GetDepth() < 4 )
		{
			sub.m_bottom = sub.m_top + 4;
		}
	}
}
