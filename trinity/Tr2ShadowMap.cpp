////////////////////////////////////////////////////////////
//
//    Created:   October 2022
//    Copyright: CCP 2022
//

#include "StdAfx.h"
#include "Tr2ShadowMap.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2EffectStateManager.h"
#include "TriRenderBatch.h"
#include "Tr2RenderTarget.h"
#include "Tr2TextureArray.h"
#include "Resources/TriTextureRes.h"

namespace
{

struct DepthSizeHash
{
	size_t operator()( const std::pair<uint32_t, uint32_t>& ds ) const
	{
		return std::hash<uint32_t>()( ds.first ) ^ std::hash<uint32_t>()( ds.second );
	}
};

std::unordered_map<std::pair<uint32_t, uint32_t>, BlueWeakRef<Tr2DepthStencil>, DepthSizeHash> s_sharedDepthMaps;

Tr2DepthStencilPtr GetShadowAtlas( uint32_t width, uint32_t height )
{
	Tr2DepthStencilPtr result;
	auto& found = s_sharedDepthMaps[{ width, height }];
	if( !found )
	{
		result.CreateInstance();
		found = result.p;
	}
	else
	{
		result = found;
	}
	if( !result->IsValid() )
	{
		result->Create( width, height, Tr2RenderContextEnum::DSFMT_D32F, 1, 0, Tr2RenderContextEnum::EX_NONE );
	}
	return result;
}

}


using namespace Tr2RenderContextEnum;
Tr2ShadowMap::Tr2ShadowMap( IRoot* lockobj ) :
	m_size( SHADOW_MAP_SIZE ),
	m_height( 2 ),
	m_width( 8 ),
	m_splitCount( SHADOW_FRUSTUM_COUNT ),
	m_disableShimmer( true ),
	m_oldZFar( 0.0 ),
	m_shadowMapHandle( NULL ),
	m_cascadedShadowMapHandle( NULL ),
	m_useDenoiser( true ),
	m_debugColorSplit( false )
{
	m_shadowEffect.CreateInstance();
	m_shadowEffect->SetEffectPathName( "res:/graphics/effect/managed/space/system/ShadowDepth.fx" );
	m_shadowMapHandle = GlobalStore().RegisterVariable( "EveSpaceSceneShadowMap", static_cast<ITr2TextureProvider*>( nullptr ) );
	m_cascadedShadowMapHandle = GlobalStore().RegisterVariable( "EveSpaceSceneCascadedShadowMap", static_cast<ITr2TextureProvider*>( nullptr ) );

	m_cascadedShadowMapDS = GetShadowAtlas( m_size * m_width, m_size * m_height );

	m_whiteTexture.CreateInstance();
	m_denoiser.CreateInstance();
	
	PrepareResources();

	SetSplitValues();
}

Tr2ShadowMap::~Tr2ShadowMap()
{
	ReleaseResources( TRISTORAGE_ALL );
}

void Tr2ShadowMap::Setup( uint32_t elementSize, uint32_t elementCount, bool useDenoiser )
{
	if( m_size != elementSize || m_splitCount != elementCount )
	{
		m_size = elementSize;
		m_splitCount = elementCount;
		ReleaseResources( TRISTORAGE_ALL );
		PrepareResources();
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

	if( IsMatch( value, m_size ) )
	{
		ReleaseResources( TRISTORAGE_ALL );
		PrepareResources();
	}

	return true;
}

void Tr2ShadowMap::ReleaseResources( TriStorage s )
{
	if( m_cascadedShadowMapHandle )
	{
		m_cascadedShadowMapHandle->Clear();
	}

	if( m_shadowMapHandle )
	{
		m_shadowMapHandle->Clear();
	}

	m_cascadedShadowMapDS = Tr2DepthStencilPtr();
	m_shadowMapResultRT = Tr2RenderTargetPtr();

	m_denoiser = nullptr;

	SetBlankTexture();
}


void Tr2ShadowMap::ClearVariableStore()
{
	if( m_cascadedShadowMapHandle )
	{
		m_cascadedShadowMapHandle->Clear();
	}

	if( m_shadowMapHandle )
	{
		m_shadowMapHandle->Clear();
	}
}

bool Tr2ShadowMap::OnPrepareResources()
{
	if( m_size == 0 )
	{
		return true;
	}

	m_perSplitData.SplitInfo.x = float( m_splitCount );

	if( !m_cascadedShadowMapDS )
	{
		m_cascadedShadowMapDS = GetShadowAtlas( m_size * m_width, m_size * m_height );
	}

	if( !m_whiteTexture->IsValid() )
	{
		BeResMan->GetResource( "res:/texture/global/white.dds", "", m_whiteTexture );
	}

	SetBlankTexture();

	return true;
}

void Tr2ShadowMap::SetBlankTexture()
{
	if( m_shadowMapHandle )
	{
		m_shadowMapHandle->SetValue( m_whiteTexture );
	}
}

void Tr2ShadowMap::ShouldUseDenoiser( bool val )
{
	m_useDenoiser = val;
}

// --------------------------------------------------------------------------------
// Description:
//  Go through all i count of frustum splits. Calculate the corresponding 
//	bounding box based on zNear and zFar values.
// --------------------------------------------------------------------------------
ShadowMap::SplitSetup Tr2ShadowMap::SetupShadowSplit( int splitIndex, Matrix invViewTransform, const Vector3 lightDirection, 
	float zNear, float leftDivNear, float rightDivNear, float topDivNear, float bottomDivNear )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	ShadowMap::SplitSetup splitSetup;

	if(splitIndex == 0 )
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

	AxisAlignedBoundingBox aabb;

	Vector3 corners[8];

	// Now transform the unit cube based off matrices
	for( unsigned int i = 0; i < 8; ++i )
	{
		Vector3 vertex = m_unitCube[i];
		// view space
		Vector4 transformedVertex = Transform( Vector4( vertex, 1.0 ), Inverse( projection ) );

		transformedVertex /= transformedVertex.w;

		// world space
		transformedVertex = Transform( transformedVertex, invViewTransform );
		corners[i] = TransformCoord( transformedVertex.GetXYZ(), ( lightView ) );
		// light view space
		aabb.IncludePoint( TransformCoord( transformedVertex.GetXYZ(), ( lightView ) ) );
	}

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
		float texelSize = (radius * 2.0f) / m_size;
		center.x = std::floor( center.x / texelSize + 0.5f ) * texelSize;
		center.y = std::floor( center.y / texelSize + 0.5f ) * texelSize;

		aabb = AxisAlignedBoundingBox( center - Vector3( radius, radius, radius ), center + Vector3( radius, radius, radius ) );
	}

	// pull the aabb towards the sun
	//aabb.m_max.z += 250000.f;
	splitSetup.aabb = aabb;
	
	splitSetup.lightViewProjection = lightView * OrthoOffCenterMatrix( aabb.m_max.x, aabb.m_min.x, aabb.m_max.y, aabb.m_min.y, -aabb.m_max.z, -aabb.m_min.z );

	m_perSplitData.CascadeDepthRanges[splitIndex / 4][splitIndex % 4] = aabb.m_max.z - aabb.m_min.z;

	// 4th element of shadowMatrix is always the same
	m_perSplitData.ShadowMatrixVal[splitIndex] = Transpose( splitSetup.lightViewProjection );
	
	// create shadow frustum out from lightView, aabb.min, aabb.max
	TriFrustumOrtho shadowFrustum;
	shadowFrustum.DeriveFrustum( lightView, aabb.m_min, aabb.m_max );
	splitSetup.shadowFrustum = shadowFrustum;
	
	return splitSetup;
}

bool Tr2ShadowMap::PrepareShadowRendering( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_cascadedShadowMapDS || m_cascadedShadowMapDS->GetTexture() == nullptr )
	{
		// This could happen if device is lost
		return false;
	}

	// Using depth stencil as shadow map
	renderContext.m_esm.PushViewport();
	renderContext.m_esm.PushRenderTarget( Tr2TextureAL() ); //empty texture
	renderContext.m_esm.PushDepthStencilBuffer( *m_cascadedShadowMapDS->GetTexture() );

	renderContext.m_esm.UpdateRenderTargetViewport( m_cascadedShadowMapDS->GetWidth(), m_cascadedShadowMapDS->GetHeight() );

	// we want a clean depth buffer for this
	CR( renderContext.Clear( CLEARFLAGS_ZBUFFER, 0xffffffff, 1, 0 ) );

	renderContext.SetReadOnlyDepth( false );

	return true;
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

	if( m_cascadedShadowMapDS->IsValid() )
	{
		m_cascadedShadowMapHandle->SetValue( m_cascadedShadowMapDS );
	}
}

void Tr2ShadowMap::DrawToShadowMapResult( Tr2RenderContext& renderContext, ITr2TextureProvider* depthMap, float upscaling )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto width = Tr2Renderer::GetViewport().width;
	auto height = Tr2Renderer::GetViewport().height;
	bool viewportChanged = false;

	if( m_shadowMapResultRT )
	{
		viewportChanged = m_shadowMapResultRT->GetWidth() != uint32_t( width ) || m_shadowMapResultRT->GetHeight() != uint32_t( height );
	}

	if( !m_shadowMapResultRT || !m_shadowMapResultRT->IsValid() || viewportChanged )
	{
		m_shadowMapResultRT = nullptr;
		m_shadowMapResultRT.CreateInstance();
		m_shadowMapResultRT->SetName("shadow_map_rt");
		m_shadowMapResultRT->Create( width, height, 1, PixelFormat::PIXEL_FORMAT_R8_UNORM );
	}

	if( m_shadowMapResultRT || m_shadowMapResultRT->IsValid() )
	{
		GPU_REGION( renderContext, "Denoising" );
		renderContext.m_esm.PushRenderTarget( *m_shadowMapResultRT );
		renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );

		Tr2Renderer::DrawTexture( renderContext, m_shadowEffect, m_shadowMapResultRT->GetRenderTarget() );

		{
			CCP_STATS_ZONE( "DO_DENOISER" );
			if( m_denoiser && m_useDenoiser && depthMap && depthMap->GetTexture()->IsValid() )
			{
				m_denoiser->Apply( *m_shadowMapResultRT, *depthMap, NULL, Tr2Renderer::GetReversedDepthProjectionTransform(), upscaling, renderContext );
			}
		}

		if( m_shadowMapHandle )
		{
			if( m_denoiser && m_useDenoiser )
			{
				m_shadowMapHandle->SetValue( m_denoiser->GetTexture() );
			}
			else
			{
				m_shadowMapHandle->SetValue( m_shadowMapResultRT );
			}
		}

		renderContext.m_esm.PopRenderTarget();
		renderContext.m_esm.PopDepthStencilBuffer();
	}
}

void Tr2ShadowMap::SetShadowMap( Tr2RenderTargetPtr shadowMapRenderTarget )
{
	if( m_shadowMapHandle )
	{
		m_shadowMapHandle->SetValue( shadowMapRenderTarget );
	}
}

Tr2DepthStencilPtr Tr2ShadowMap::GetCascadedShadowMapDS() const
{
	return m_cascadedShadowMapDS;
}

Tr2RenderTargetPtr Tr2ShadowMap::GetCascadedShadowMapRT() const
{
	return m_shadowMapResultRT;
}

ITr2TextureProvider* Tr2ShadowMap::GetShadowMap() const
{
	if( m_denoiser )
	{
		return m_denoiser->GetTexture();
	}
	else
	{
		return m_shadowMapResultRT;
	}
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

void Tr2ShadowMap::SetSplitValues()
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

