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

	const unsigned destX = m_destinationViewport ? m_destinationViewport->x : 0;
	const unsigned destY = m_destinationViewport ? m_destinationViewport->y : 0;


	HRESULT hr = 0;

	if( m_destinationRT )
	{
		if( !m_sourceViewport )
		{
			Tr2TextureSubresource dest( 0 );
			dest.SetRect( destX, destY, destX + m_sourceRT->GetWidth(), destY + m_sourceRT->GetHeight() );
			hr = m_destinationRT->GetRenderTarget().CopySubresourceRegion( dest, *m_sourceRT, Tr2TextureSubresource( 0 ), renderContext );
		}
		else
		{
			const auto& vp = *m_sourceViewport;
			Tr2TextureSubresource src( 0 );
			src.SetRect( (uint32_t)vp.x, (uint32_t)vp.y, (uint32_t)(vp.x + vp.width), (uint32_t)(vp.y + vp.height) );
			Tr2TextureSubresource dest( 0 );
			dest.SetRect( destX, destY, destX + src.m_right - src.m_left, destY + src.m_bottom - src.m_top );

			hr = m_destinationRT->GetRenderTarget().CopySubresourceRegion( dest, *m_sourceRT, src, renderContext );
		}
	}
	else
	if( m_destinationTexture && m_destinationTexture->GetTexture() )
	{
		Tr2TextureSubresource destView;
		destView.m_left = destX;
		destView.m_top  = destY;

		Tr2TextureSubresource sourceView;
		if( m_sourceViewport )
		{
			const auto& vp = *m_sourceViewport;
			sourceView.m_left		= vp.x;
			sourceView.m_top		= vp.y;
			sourceView.m_right		= vp.x + vp.width;
			sourceView.m_bottom		= vp.y + vp.height;			
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