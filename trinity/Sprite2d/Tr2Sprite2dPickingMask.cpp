////////////////////////////////////////////////////////////
//
//    Created:   October 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "Tr2Sprite2dPickingMask.h"
#include "Resources/Tr2ImageRes.h"


Tr2Sprite2dPickingMask::Tr2Sprite2dPickingMask( IRoot* ) :
	m_channel( 3 ),
	m_threshold( 0.f ),
	m_leftEdge( 0 ),
	m_topEdge( 0 ),
	m_rightEdge( 0 ),
	m_bottomEdge( 0 )
{
}

const std::wstring Tr2Sprite2dPickingMask::GetMaskPath() const
{
	return m_maskPath;
}

void Tr2Sprite2dPickingMask::SetMaskPath( const wchar_t* path )
{
	if( m_maskPath != path )
	{
		m_maskPath = path;
		m_mask = nullptr;
		BeResMan->GetResource( m_maskPath, L"raw", m_mask );
	}
}

bool Tr2Sprite2dPickingMask::SampleMask( const Vector2& point, const Vector2& topLeft, float width, float height ) const
{
	if( !m_mask || !m_mask->IsGood() )
	{
		return false;
	}
	auto& bitmap = m_mask->GetBitmap();
	auto mwidth = bitmap.GetWidth();
	auto mheight = bitmap.GetHeight();

	if( mwidth < m_leftEdge + m_rightEdge )
	{
		return false;
	}
	if( mheight < m_topEdge + m_bottomEdge )
	{
		return false;
	}

	float x = std::max( 0.f, std::min( width, point.x - 0.5f - topLeft.x ) );
	float y = std::max( 0.f, std::min( height, point.y - 0.5f - topLeft.y ) );

	uint32_t px, py;

	if( x < m_leftEdge )
	{
		px = uint32_t( x );
	}
	else if( x >= width - m_rightEdge )
	{
		px = std::min( mwidth - uint32_t( width - x ), mwidth - 1 );
	}
	else
	{
		if( mwidth == m_leftEdge + m_rightEdge )
		{
			px = m_leftEdge;
		}
		else
		{
			px = std::min( uint32_t( ( x - m_leftEdge ) / ( width - m_leftEdge - m_rightEdge ) * ( mwidth - m_leftEdge - m_rightEdge ) + m_leftEdge ), mwidth - 1 );
		}
	}

	if( y < m_topEdge )
	{
		py = uint32_t( y );
	}
	else if( y >= height - m_bottomEdge )
	{
		py = std::min( mheight - uint32_t( height - y ), mheight - 1 );
	}
	else
	{
		if( mheight == m_topEdge + m_bottomEdge )
		{
			py = m_topEdge;
		}
		else
		{
			py = std::min( uint32_t( ( y - m_topEdge ) / ( height - m_topEdge - m_bottomEdge ) * ( mheight - m_topEdge - m_bottomEdge ) + m_topEdge ), mheight - 1 );
		}
	}

	float channels[4];
	if( !bitmap.GetPixel( px, py, channels[0], channels[1], channels[2], channels[3] ) )
	{
		return false;
	}
	auto channel = std::min( m_channel, 3u );
	if( bitmap.GetFormat() == Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM )
	{
		channel = 2;
	}
	return channels[channel] > m_threshold;
}
