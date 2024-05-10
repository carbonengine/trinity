////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2FidelityFxCas.h"
#include "Tr2Renderer.h"

// FidelityFX headers
#define A_CPU
#include "ffx_a.h"
#include "ffx_cas.h"

const float CAS_INTENSITY = 0.5f;
const uint32_t CAS_THREAD_GROUP_WORK_REGION_DIM = 16;

Tr2FidelityFxCas::Tr2FidelityFxCas( IRoot* lockobj ) :
	m_dispatchX(1),
	m_dispatchY(1)
{
	m_shader.CreateInstance();
	m_shader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/CAS.fx" );
}

Tr2FidelityFxCas::~Tr2FidelityFxCas()
{
}

void Tr2FidelityFxCas::Setup( uint32_t renderWidth, uint32_t renderHeight )
{
	AF1 outWidth = static_cast<AF1>( renderWidth );
	AF1 outHeight = static_cast<AF1>( renderHeight );

	CasSetup( m_casConst.const0.u, m_casConst.const1.u, CAS_INTENSITY, outWidth, outHeight, outWidth, outHeight );

	m_shader->StartUpdate();
	m_shader->SetParameter( BlueSharedString( "const0" ), AMDUpscaling::AsVector( m_casConst.const0 ) );
	m_shader->SetParameter( BlueSharedString( "const1" ), AMDUpscaling::AsVector( m_casConst.const1 ) );
	m_shader->EndUpdate();

	m_dispatchX = ( renderWidth + ( CAS_THREAD_GROUP_WORK_REGION_DIM - 1 ) ) / CAS_THREAD_GROUP_WORK_REGION_DIM;
	m_dispatchY = ( renderHeight + ( CAS_THREAD_GROUP_WORK_REGION_DIM - 1 ) ) / CAS_THREAD_GROUP_WORK_REGION_DIM;
}

void Tr2FidelityFxCas::Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures )
{
	GPU_REGION( renderContext, "CAS Sharpening" );

	m_shader->SetParameter( BlueSharedString( "InputTexture" ), textures.input );
	m_shader->SetParameter( BlueSharedString( "OutputTexture" ), textures.output );
	
	Tr2Renderer::RunComputeShader( m_shader, m_dispatchX, m_dispatchY, 1, renderContext );
}
