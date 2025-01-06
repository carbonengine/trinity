#include "StdAfx.h"
#include "Tr2RaytracingManager.h"
#include "Shader/Tr2Shader.h"
#include "Resources/TriTextureRes.h"
#include "ITr2TextureProvider.h"
#include "Tr2RenderTarget.h"
#include "Tr2Renderer.h"

namespace
{
	// for shadow scene
	struct ShadowPerFrameData
	{
		Matrix projectionInverse;
		Matrix viewInverse;

		Vector3 sunDirection;
		float sunAngle;

		CcpMath::Sphere planets[2];

		Vector2 resolution;
		uint32_t frameIndex;
	};

	const BlueSharedString RtShadowTechniqueName = BlueSharedString( "RtShadow" );
	const BlueSharedString RtShadowMapTechniqueName = BlueSharedString( "RtShadowShadowDest" );
	const BlueSharedString NormalBufferTechniqueName = BlueSharedString( "RtShadowNormalBuffer" );
	const BlueSharedString RtSceneTechniqueName = BlueSharedString( "RtShadowScene" );
}

//***********************************************************
// Tr2RaytracingManager
//***********************************************************

Tr2RaytracingManager::Tr2RaytracingManager( IRoot* lockobj ) :
	m_sunAngle( 0.01 ),
	m_applyDenoiser( true )
{
	m_geometry.CreateInstance();
	m_destTex.CreateInstance();

	m_shadowEffect.CreateInstance();
	m_shadowEffect->SetEffectPathName( "res:/graphics/effect/managed/space/system/raytracing/rtshadows.fx" );

	m_denoiser.CreateInstance();
	m_denoiser->SetRadius( 3 );

	m_whiteTexture.CreateInstance();
}

Tr2RaytracingManager::~Tr2RaytracingManager()
{
	ReleaseResources( TRISTORAGE_ALL );
}

Tr2RaytracingGeometry& Tr2RaytracingManager::GetGeometry()
{
	return *m_geometry;
}

bool Tr2RaytracingManager::OnPrepareResources()
{
	return true;
}

void Tr2RaytracingManager::ReleaseResources( TriStorage s )
{
	m_geometry->ReleaseResources( s );

	m_geometry = nullptr;
	m_shadowEffect = nullptr;
	m_destTex = Tr2RenderTargetPtr();
	m_denoiser = nullptr;

	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		m_shadowPerFrameData = Tr2ConstantBufferAL();
	}
}

ITr2TextureProvider* Tr2RaytracingManager::GetShadowMap() const
{
	if( m_denoiser )
	{
		return m_denoiser->GetTexture();
	}
	else
	{
		return m_destTex;
	}
}

void Tr2RaytracingManager::RenderShadows( ITr2TextureProvider* depth, ITr2TextureProvider* normal, const Vector3& sunDirection, const CcpMath::Sphere* planets, size_t planetCount, Tr2RenderContext& renderContext )
{
	renderContext.AddGpuMarker( __FUNCTION__ );
	GPU_REGION( renderContext, "Raytraced shadows" );
	CCP_STATS_ZONE( __FUNCTION__ );

	auto depthTex = depth->GetTexture();
	if( !depthTex )
	{
		return;
	}

	if( !m_destTex->IsValid() || m_destTex->GetWidth() != depthTex->GetWidth() || m_destTex->GetHeight() != depthTex->GetHeight() )
	{
		// Create the output resource. The dimensions and format should match the swap-chain
		if( FAILED( m_destTex->Create( depthTex->GetWidth(), depthTex->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM, 1, 0, Tr2RenderContextEnum::EX_BIND_UNORDERED_ACCESS ) ) )
		{
			return;
		}
		m_destTex->SetName( "raytracing_shadow_dest" );
	}

	// texture uav
	m_shadowEffect->SetParameter( RtShadowMapTechniqueName, m_destTex );
	m_shadowEffect->SetParameter( NormalBufferTechniqueName, normal );

	// scene srv
	m_shadowEffect->SetParameter( RtSceneTechniqueName, m_geometry );

	if( !m_shadowEffect->GetEffectRes() || !m_shadowEffect->GetEffectRes()->IsGood() )
	{
		return;
	}

	if( !m_geometry->HasGeometry() )
	{
		return;
	}

	uint32_t techniqueIndex;
	if( !m_shadowEffect->GetShaderStateInterface()->GetTechniqueIndex( RtShadowTechniqueName, techniqueIndex ) )
	{
		return;
	}

	std::wstring rayGenName, missName;
	m_pipelineManager.AddLibrary( rayGenName, missName, m_shadowEffect, RtShadowTechniqueName );

	auto pipelineState = m_pipelineManager.GetPipelineState( renderContext );

	if( !pipelineState.IsValid() )
	{
		return;
	}

	{
		CCP_STATS_ZONE( "Create shader table" );
		m_shaderTableDesc.AddRayGenShader( rayGenName.c_str() );
		m_shaderTableDesc.AddMissShader( missName.c_str() );
		m_shadowShaderTable.Create( m_shaderTableDesc, pipelineState, renderContext.GetPrimaryRenderContext() );
	}

	if( !m_shadowPerFrameData.IsValid() )
	{
		m_shadowPerFrameData.Create( sizeof( ShadowPerFrameData ), renderContext.GetPrimaryRenderContext() );
	}

	auto destTex = m_destTex->GetTexture();
	{
		ShadowPerFrameData* data;
		m_shadowPerFrameData.Lock( reinterpret_cast<void**>(&data), renderContext );
		data->projectionInverse = Transpose( Inverse( Tr2Renderer::GetReversedDepthProjectionTransform() ) );
		data->viewInverse = Transpose( Tr2Renderer::GetInverseViewTransform() );

		data->sunDirection = Normalize( sunDirection );
		data->sunAngle = m_sunAngle / 180.f * XM_PI;

		std::fill( std::begin( data->planets ), std::end( data->planets ), CcpMath::Sphere( Vector3( 0, 0, 0 ), 0 ) );

		for( size_t i = 0; i < std::min( planetCount, sizeof( data->planets ) / sizeof( data->planets[0] ) ); ++i )
		{
			data->planets[i] = planets[i];
		}

		data->resolution = Vector2( float( destTex->GetWidth() ), float( destTex->GetHeight() ) );
		data->frameIndex = uint32_t( Tr2Renderer::GetCurrentFrameCounter() & 0xffffffff );

		m_shadowPerFrameData.Unlock( renderContext );
	}

	const float clearValue[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	renderContext.ClearUav( *destTex, 0, clearValue );

	if( m_shadowShaderTable.IsValid() )
	{
		m_shadowEffect->ApplyMaterialDataForRtState( techniqueIndex, pipelineState, renderContext );

		renderContext.UseAccelerationStructure( m_geometry->GetTLAS() );

		renderContext.SetConstants( m_shadowPerFrameData, Tr2RenderContextEnum::COMPUTE_SHADER, 2 );
		renderContext.DispatchRays( pipelineState, m_shadowShaderTable, rayGenName.c_str(), destTex->GetWidth(), destTex->GetHeight(), 1 );
	}
	if( m_denoiser && m_applyDenoiser )
	{
		m_denoiser->Apply( *m_destTex, *depth, NULL, Tr2Renderer::GetReversedDepthProjectionTransform(), renderContext );
		GlobalStore().RegisterVariable( "EveSpaceSceneShadowMap", m_denoiser->GetTexture() );
	}
	else
	{
		GlobalStore().RegisterVariable( "EveSpaceSceneShadowMap", m_destTex );
	}
}
