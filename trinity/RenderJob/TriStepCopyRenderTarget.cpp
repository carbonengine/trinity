// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepCopyRenderTarget.h"
#include "Tr2RenderTarget.h"
#include "Resources/TriTextureRes.h"
#include "TriViewport.h"

TriStepCopyRenderTarget::TriStepCopyRenderTarget( IRoot* lockobj )
{
}

TriStepResult TriStepCopyRenderTarget::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( ( !m_destinationRT && !m_destinationTexture ) || !m_sourceRT )
	{
		return RS_OK;
	}

	unsigned destX = m_destinationViewport ? m_destinationViewport->x : 0;
	unsigned destY = m_destinationViewport ? m_destinationViewport->y : 0;


	HRESULT hr = 0;

	if( m_destinationRT )
	{
		if( !m_sourceViewport )
		{
			Tr2TextureSubresource dest( 0 );
			dest.SetRect( destX, destY, destX + m_sourceRT->GetWidth(), destY + m_sourceRT->GetHeight() );

			if( m_destinationViewport )
			{
				if( m_destinationViewport->x < 0 )
				{
					dest.m_box.left = 0;
					dest.m_box.right = uint32_t( int32_t( m_sourceRT->GetWidth() ) + int32_t( m_destinationViewport->x ) );
				}
				if( m_destinationViewport->y < 0 )
				{
					dest.m_box.top = 0;
					dest.m_box.bottom = uint32_t( int32_t( m_sourceRT->GetHeight() ) + int32_t( m_destinationViewport->y ) );
				}
			}
			hr = m_destinationRT->GetRenderTarget().CopySubresourceRegion( dest, *m_sourceRT, Tr2TextureSubresource( 0 ), renderContext );
		}
		else
		{
			const auto& vp = *m_sourceViewport;
			Tr2TextureSubresource src( 0 );
			src.SetRect( (uint32_t)vp.x, (uint32_t)vp.y, (uint32_t)( vp.x + vp.width ), (uint32_t)( vp.y + vp.height ) );
			if( vp.width <= 0 || vp.height <= 0 )
			{
				return RS_OK;
			}

			if( m_destinationViewport )
			{
				if( m_destinationViewport->x < 0 )
				{
					destX = 0;
					src.m_box.right -= uint32_t( -m_destinationViewport->x );
				}
				if( m_destinationViewport->y < 0 )
				{
					destY = 0;
					src.m_box.bottom -= uint32_t( -m_destinationViewport->y );
				}
			}

			Tr2TextureSubresource dest( 0 );
			dest.SetRect( destX, destY, destX + src.m_box.right - src.m_box.left, destY + src.m_box.bottom - src.m_box.top );

			hr = m_destinationRT->GetRenderTarget().CopySubresourceRegion( dest, *m_sourceRT, src, renderContext );
		}
	}
	else if( m_destinationTexture && m_destinationTexture->GetTexture() )
	{
		Tr2TextureSubresource destView;
		destView.m_box.left = destX;
		destView.m_box.top = destY;

		Tr2TextureSubresource sourceView;
		if( m_sourceViewport )
		{
			const auto& vp = *m_sourceViewport;
			sourceView.m_box.left = vp.x;
			sourceView.m_box.top = vp.y;
			sourceView.m_box.right = vp.x + vp.width;
			sourceView.m_box.bottom = vp.y + vp.height;
		}

		hr = m_destinationTexture->GetTexture()->CopySubresourceRegion( destView, *m_sourceRT, sourceView, renderContext );
	}

	if( !SUCCEEDED( hr ) )
	{
		CCP_LOGERR( "TriStepCopyRenderTarget: CopySubresourceRegion failed - 0x%x", hr );
		return RS_FAILED;
	}

	return RS_OK;
}