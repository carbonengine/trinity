// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2ReflectionProbe.h"
#include "Tr2Renderer.h"
#include "Tr2TextureReference.h"
#include "Tr2RenderTarget.h"
#include "Shader/Tr2Effect.h"
#include "Resources/TriTextureRes.h"
#include "Shader/Parameter/Tr2RuntimeTextureParameter.h"
#include <sstream>

using namespace Tr2RenderContextEnum;

const unsigned MIP_COUNT = 7;
const unsigned FILTER_SIZE = 128;
const unsigned FILTER_GROUP_DIM = 342; // ceil(sum(2**(x+x) for x in range(1, MIP_COUNT + 1)) / GROUP_SIZE)

namespace
{

const auto s_backLightColorName = BlueSharedString( "BackLightColor" );
const auto s_backLightContrastName = BlueSharedString( "BackLightContrast" );
const auto s_viewDirectionName = BlueSharedString( "ViewDirection" );

}

Tr2ReflectionProbe::Tr2ReflectionProbe( IRoot* lockobj ) :
	m_initialized( false ),
	m_hasData( false ),
	m_position( 0, 0, 0 ),
	m_intermediateSize( FILTER_SIZE * 4 ),
	m_prevCullInversion( false ),
	m_customSourceTexture(),
	m_renderFrequency( ONE_SIDE_PER_FRAME ),
	m_currentFrame( 0 ),
	m_onePassDone( false ),
	m_hdrOutput( true ),
	m_hollywoodMode( true ),
	m_lockPosition( false ),
	m_backlightColor( 1, 1, 1, 1 ),
	m_backlightContrast( 16 )
{
	for( unsigned i = 0; i < 6; i++ )
	{
		m_renderTargets[i].CreateInstance();
	}

	m_renderTargetCube.CreateInstance();
	m_preFilterEffect.CreateInstance();
	m_copyMipEffect.CreateInstance();
	m_filterEffect.CreateInstance();
	m_preFilterTarget.CreateInstance();
	m_postFilterTarget.CreateInstance();
	PrepareResources();
}

Tr2ReflectionProbe::~Tr2ReflectionProbe()
{
	ReleaseResources( TRISTORAGE_ALL );
}

bool Tr2ReflectionProbe::IsValid()
{
	return PrepareResources();
}

bool Tr2ReflectionProbe::HasData() const
{
	return m_hasData && m_postFilterTarget->IsValid();
}

TriFrustum Tr2ReflectionProbe::GetFrustum( unsigned face, Tr2RenderContext& renderContext )
{
	TriFrustum frustum = TriFrustum();
	frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &m_position, &Tr2Renderer::GetProjectionTransform(), renderContext.m_esm.GetViewport() );
	return frustum;
}

uint8_t Tr2ReflectionProbe::GetStartFace() const
{
	if( m_renderFrequency == ALL_SIDES_PER_FRAME || !m_onePassDone )
	{
		return 0;
	}
	return m_currentFrame;
}

uint8_t Tr2ReflectionProbe::GetEndFace() const
{
	if( m_renderFrequency == ALL_SIDES_PER_FRAME || !m_onePassDone )
	{
		return 6;
	}
	return m_currentFrame + 1;
}


void Tr2ReflectionProbe::InitRenderPass( Tr2RenderContext& renderContext )
{
	if( !m_lockPosition )
	{
		m_position = Tr2Renderer::GetViewPosition();
	}

	renderContext.m_esm.PushViewport();
	Tr2Renderer::PushViewTransform();
	Tr2Renderer::PushProjection();
	renderContext.m_esm.PushRenderTarget();
	renderContext.m_esm.PushDepthStencilBuffer();

	renderContext.m_esm.SetViewport( m_intermediateSize, m_intermediateSize, 0, 0, 0, 1 );

	// Square projection matrix, with near and far clip planes from the current projection
	Matrix newProjection = IdentityMatrix();
	const Matrix& currentProjection = Tr2Renderer::GetProjectionTransform();
	newProjection.m[2][2] = currentProjection.m[2][2];
	newProjection.m[2][3] = -1.0f;
	newProjection.m[3][2] = currentProjection.m[3][2];
	newProjection.m[3][3] = 0.0f;

	Tr2Renderer::SetProjectionTransform( newProjection );

	// We need to invert cull-mode because of the left-handed coordinates of the cube rendertarget
	m_prevCullInversion = renderContext.m_esm.IsCullModeInverted();
	renderContext.m_esm.SetInvertedCullMode( true );

#if !TRINITY_PLATFORM_SUPPORTS_RENDER_PASS_HINTS
	{
		GPU_REGION( renderContext, "Reflection Depth Clear" );

		for( int i = GetStartFace(); i < GetEndFace(); i++ )
		{
			renderContext.SetRenderTarget( *m_renderTargets[i], 0 );
			renderContext.m_esm.SetDepthStencilBuffer( m_stencilMaps[i] );
			CR( renderContext.Clear( CLEARFLAGS_ZBUFFER | CLEARFLAGS_TARGET, 0, 0, 0 ) );
		}
	}
#endif
}

Tr2TextureAL Tr2ReflectionProbe::GetDepthBuffer( unsigned face )
{
	return m_stencilMaps[face];
}

void Tr2ReflectionProbe::StartRenderFace( unsigned face, Tr2RenderContext& renderContext )
{
	renderContext.RenderPassHint( { Tr2LoadAction::CLEAR, Tr2StoreAction::STORE, 0 }, { Tr2LoadAction::CLEAR, Tr2StoreAction::DONT_CARE, 0.F } );

	renderContext.m_esm.SetRenderTarget( 0, *m_renderTargets[face] );
	renderContext.m_esm.SetDepthStencilBuffer( m_stencilMaps[face] );

	static const Vector3 faceDirections[] = {
		Vector3( 0, 1, 0 ), Vector3( 1, 0, 0 ), //+X
		Vector3( 0, 1, 0 ),
		Vector3( -1, 0, 0 ), //-X
		Vector3( 0, 0, -1 ),
		Vector3( 0, 1, 0 ), //+Y
		Vector3( 0, 0, 1 ),
		Vector3( 0, -1, 0 ), //-Y
		Vector3( 0, 1, 0 ),
		Vector3( 0, 0, 1 ), //+Z
		Vector3( 0, 1, 0 ),
		Vector3( 0, 0, -1 ), //-Z
	};

	// We need to invert the X-axis because of the left-handed coordinates of the cube rendertarget
	static const Matrix xInverter = Matrix(
		-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );

	const Vector3& up = faceDirections[face * 2];
	const Vector3& at = faceDirections[face * 2 + 1];
	Tr2Renderer::SetViewTransform( LookAtMatrix( m_position, m_position + at, up ) * xInverter );
}

void Tr2ReflectionProbe::EndRenderPass( Tr2RenderContext& renderContext )
{
	{
		GPU_REGION( renderContext, "Reflection RT Copy" );

		for( int i = GetStartFace(); i < GetEndFace(); i++ )
		{
			CR( m_renderTargetCube->GetRenderTarget().CopySubresourceRegion( Tr2TextureSubresource( i, 0 ), *m_renderTargets[i], Tr2TextureSubresource( 0, 0 ), renderContext ) );
		}
	}

	renderContext.m_esm.SetInvertedCullMode( m_prevCullInversion );

	renderContext.m_esm.PopDepthStencilBuffer();
	renderContext.m_esm.PopRenderTarget();
	Tr2Renderer::PopProjection();
	Tr2Renderer::PopViewTransform();
	renderContext.m_esm.PopViewport();

	Filter( renderContext );
	m_hasData = true;
	m_onePassDone = true;

	if( m_renderFrequency != ALL_SIDES_PER_FRAME )
	{
		m_currentFrame = ( m_currentFrame + 1 ) % 6;
	}
}

Tr2RenderTargetPtr Tr2ReflectionProbe::GetReflection()
{
	return m_postFilterTarget;
}

void Tr2ReflectionProbe::SetBackLightColor( Color color )
{
	m_backlightColor = color;
}

void Tr2ReflectionProbe::SetBackLightContrast( float contrast )
{
	m_backlightContrast = contrast;
}

void Tr2ReflectionProbe::ReleaseResources( TriStorage s )
{
	m_initialized = false;
}

bool Tr2ReflectionProbe::OnPrepareResources()
{
	if( !SHADER_TYPE_EXISTS( COMPUTE_SHADER ) )
	{
		return false;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	auto rtFormat = renderContext.FormatIsUAVCompatibleDx12( DXGI_FORMAT_R11G11B10_FLOAT ) ? PIXEL_FORMAT_R11G11B10_FLOAT : PIXEL_FORMAT_R16G16B16A16_FLOAT;
#else
	auto rtFormat = PIXEL_FORMAT_R11G11B10_FLOAT;
#endif
	return DoPrepareResources( rtFormat, renderContext );
}

bool Tr2ReflectionProbe::DoPrepareResources( ImageIO::PixelFormat rtFormat, Tr2PrimaryRenderContext& renderContext )
{

	for( int i = 0; i < 6; i++ )
	{
		if( !m_renderTargets[i]->IsValid() )
		{
			CR_RETURN_VAL( m_renderTargets[i]->CreateManual( m_intermediateSize, m_intermediateSize, 1, rtFormat, 0, 0, EX_BIND_UNORDERED_ACCESS, TEX_TYPE_2D, Tr2CpuUsage::NONE, Tr2GpuUsage::RENDER_TARGET ), false );
		}

		if( !m_stencilMaps[i].IsValid() )
		{
			int stencilSize = ( m_customSourceTexture && m_customSourceTexture->IsValid() ) ? m_customSourceTexture->GetWidth() : m_intermediateSize;
			CR_RETURN_VAL( m_stencilMaps[i].Create( Tr2BitmapDimensions( stencilSize, stencilSize, 1, PIXEL_FORMAT_D32_FLOAT ), Tr2GpuUsage::DEPTH_STENCIL | Tr2GpuUsage::SHADER_RESOURCE, renderContext ), false );
		}
	}

	if( !m_renderTargetCube->IsValid() )
	{
		CR_RETURN_VAL( m_renderTargetCube->Create( m_intermediateSize, m_intermediateSize, 1, rtFormat, 0, 0, EX_BIND_UNORDERED_ACCESS, TEX_TYPE_CUBE ), false );
	}

	if( !m_preFilterTarget->IsValid() )
	{
		CR_RETURN_VAL( m_preFilterTarget->Create( FILTER_SIZE, FILTER_SIZE, 8, rtFormat, 0, 0, EX_BIND_UNORDERED_ACCESS, TEX_TYPE_CUBE ), false );
	}

	if( !m_postFilterTarget->IsValid() )
	{
		CR_RETURN_VAL( m_postFilterTarget->Create( FILTER_SIZE * 2, FILTER_SIZE * 2, MIP_COUNT + 1, m_hdrOutput ? rtFormat : PIXEL_FORMAT_R8G8B8A8_UNORM, 0, 0, EX_BIND_UNORDERED_ACCESS, TEX_TYPE_CUBE ), false );
		m_hasData = false;
	}

	if( !m_initialized )
	{
		m_filterEffect->SetEffectPathName( "res:/graphics/effect/managed/space/System/Reflection/ReflectionFilterActivision128.fx" );
		m_preFilterEffect->SetEffectPathName( "res:/graphics/effect/managed/space/System/Reflection/ReflectionFilterActivisionPre.fx" );
		auto source = dynamic_cast<ITr2TextureProvider*>( m_customSourceTexture.p );
		m_preFilterEffect->SetParameter( BlueSharedString( "tex_hi_res" ), source ? source : static_cast<ITr2TextureProvider*>( m_renderTargetCube ) );
		m_preFilterEffect->SetParameter( BlueSharedString( "tex_lo_res" ), m_preFilterTarget );

		m_preFilterEffect->SetOption( BlueSharedString( "HOLLYWOOD_MODE" ), BlueSharedString( m_hollywoodMode ? "HOLLYWOOD_ON" : "HOLLYWOOD_OFF" ) );
		if( m_hollywoodMode )
		{
			m_preFilterEffect->SetParameter( s_backLightColorName, Vector4( m_backlightColor ) );
			m_preFilterEffect->SetParameter( s_backLightContrastName, m_backlightContrast );
			m_preFilterEffect->SetParameter( s_viewDirectionName, Vector3( 0, 1, 0 ) );
		}

		m_filterEffect->SetParameter( BlueSharedString( "tex_in" ), m_preFilterTarget );

		for( int i = 0; i < MIP_COUNT; i++ )
		{
			std::stringstream str;
			str << "tex_out" << i;
			Tr2RuntimeTextureParameterPtr param;
			param.CreateInstance();
			param->Create( BlueSharedString( str.str() ), m_postFilterTarget, i + 1 );
			m_filterEffect->AddResource( param );
		}

		m_filterEffect->SetParameter( BlueSharedString( "output_srgb" ), m_hdrOutput ? 0.f : 1.f );

		m_copyMipEffect->SetEffectPathName( "res:/graphics/effect/managed/space/System/Reflection/CopyCube.fx" );
		m_copyMipEffect->SetParameter( BlueSharedString( "tex_hi_res" ), source ? source : static_cast<ITr2TextureProvider*>( m_renderTargetCube ) );
		m_copyMipEffect->SetParameter( BlueSharedString( "tex_lo_res" ), m_postFilterTarget );
		m_copyMipEffect->SetOption( BlueSharedString( "HOLLYWOOD_MODE" ), BlueSharedString( m_hollywoodMode ? "HOLLYWOOD_ON" : "HOLLYWOOD_OFF" ) );
		if( m_hollywoodMode )
		{
			m_copyMipEffect->SetParameter( s_backLightColorName, Vector4( m_backlightColor ) );
			m_copyMipEffect->SetParameter( s_backLightContrastName, m_backlightContrast );
			m_copyMipEffect->SetParameter( s_viewDirectionName, Vector3( 0, 1, 0 ) );
		}

		m_initialized = true;
	}

	return true;
}

bool Tr2ReflectionProbe::OnModified( Be::Var* value )
{
	DestroyRenderTargets();
	PrepareResources();

	return true;
}

void Tr2ReflectionProbe::DestroyRenderTargets()
{
	for( unsigned i = 0; i < 6; i++ )
	{
		m_renderTargets[i]->Destroy();
		m_stencilMaps[i] = Tr2TextureAL();
	}

	m_renderTargetCube->Destroy();
	m_preFilterTarget->Destroy();
	m_postFilterTarget->Destroy();
	m_initialized = false;
	m_onePassDone = false;
	m_currentFrame = 0;
}

void Tr2ReflectionProbe::Filter( Tr2RenderContext& renderContext )
{
	if( !IsValid() )
		return;

	{
		GPU_REGION( renderContext, "Reflection Pre Filter" );

		if( m_hollywoodMode )
		{
			m_preFilterEffect->SetParameter( s_backLightColorName, Vector4( m_backlightColor ) );
			m_preFilterEffect->SetParameter( s_backLightContrastName, m_backlightContrast );
			m_preFilterEffect->SetParameter( s_viewDirectionName, Tr2Renderer::GetInverseViewTransform().GetZ() );

			m_copyMipEffect->SetParameter( s_backLightColorName, Vector4( m_backlightColor ) );
			m_copyMipEffect->SetParameter( s_backLightContrastName, m_backlightContrast );
			m_copyMipEffect->SetParameter( s_viewDirectionName, Tr2Renderer::GetInverseViewTransform().GetZ() );
		}

		Tr2Renderer::RunComputeShader( m_preFilterEffect, FILTER_SIZE * 2 / 8, FILTER_SIZE * 2 / 8, 6, renderContext );
	}

	{
		GPU_REGION( renderContext, "Reflection Filter Mip Generation" );
		m_preFilterTarget->GenerateMipMaps();
	}
	{
		GPU_REGION( renderContext, "Reflection Main Filter" );
		Tr2Renderer::RunComputeShader( m_filterEffect, FILTER_GROUP_DIM, 6, 1, renderContext );
	}
	{
		GPU_REGION( renderContext, "Copy Mip" );
		Tr2Renderer::RunComputeShader( m_copyMipEffect, FILTER_SIZE * 2 / 8, FILTER_SIZE * 2 / 8, 6, renderContext );
	}
}

void Tr2ReflectionProbe::RunFilter()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	DestroyRenderTargets();
	DoPrepareResources( PIXEL_FORMAT_R16G16B16A16_FLOAT, renderContext );
	Filter( renderContext );
}

bool Tr2ReflectionProbe::IsHollyWoodModeOn() const
{
	return m_hollywoodMode;
}

bool Tr2ReflectionProbe::ReadyForDynamicObjectReflections() const
{
	// We need to have initialized the cubemap, before we start using it for rendering reflections (otherwise we just get garbage!)
	return m_onePassDone;
}
