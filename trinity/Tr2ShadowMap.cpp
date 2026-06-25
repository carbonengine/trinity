// Copyright © 2022 CCP ehf.

#include "StdAfx.h"
#include "Tr2ShadowMap.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2EffectStateManager.h"

namespace
{
const Vector3 unitCube[8] = {
	//The unit cube in DirectX is from( -1, -1, 0 ) to ( 1, 1, 1 )
	Vector3( -1, -1, 0 ), // vertex 0
	Vector3( -1, 1, 0 ), // vertex 1
	Vector3( 1, 1, 0 ), // etc..
	Vector3( 1, -1, 0 ),
	Vector3( -1, -1, 1 ),
	Vector3( -1, 1, 1 ),
	Vector3( 1, 1, 1 ),
	Vector3( 1, -1, 1 )
};

}

using namespace Tr2RenderContextEnum;
Tr2ShadowMap::Tr2ShadowMap( IRoot* lockobj ) :
	m_size( SHADOW_MAP_SIZE ),
	m_height( 2 ),
	m_width( 8 ),
	m_splitCount( SHADOW_FRUSTUM_COUNT ),
	m_disableShimmer( true ),
	m_oldZFar( 0.0 ),
	m_useDenoiser( true ),
	m_debugColorSplit( false ),
	m_lastNearClip( 0.0f ),
	m_lastFarClip( 0.0f ),
	m_shadowSplitMode( Tr2ShadowMap::ShadowSplitMode::STATIC )
{
	m_shadowEffect.CreateInstance();
	m_shadowEffect->SetEffectPathName( "res:/graphics/effect/managed/space/system/ShadowDepth.fx" );
	m_shadowEffect->SetParameter( BlueSharedString( "EveSpaceSceneCascadedShadowMap" ), Tr2TextureAL{} );
	m_shadowEffect->SetParameter( BlueSharedString( "DepthMap" ), Tr2TextureAL{} );
	GlobalStore().RegisterVariable( "EveSpaceSceneShadowMap", static_cast<ITr2TextureProvider*>( nullptr ) );
	GlobalStore().RegisterVariable( "EveSpaceSceneCascadedShadowMap", static_cast<ITr2TextureProvider*>( nullptr ) );

	m_denoiser.CreateInstance();

	SetStaticShadowSplits();
}

void Tr2ShadowMap::Setup( uint32_t elementSize, uint32_t elementCount, bool useDenoiser )
{
	if( m_size != elementSize || m_splitCount != elementCount )
	{
		m_size = elementSize;
		m_splitCount = elementCount;
		m_perSplitData.SplitInfo.x = float( m_splitCount );
	}
	m_useDenoiser = useDenoiser;
	if( !useDenoiser )
	{
		m_denoiser = nullptr;
	}
}

bool Tr2ShadowMap::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_debugColorSplit ) )
	{
		if( m_debugColorSplit )
		{
			m_shadowEffect->SetOption( BlueSharedString( "SHADOW_DEBUG_MODE" ), BlueSharedString( "SDM_COLOR" ) );
		}
		else
		{
			m_shadowEffect->SetOption( BlueSharedString( "SHADOW_DEBUG_MODE" ), BlueSharedString( "SDM_NONE" ) );
		}
	}
	if( IsMatch( value, m_shadowSplitMode ) )
	{
		if( m_shadowSplitMode == Tr2ShadowMap::ShadowSplitMode::STATIC )
		{
			SetStaticShadowSplits();
		}
		else if( m_shadowSplitMode == Tr2ShadowMap::ShadowSplitMode::DYNAMIC )
		{
			m_lastNearClip = 0.0f;
			m_lastFarClip = 0.0f;
		}
	}
	m_perSplitData.SplitInfo.x = float( m_splitCount );
	return true;
}

void Tr2ShadowMap::ShouldUseDenoiser( bool val )
{
	m_useDenoiser = val;
}

void Tr2ShadowMap::UpdateSplitValues( float nearClip, float farClip )
{
	if( m_shadowSplitMode == Tr2ShadowMap::DYNAMIC )
	{
		if( m_lastNearClip != nearClip || m_lastFarClip != farClip )
		{
			m_lastNearClip = nearClip;
			m_lastFarClip = farClip;

			float logNearClip = log2f( nearClip );
			float logFarClip = log2f( farClip );

			for( uint32_t i = 0; i < m_splitCount; i++ )
			{
				m_splitValues[i] = exp2f( logNearClip + ( ( logFarClip - logNearClip ) * ( ( i + 1 ) / float( m_splitCount ) ) ) );
			}
		}
	}
}

AxisAlignedBoundingBox Tr2ShadowMap::CalculateAABB( Matrix projection, Matrix invViewTransform, Matrix lightView, Vector3 ( &corners )[8] )
{
	AxisAlignedBoundingBox aabb;
	for( unsigned int i = 0; i < 8; ++i )
	{
		Vector3 vertex = DX_UNIT_CUBE[i];
		// view space
		Vector4 transformedVertex = Transform( Vector4( vertex, 1.0 ), Inverse( projection ) );

		transformedVertex /= transformedVertex.w;

		// world space
		transformedVertex = Transform( transformedVertex, invViewTransform );
		corners[i] = TransformCoord( transformedVertex.GetXYZ(), ( lightView ) );
		// light view space
		aabb.IncludePoint( TransformCoord( transformedVertex.GetXYZ(), ( lightView ) ) );
	}
	return aabb;
}
// --------------------------------------------------------------------------------
// Description:
//  Go through all i count of frustum splits. Calculate the corresponding
//	bounding box based on zNear and zFar values.
// --------------------------------------------------------------------------------
ShadowMap::SplitSetup Tr2ShadowMap::SetupShadowSplit( int splitIndex, Matrix invViewTransform, const Vector3 lightDirection, float zNear, float leftDivNear, float rightDivNear, float topDivNear, float bottomDivNear )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	ShadowMap::SplitSetup splitSetup;

	if( splitIndex == 0 )
	{
		// reset this value
		m_oldZFar = zNear;
	}

	float zFar = m_splitValues[splitIndex];

	// to save some shader data mem we combine the zFar values into a float4 array of 4 so (x,y,z,w) are zFar values
	m_perSplitData.ShadowMapValues[splitIndex / 4][splitIndex % 4] = zFar;

	float left = leftDivNear * m_oldZFar;
	float right = rightDivNear * m_oldZFar;
	float top = topDivNear * m_oldZFar;
	float bottom = bottomDivNear * m_oldZFar;
	auto projection = PerspectiveOffCenterMatrix( left, right, bottom, top, m_oldZFar, zFar );

	m_oldZFar = zFar;

	// we can apply the inverse of the view and projection matrices on the corner points of the unit cube to get the frustum corners in world space
	splitSetup.invViewProj = Inverse( projection ) * invViewTransform;

	// Find light view
	Matrix lightView = Inverse( OrthoNormalBasisZ( -lightDirection ) );

	Vector3 corners[8];

	AxisAlignedBoundingBox aabb = CalculateAABB( projection, invViewTransform, lightView, corners );

	// Snap the projection in texel-sized increments to avoid crawling shadows on still objects
	if( m_disableShimmer )
	{
		// find max dist
		float maxDist = 0.0;

		// First take the farthest corners of our camera view projection
		// subtract them, get the length of the results and divide by 2 to get the radius
		for( unsigned int i = 0; i < 8; ++i )
		{
			// length between i and j is the same as between j and i so let's skip that comparison
			for( unsigned int j = i + 1; j < 8; ++j )
			{
				// longest dist between 2 corners
				maxDist = std::max( maxDist, Length( corners[i] - corners[j] ) );
			}
		}
		float radius = std::ceil( maxDist / 2.0f );
		Vector3 shipPos = Vector3( 0.0, 0.0, 0.0 );
		Vector3 center = aabb.Center();
		// rounding up
		float texelSize = ( radius * 2.0f ) / m_size;
		center.x = std::floor( center.x / texelSize + 0.5f ) * texelSize;
		center.y = std::floor( center.y / texelSize + 0.5f ) * texelSize;

		aabb = AxisAlignedBoundingBox( center - Vector3( radius, radius, radius ), center + Vector3( radius, radius, radius ) );
	}

	// pull the aabb towards the sun
	//aabb.m_max.z += 250000.f;
	splitSetup.aabb = aabb;

	splitSetup.lightViewProjection = lightView * OrthoOffCenterMatrix( aabb.m_max.x, aabb.m_min.x, aabb.m_max.y, aabb.m_min.y, -aabb.m_max.z, -aabb.m_min.z );

	m_perSplitData.CascadeRanges[splitIndex] = Vector4( aabb.m_max.x - aabb.m_min.x, aabb.m_max.y - aabb.m_min.y, aabb.m_max.z - aabb.m_min.z, 0 );

	// 4th element of shadowMatrix is always the same
	m_perSplitData.ShadowMatrixVal[splitIndex] = Transpose( splitSetup.lightViewProjection );

	// create shadow frustum out from lightView, aabb.min, aabb.max
	TriFrustumOrtho shadowFrustum;
	shadowFrustum.DeriveFrustum( lightView, aabb.m_min, aabb.m_max );
	splitSetup.shadowFrustum = shadowFrustum;

	return splitSetup;
}

Tr2GpuResourcePool::Texture Tr2ShadowMap::PrepareShadowRendering( Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto cascadedShadowDepth = gpuResourcePool.GetTempTexture( "cascadedShadowDepth", m_size * m_width, m_size * m_height, PixelFormat::PIXEL_FORMAT_D32_FLOAT, Tr2GpuUsage::DEPTH_STENCIL | Tr2GpuUsage::SHADER_RESOURCE );

	if( !cascadedShadowDepth.IsValid() )
	{
		// This could happen if device is lost
		return {};
	}

	// Using depth stencil as shadow map
	renderContext.m_esm.PushViewport();
	renderContext.m_esm.PushRenderTarget( Tr2TextureAL() ); //empty texture
	renderContext.m_esm.PushDepthStencilBuffer( cascadedShadowDepth );

	renderContext.m_esm.UpdateRenderTargetViewport( cascadedShadowDepth->GetWidth(), cascadedShadowDepth->GetHeight() );

	// we want a clean depth buffer for this
	CR( renderContext.Clear( CLEARFLAGS_ZBUFFER, 0xffffffff, 1, 0 ) );

	renderContext.SetReadOnlyDepth( false );

	return cascadedShadowDepth;
}

void Tr2ShadowMap::BeginShadowRendering( Tr2RenderContext& renderContext, int splitIndex )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( splitIndex < 8 )
	{
		renderContext.m_esm.SetViewport( m_size, m_size, splitIndex * m_size, 0, 0.f, 1.f );
	}
	else
	{
		renderContext.m_esm.SetViewport( m_size, m_size, ( splitIndex % 8 ) * m_size, m_size, 0.f, 1.f );
	}
}

void Tr2ShadowMap::EndShadowRendering( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	renderContext.SetReadOnlyDepth( false );

	//***** End shadow rendering *****//
	renderContext.m_esm.PopRenderTarget();
	renderContext.m_esm.PopDepthStencilBuffer();
	renderContext.m_esm.PopViewport();
}

Tr2GpuResourcePool::Texture Tr2ShadowMap::DrawToShadowMapResult( Tr2RenderContext& renderContext, Tr2GpuResourcePool& gpuResourcePool, const Tr2TextureAL& depthMap, const Tr2TextureAL& cascadedShadowDepth, float upscaling )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto shadowMapResult = gpuResourcePool.GetTempTexture( "shadowMapResult", depthMap.GetWidth(), depthMap.GetHeight(), PixelFormat::PIXEL_FORMAT_R8_UNORM, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );

	GPU_REGION( renderContext, "Denoising" );
	renderContext.m_esm.PushRenderTarget( shadowMapResult );
	renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );

	m_shadowEffect->SetParameter( BlueSharedString( "EveSpaceSceneCascadedShadowMap" ), cascadedShadowDepth );
	m_shadowEffect->SetParameter( BlueSharedString( "DepthMap" ), depthMap );
	Tr2Renderer::DrawScreenQuad( renderContext, m_shadowEffect );
	m_shadowEffect->SetParameter( BlueSharedString( "EveSpaceSceneCascadedShadowMap" ), Tr2TextureAL{} );
	m_shadowEffect->SetParameter( BlueSharedString( "DepthMap" ), Tr2TextureAL{} );

	{
		CCP_STATS_ZONE( "DO_DENOISER" );
		if( m_denoiser && m_useDenoiser && depthMap.IsValid() )
		{
			shadowMapResult = m_denoiser->Apply( std::move( shadowMapResult ), depthMap, {}, Tr2Renderer::GetReversedDepthProjectionTransform(), upscaling, gpuResourcePool, renderContext );
		}
	}

	renderContext.m_esm.PopRenderTarget();
	renderContext.m_esm.PopDepthStencilBuffer();

	return shadowMapResult;
}

const unsigned int Tr2ShadowMap::GetShadowSplitCount() const
{
	return m_splitCount;
}

const unsigned int Tr2ShadowMap::GetShadowMapSize() const
{
	return m_size;
}

Tr2EffectPtr Tr2ShadowMap::GetShadowEffect() const
{
	return m_shadowEffect;
}

bool Tr2ShadowMap::GetDebugSplitValue() const
{
	return m_debugColorSplit;
}

uint32_t Tr2ShadowMap::GetDebugColors( int switchCase ) const
{
	uint32_t color;
	switch( switchCase )
	{
	case 0:
		// white
		color = 0xffffffff;
		break;
	case 1:
		// red
		color = 0xffff0000;
		break;
	case 2:
		//green
		color = 0xff00ff00;
		break;
	case 3:
		//blue
		color = 0xff0000ff;
		break;
	case 4:
		//yellow
		color = 0xffffff00;
		break;
	case 5:
		//purple
		color = 0xff00ffff;
		break;
	case 6:
		color = 0x2200ffff;
		break;
	case 7:
		color = 0xff555555;
		break;
	case 8:
		color = 0xff888888;
		break;
	}
	return color;
}

void Tr2ShadowMap::SetStaticShadowSplits()
{
	m_splitValues[0] = 25.f;
	m_splitValues[1] = 75.f;
	m_splitValues[2] = 150.f;
	m_splitValues[3] = 300.f;
	m_splitValues[4] = 600.f;
	m_splitValues[5] = 1200.f;
	m_splitValues[6] = 2400.f;
	m_splitValues[7] = 4800.f;
	m_splitValues[8] = 9600.f;
	m_splitValues[9] = 19200.f;
	m_splitValues[10] = 38400.f;
	m_splitValues[11] = 76800.f;
	m_splitValues[12] = 153600.f;
	m_splitValues[13] = 307200.f;
	m_splitValues[14] = 614400.f;
	m_splitValues[15] = 1228800.f;
}