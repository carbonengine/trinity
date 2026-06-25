// Copyright © 2024 CCP ehf.

#include "Tr2SSSSS.h"
#include "Tr2Renderer.h"
#include "Tr2TextureReference.h"
#include "Tr2DepthStencil.h"
#include "Tr2RenderTarget.h"
#include "Tr2GpuBuffer.h"
#include "TriRenderBatch.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2Shader.h"
#include "Resources/TriTextureRes.h"
#include "Tr2GpuStructuredBuffer.h"
#include "../Tr2GpuResourcePool.h"

using namespace Tr2RenderContextEnum;


namespace
{
Vector3 gaussian( float variance, float r )
{
	/**
			 * We use a falloff to modulate the shape of the profile. Big falloffs
			 * spreads the shape making it wider, while small falloffs make it
			 * narrower.
			 */
	Vector3 falloff = Vector3( 1.0f, 0.37f, 0.3f );

	Vector3 g;
	for( int i = 0; i < 3; i++ )
	{
		float rr = r / ( 0.001f + falloff[i] );
		g[i] = exp( ( -( rr * rr ) ) / ( 2.0f * variance ) ) / ( 2.0f * 3.14f * variance );
	}
	return g;
}

Vector3 profile( float r )
{
	/**
			 * We used the red channel of the original skin profile defined in
			 * [d'Eon07] for all three channels. We noticed it can be used for green
			 * and blue channels (scaled using the falloff parameter) without
			 * introducing noticeable differences and allowing for total control over
			 * the profile. For example, it allows to create blue SSS gradients, which
			 * could be useful in case of rendering blue creatures.
			 */
	return // 0.233f * gaussian(0.0064f, r) + /* We consider this one to be directly bounced light, accounted by the strength parameter (see @STRENGTH) */
		gaussian( 0.0484f, r ) * 0.100f +
		gaussian( 0.187f, r ) * 0.118f +
		gaussian( 0.567f, r ) * 0.113f +
		gaussian( 1.99f, r ) * 0.358f +
		gaussian( 7.41f, r ) * 0.078f;
}
}

Tr2SSSSS::Tr2SSSSS( IRoot* lockobj ) :
	m_enabled( true ),
	m_hasSSSSSInScene( true ),
	m_subSurfaceScatteringWidth( 0.1277f ),
	m_subSurfaceFrontScatterColor( Color( 1, 1, 1, 1 ) )
{
}

void Tr2SSSSS::SetupScreenSpaceSubSurfaceScattering( Tr2RenderContext& renderContext, ITriRenderBatchAccumulator* batches, const Tr2TextureAL& colorMap, const Tr2TextureAL& opaqueColorMap, const Tr2TextureAL& depthMap, Tr2GpuResourcePool& gpuResourcePool )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_enabled )
	{
		return;
	}

	static const auto SSSSS = BlueSharedString( "SSSSS" );
	m_hasSSSSSInScene = renderContext.TechniqueInBatch( batches->GetGdprBatches(), SSSSS ) || renderContext.TechniqueInBatch( batches->GetBatches(), SSSSS );
	if( !m_hasSSSSSInScene )
	{
		return;
	}

	auto seprableSpecularColorMap = gpuResourcePool.GetTempTexture( "SeprableSpecularColorMap", colorMap.GetWidth(), colorMap.GetHeight(), ImageIO::PIXEL_FORMAT_R16G16B16A16_FLOAT, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
	// Update RT
	{
		// Using depth stencil as shadow map
		renderContext.m_esm.PushViewport();
		renderContext.m_esm.PushRenderTarget( seprableSpecularColorMap );


		renderContext.m_esm.UpdateRenderTargetViewport( seprableSpecularColorMap->GetWidth(), seprableSpecularColorMap->GetHeight() );

		renderContext.Clear( CLEARFLAGS_TARGET, 0, 0, 0 );
	}

	// JKG need to confirm m_primaryBatches is a safe batch to render from
	{
		CCP_STATS_ZONE( "SeprableSpecularRendering" );

		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
		renderContext.RenderBatches( batches, SSSSS );
	}


	// Reset render target back to OG
	{
		renderContext.m_esm.PopRenderTarget();
		renderContext.m_esm.PopViewport();
	}


	// Setup the blur effect as well as the buffers required for the blur
	if( m_screenSpaceSubSurfaceScatteringEffect == nullptr )
	{
		m_subSurfaceFrontScatterColorBuffer.CreateInstance();

		// Currently we only have once instance of the front scatter data, this can be scaled in the future if needed
		m_subSurfaceFrontScatterColorBuffer->Create( 1, sizeof( PerSubSurfaceFrontScatterData ), Tr2GpuBuffer::CPU_WRITABLE );
		m_subSurfaceFrontScatterColorBuffer->SetName( "SubSurface Front Scatter Color Buffer" );

		// Create and configure the basic effect settings
		m_screenSpaceSubSurfaceScatteringEffect.CreateInstance();
		m_screenSpaceSubSurfaceScatteringEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpaceObject/V5/PBR/Global/SSSSSBlur.fx" );
		m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "SubSurfaceFrontScatterColorBuffer" ), m_subSurfaceFrontScatterColorBuffer );
	}


	// Generate from the incoming data defined via blue the blur kernal
	UpdateSubSurfaceFrontScatterData( renderContext );


	// 1: Blur X
	{
		m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "SSSWidth" ), m_subSurfaceScatteringWidth );
		m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "SSSDirection" ), Vector2( 1.0f, 0.0f ) );
		m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "BlitCurrent" ), opaqueColorMap );
		m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "DepthMap" ), depthMap );
		m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "SeprableSpecularMap" ), seprableSpecularColorMap );
		Tr2Renderer::DrawScreenQuad( renderContext, m_screenSpaceSubSurfaceScatteringEffect );
	}


	// 2: Blur Y
	{
		// Update RT
		{
			renderContext.m_esm.PushViewport();
			renderContext.m_esm.PushRenderTarget( opaqueColorMap );

			renderContext.m_esm.UpdateRenderTargetViewport( opaqueColorMap.GetWidth(), opaqueColorMap.GetHeight() );
		}

		m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "SSSWidth" ), m_subSurfaceScatteringWidth );
		m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "SSSDirection" ), Vector2( 0.0f, 1.0f ) );
		m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "BlitCurrent" ), colorMap );
		m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "DepthMap" ), depthMap );
		m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "SeprableSpecularMap" ), seprableSpecularColorMap );
		Tr2Renderer::DrawScreenQuad( renderContext, m_screenSpaceSubSurfaceScatteringEffect );

		// Reset render target back to OG
		{
			renderContext.m_esm.PopRenderTarget();
			renderContext.m_esm.PopViewport();
		}
	}

	m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "BlitCurrent" ), Tr2TextureAL{} );
	m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "DepthMap" ), Tr2TextureAL{} );
	m_screenSpaceSubSurfaceScatteringEffect->SetParameter( BlueSharedString( "SeprableSpecularMap" ), Tr2TextureAL{} );

	// 3: Specular Recombine after blur
	{
		if( m_specularRecombineEffect == nullptr )
		{
			m_specularRecombineEffect.CreateInstance();
			m_specularRecombineEffect->StartUpdate();
			m_specularRecombineEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpaceObject/V5/PBR/Global/SSSSSRecombine.fx" );
			m_specularRecombineEffect->EndUpdate();
		}

		m_specularRecombineEffect->SetParameter( BlueSharedString( "BlitCurrent" ), opaqueColorMap );
		m_specularRecombineEffect->SetParameter( BlueSharedString( "SeprableSpecularMap" ), seprableSpecularColorMap );
		Tr2Renderer::DrawScreenQuad( renderContext, m_specularRecombineEffect );
		m_specularRecombineEffect->SetParameter( BlueSharedString( "SeprableSpecularMap" ), Tr2TextureAL{} );
	}
}

void Tr2SSSSS::UpdateSubSurfaceFrontScatterData( Tr2RenderContext& renderContext )
{

	// FUTUREWORK this will be exposed via a "quality" setting
	const int nSamples = 17;
	std::array<Vector4, nSamples> kernel;

	// Update front scattering kernal
	{


		const float RANGE = nSamples > 20 ? 3.0f : 2.0f;
		const float EXPONENT = 2.0f;


		// Calculate the offsets
		float step = 2.0f * RANGE / ( nSamples - 1 );
		for( int i = 0; i < nSamples; i++ )
		{
			float o = -RANGE + float( i ) * step;
			float sign = o < 0.0f ? -1.0f : 1.0f;
			kernel[i].w = RANGE * sign * abs( pow( o, EXPONENT ) ) / pow( RANGE, EXPONENT );
		}

		// Calculate the weights
		for( int i = 0; i < nSamples; i++ )
		{
			float w0 = i > 0 ? abs( kernel[i].w - kernel[i - 1].w ) : 0.0f;
			float w1 = i < nSamples - 1 ? abs( kernel[i].w - kernel[i + 1].w ) : 0.0f;
			float area = ( w0 + w1 ) / 2.0f;
			Vector3 t = profile( kernel[i].w ) * area;
			kernel[i].x = t.x;
			kernel[i].y = t.y;
			kernel[i].z = t.z;
		}


		// We want the offset 0.0 to come first
		Vector4 t = kernel[nSamples / 2];
		for( int i = nSamples / 2; i > 0; i-- )
			kernel[i] = kernel[i - 1];
		kernel[0] = t;


		// Calculate the sum of the weights, we will need to normalize them below
		Vector3 sum = Vector3( 0.0f, 0.0f, 0.0f );
		for( int i = 0; i < nSamples; i++ )
		{
			sum += Vector3( kernel[i].x, kernel[i].y, kernel[i].z );
		}

		// Normalize the weights
		for( int i = 0; i < nSamples; i++ )
		{
			kernel[i].x /= sum.x;
			kernel[i].y /= sum.y;
			kernel[i].z /= sum.z;
		}

		// Tweak them using the desired strength. The first one is
		//     lerp(1.0, kernel[0].rgb, strength)
		kernel[0].x = ( 1.0f - m_subSurfaceFrontScatterColor.r ) * 1.0f + m_subSurfaceFrontScatterColor.r * kernel[0].x;
		kernel[0].y = ( 1.0f - m_subSurfaceFrontScatterColor.g ) * 1.0f + m_subSurfaceFrontScatterColor.g * kernel[0].y;
		kernel[0].z = ( 1.0f - m_subSurfaceFrontScatterColor.b ) * 1.0f + m_subSurfaceFrontScatterColor.b * kernel[0].z;

		// The others
		//     lerp(0.0, kernel[0].rgb, m_subSurfaceFrontScatterColor)
		for( int i = 1; i < nSamples; i++ )
		{
			kernel[i].x *= m_subSurfaceFrontScatterColor.r;
			kernel[i].y *= m_subSurfaceFrontScatterColor.g;
			kernel[i].z *= m_subSurfaceFrontScatterColor.b;
		}
	}

	// Update the kernal data for the gpu to consume
	Vector4* data;
	m_subSurfaceFrontScatterColorBuffer->GetGpuBuffer( 0 )->MapForWriting( data, renderContext );

	memcpy( data, kernel.data(), MAX_BLUR_KERNALS * sizeof( Vector4 ) );
	m_subSurfaceFrontScatterColorBuffer->GetGpuBuffer( 0 )->UnmapForWriting( renderContext );
}