#include "StdAfx.h"
#include "TriShadowMap.h"
#include "Tr2VariableStore.h"
#include "Tr2Renderer.h"
#include "TriFrustumOrtho.h"
#include "Shader/Tr2Effect.h"
#include "TriViewport.h"
#include "TriLineSet.h"

using namespace Tr2RenderContextEnum;

TriShadowMap::TriShadowMap( IRoot* lockobj ) :
	m_size( 1024 ),
	m_depthBias( 0.002f ),
	m_lightLeakStep( 0.5f ),
	m_enabled( true ),
	m_useBlankTexture( false ),
	m_useMips( false ),
	m_generateMips( false ),
	m_filterVsm( true ),
	m_lightDistance( 10000.0f ),
	m_debugFreezeObb( false ),
	m_shadowMapHandle( NULL ),
	m_shadowSizeHandle( NULL )
{
	// create some empty debug drawers
	m_lineSet.CreateInstance();

	m_filterBlurEffect.CreateInstance();
	m_filterBlurEffect->SetEffectPathName( "res:/graphics/effect/managed/space/system/shadowblur.fx" );

	PrepareResources();

	// register variable handle to texture
	m_shadowMapHandle    = GlobalStore().RegisterVariable( "EveSpaceSceneShadowMap", static_cast<ITr2TextureProvider*>( nullptr ) );
	m_shadowSizeHandle   = GlobalStore().RegisterVariable( "EveSpaceSceneShadowMapSettings", Vector4( 1.0f / m_size, 1.0f / m_size, m_depthBias, m_lightLeakStep ) );
	m_invInputSizeHandle = GlobalStore().RegisterVariable( "invTexelSize", Vector2( 0, 0 ) );

	// when adding this variable to the global store, we must re-init all already active shaders
	// so they get the shadowmap texture slot binding
	Tr2Renderer::ReinitializeRegisteredEffects();
}

TriShadowMap::~TriShadowMap()
{
	ReleaseResources( TRISTORAGE_ALL );
}

// --------------------------------------------------------------------------------
// Description:
//   Calculates shadow volume frustum given basic shadow parameters. This function
//   is thread-safe.
// Arguments:
//   lightDirection - direction of light
//   lightDistance - distance to the light source
//   receiverBoundingBox - bounding box of the shadow receiver
//   frustum - (out) shadow volume frustum
// --------------------------------------------------------------------------------
void TriShadowMap::CalculateShadowFrustum( 
	Vector3 lightDirection,
	float lightDistance,
	Obb receiverBoundingBox,
	TriFrustumOrtho& frustum )
{
	Vector3 targetPos = receiverBoundingBox.center;
	Vector3 lightViewPosition = targetPos - lightDistance * lightDirection;

    Vector3 vUp( 0.0f, 1.0f, 0.0f );
	if( fabs( lightDirection.y ) > 0.99f )
	{
		// Prevent singularity when light is directly above or below.
		vUp = Vector3( 0.0f, 0.0f, 1.0f );
	}

	Matrix lightView;
    D3DXMatrixLookAtRH( &lightView, &lightViewPosition, &targetPos, &vUp );

	//
	// Set up projection matrix.
	// First, move bounds into view space.
	// Find the m_boundsMin/Max by taking every corner of the OBB through lightView
	Vector3 minBounds, maxBounds;
	receiverBoundingBox.ComputeAABB( minBounds, maxBounds, lightView );

	// Next, set up an orthogonal projection using the bounds of the box.
	// bounds min is now the near plane - pull it in towards the camera
	maxBounds.z = minBounds.z / 1000000.0f;

	frustum.DeriveFrustum( lightView, minBounds, maxBounds );
}

// --------------------------------------------------------------------------------
// Description:
//   Empty
// --------------------------------------------------------------------------------
bool TriShadowMap::OnModified( Be::Var* value )
{
	if( ( (Be::Var*)&m_size == value ) || ( (Be::Var*)&m_enabled == value ) )
	{
		m_size = std::max( 1u, m_size );
		// if the size or state of the shadow map has changed we must nearly
		// re-create this whole shadow module
		ReleaseResources( TRISTORAGE_ALL );
		PrepareResources();

		if( m_shadowSizeHandle )
		{
			m_shadowSizeHandle->SetValue( Vector4( 1.0f / m_size, 1.0f / m_size, m_depthBias, m_lightLeakStep ) );
		}
	}
	if( (Be::Var*)&m_depthBias == value || (Be::Var*)&m_lightLeakStep == value )
	{
		if( m_shadowSizeHandle )
		{
			m_shadowSizeHandle->SetValue( Vector4( 1.0f / m_size, 1.0f / m_size, m_depthBias, m_lightLeakStep ) );
		}
	}
	return true;
}

void TriShadowMap::ClearVariableStore()
{
	if( m_shadowMapHandle )
	{
		m_shadowMapHandle->Clear();
	}
}

void TriShadowMap::ReleaseResources( TriStorage s )
{
	// handles
	if( m_shadowMapHandle )
	{
		m_shadowMapHandle->Clear();
	}
	// textures
	m_noShadowTexture.Destroy();
	m_shadowMapRT.Destroy();
	m_shadowMapDS.Destroy();
	m_filterBlurRT.Destroy();
}

#ifdef TRINITYDEV
void TriShadowMap::GetDescription( std::string& desc )
{
	char buffer[64];
	sprintf_s( buffer, sizeof(buffer), "TriShadowMap %u", m_size );
	desc = std::string( buffer );
}
#endif

void TriShadowMap::SetLightDirection( const Vector3& dir )
{
	m_lightDirection = dir;
}

void TriShadowMap::SetLightDistance( float dist )
{
	m_lightDistance = dist;
}

void TriShadowMap::SetReceiverObb( const Obb &obb )
{
	if( !m_debugFreezeObb )
	{
		m_receiverObb = obb;
	}
}

bool TriShadowMap::OnPrepareResources()
{
	// check for shadowmap formats, 32bit is really fine, if not supported, use 16bit

	PixelFormat pixelFormat = PIXEL_FORMAT_R32G32_FLOAT;
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !renderContext.IsSupportedRenderTargetFormat( pixelFormat, true ) )
	{
		pixelFormat = PIXEL_FORMAT_R16G16_UNORM;
		if( !renderContext.IsSupportedRenderTargetFormat( pixelFormat ) )
		{
			// it makes no sense to go with 8bit...
			return false;
		}
	}

	// shadowmap size check
	if( m_size == 0 )
	{
		return true;
	}

    //
	// Create the shadow map texture.
	//
	uint32_t mipCount = 1;
	if( m_useMips )
	{
		mipCount = 0;
	}

	if( !m_shadowMapRT.IsValid() )
	{
		CR_RETURN_VAL( 
				m_shadowMapRT.Create(	m_size, 
										m_size, 
										mipCount, 
										pixelFormat, 
										renderContext )
				, false );
		
		if( m_filterVsm && !m_filterBlurRT.IsValid() )
		{
			CR_RETURN_VAL( 
				m_filterBlurRT.Create(	m_size, 
										m_size, 
										1, 
										pixelFormat, 
										renderContext )
				, false );
		}

		// not an RT, just a texture, so should be available as 2x32 bit float.
		float pixels[2*2*2];
		for( unsigned i = 0; i != 8; ++i )
		{
			pixels[i] = 1.0f;
		}
		
		Tr2SubresourceData init = { pixels, 16, 32 };
		CR_RETURN_VAL( m_noShadowTexture.Create2D( 2, 2, 1, PIXEL_FORMAT_R32G32_FLOAT, USAGE_IMMUTABLE, &init, renderContext ), false );
	}

	if( !m_shadowMapDS.IsValid() )
	{
		CR_RETURN_VAL( 
				m_shadowMapDS.Create(	m_size, 
										m_size, 
										DSFMT_AUTO, 
										0, 
										0, 
										renderContext )
				, false );
	}

	if( m_shadowSizeHandle )
	{
		m_shadowSizeHandle->SetValue( Vector4( 1.0f / m_size, 1.0f / m_size, m_depthBias, m_lightLeakStep ) );
	}

	return true;
}

Tr2TextureAL* TriShadowMap::GetTexture()
{
	if( m_useBlankTexture || !m_enabled )
	{
		return &m_noShadowTexture;
	}
	else
	{
		return &m_shadowMapRT.GetTexture();
	}
}

void TriShadowMap::SetShadowTexture( bool useBlankTexture )
{
	if( m_shadowMapHandle )
	{
		m_useBlankTexture = useBlankTexture;
		m_shadowMapHandle->SetValue( this );
	}
}

bool TriShadowMap::BeginShadowRendering( Vector3& lightViewPosition, Matrix& lightView, Matrix& lightViewProj, Tr2RenderContext& renderContext )
{
	if( !m_shadowMapRT.IsValid() )
	{
		return false;
	}

	// empty our debug drawer here at shadow rendering beginning
	m_lineSet->Clear();

	// set 0 to all texture stages, so we the shaodwmap is not bound any more
	renderContext.m_esm.UnsetAllTextures();

	// Set aside the current render target and depth buffer and set
	// up the ones to use for generating the shadow map
	Tr2Renderer::PushViewport();
	Tr2Renderer::PushRenderTarget( m_shadowMapRT, renderContext );
	Tr2Renderer::PushDepthStencilBuffer( m_shadowMapDS, renderContext );

	// viewport is always quadratic
	Tr2Renderer::SetViewport( m_size, m_size, 0, 0, 0.f, 1.f );

	CR( renderContext.Clear( CLEARFLAGS_TARGET | CLEARFLAGS_ZBUFFER, 0xffffffff, 1, 0 ) );

	Vector3 targetPos = m_receiverObb.center;
	lightViewPosition = targetPos - m_lightDistance * m_lightDirection;

	// debug display some stuff
	if( m_lineSet )
	{
		m_lineSet->Add( lightViewPosition, 0xff00ff00, targetPos, 0xffff00ff );
	}

    Vector3 vUp( 0.0f, 1.0f, 0.0f );
	if( fabs( m_lightDirection.y ) > 0.99f )
	{
		// Prevent singularity when light is directly above or below.
		vUp = Vector3( 0.0f, 0.0f, 1.0f );
	}

    D3DXMatrixLookAtRH( &lightView, &lightViewPosition, &targetPos, &vUp );

	//
	// Set up projection matrix.
	// First, move bounds into view space.
	// Find the m_boundsMin/Max by taking every corner of the OBB through lightView
	m_receiverObb.ComputeAABB( m_boundsMin, m_boundsMax, lightView );

	m_receiverLightAabbMin = m_boundsMin;
	m_receiverLightAabbMax = m_boundsMax;
	
	// Next, set up an orthogonal projection using the bounds of the box.
	// bounds min is now the near plane - pull it in towards the camera
	m_boundsMax.z = m_boundsMin.z / 1000000.0f;

	Matrix proj;

	D3DXMatrixOrthoRH( &proj, m_boundsMax.x - m_boundsMin.x, m_boundsMax.y - m_boundsMin.y, -m_boundsMax.z, -m_boundsMin.z );

	D3DXMatrixMultiply( &lightViewProj, &lightView, &proj );

	return true;
}

void TriShadowMap::EndShadowRendering()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( m_filterVsm && m_filterBlurRT.IsValid() && m_invInputSizeHandle )
	{
		Tr2Renderer::SetDepthStencilBuffer( nullDS, renderContext );

		CTriViewport viewport0;
		viewport0.x = 0;
		viewport0.y = 0;
		viewport0.width = m_size;
		viewport0.height = m_size;
		Tr2Renderer::SetViewport( viewport0 );

		renderContext.m_esm.UnsetAllTextures();
		m_invInputSizeHandle->SetValue( Vector2( 1.0f / m_size, 0 ) );
		Tr2Renderer::SetRenderTarget( 0, m_filterBlurRT, renderContext );
		Tr2Renderer::DrawTexture( m_filterBlurEffect, m_shadowMapRT.GetTexture() );

		renderContext.m_esm.UnsetAllTextures();
		m_invInputSizeHandle->SetValue( Vector2( 0, 1.0f / m_size ) );
		Tr2Renderer::SetRenderTarget( 0, m_shadowMapRT, renderContext );
		Tr2Renderer::DrawTexture( m_filterBlurEffect, m_filterBlurRT.GetTexture() );
	}

	m_shadowMapRT.GenerateMipMaps( renderContext );

	Tr2Renderer::PopRenderTarget( renderContext );
	Tr2Renderer::PopDepthStencilBuffer( renderContext );
	Tr2Renderer::PopViewport();
}

void TriShadowMap::DrawDebugInfo()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( m_lineSet )
	{
		const unsigned color = 0xff80ffff;
		m_lineSet->Add( m_receiverObb.GetPoint( 0 ), color, m_receiverObb.GetPoint( 2 ), color );
		m_lineSet->Add( m_receiverObb.GetPoint( 1 ), color, m_receiverObb.GetPoint( 3 ), color );
		m_lineSet->Add( m_receiverObb.GetPoint( 4 ), color, m_receiverObb.GetPoint( 6 ), color );
		m_lineSet->Add( m_receiverObb.GetPoint( 5 ), color, m_receiverObb.GetPoint( 7 ), color );

		m_lineSet->Add( m_receiverObb.GetPoint( 0 ), color, m_receiverObb.GetPoint( 4 ), color );
		m_lineSet->Add( m_receiverObb.GetPoint( 1 ), color, m_receiverObb.GetPoint( 5 ), color );
		m_lineSet->Add( m_receiverObb.GetPoint( 2 ), color, m_receiverObb.GetPoint( 6 ), color );
		m_lineSet->Add( m_receiverObb.GetPoint( 3 ), color, m_receiverObb.GetPoint( 7 ), color );

		m_lineSet->Add( m_receiverObb.GetPoint( 0 ), color, m_receiverObb.GetPoint( 1 ), color );
		m_lineSet->Add( m_receiverObb.GetPoint( 2 ), color, m_receiverObb.GetPoint( 3 ), color );
		m_lineSet->Add( m_receiverObb.GetPoint( 4 ), color, m_receiverObb.GetPoint( 5 ), color );
		m_lineSet->Add( m_receiverObb.GetPoint( 6 ), color, m_receiverObb.GetPoint( 7 ), color );
		
		m_lineSet->Render( renderContext );
	}
}

void TriShadowMap::GetBounds( Vector3& minBounds, Vector3& maxBounds )
{
	minBounds = m_boundsMin;
	maxBounds = m_boundsMax;
}

void TriShadowMap::GetReceiverLightAabb( Vector3& min, Vector3& max )
{
	min = m_receiverLightAabbMin;
	max = m_receiverLightAabbMax;
}

Vector4 TriShadowMap::GetShadowMapSettings() const
{
	Vector4 settings;
	m_shadowSizeHandle->GetValue( settings );
	return settings;
}