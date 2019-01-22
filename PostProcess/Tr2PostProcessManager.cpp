////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#pragma once

#include "StdAfx.h"
#include "Tr2Renderer.h"
#include "Tr2PostProcessManager.h"
#include "RenderJob\TriStepSetStandardRenderStates.h"


Tr2PostProcessManager::Tr2PostProcessManager( IRoot* lockobj )
{
	m_rt1.CreateInstance();
	m_rt1->m_name = "PostProcess RT1";

	m_rt2.CreateInstance();
	m_rt2->m_name = "PostProcess RT2";

	m_accumulationBuffer.CreateInstance();
	m_accumulationBuffer->m_name = "PostProcess Accumulation";

	m_velocityBuffer.CreateInstance();
	m_velocityBuffer->m_name = "PostProcess Velocity";

	m_distortionBuffer.CreateInstance();
	m_distortionBuffer->m_name = "PostProcess Distrotion";

	m_basePostProcess.CreateInstance();
}


Tr2PostProcessManager::~Tr2PostProcessManager()
{
	if( m_rt1->IsValid() )
	{
		m_rt1->Destroy();
	}
	if( m_rt2->IsValid() )
	{
		m_rt2->Destroy();
	}
	if( m_accumulationBuffer->IsValid() )
	{
		m_accumulationBuffer->Destroy();
	}
	if( m_velocityBuffer->IsValid() )
	{
		m_velocityBuffer->Destroy();
	}
	if( m_distortionBuffer->IsValid() )
	{
		m_distortionBuffer->Destroy();
	}
}


bool Tr2PostProcessManager::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_sourceBuffer ) ) {
		CopySourceTo( m_rt1 );
		CopySourceTo( m_rt2 );
	}

	return true;
}


void Tr2PostProcessManager::CopySourceTo( Tr2RenderTargetPtr renderTarget )
{
	if( renderTarget->IsValid() )
	{
		renderTarget->Destroy();
	}
	
	if( m_sourceBuffer )
	{
		renderTarget->Create(
			uint32_t( float( m_sourceBuffer->GetWidth() ) * 0.5f),
			uint32_t( float( m_sourceBuffer->GetHeight() ) * 0.5f ),
			m_sourceBuffer->GetMipCount(),
			m_sourceBuffer->GetFormat(),
			m_sourceBuffer->GetMsaaType(),
			m_sourceBuffer->GetMsaaQuality() );
	}
}


void Tr2PostProcessManager::SetBasePostProcess( Tr2PostProcess2Ptr postProcess )
{
	m_basePostProcess = postProcess;
}


void Tr2PostProcessManager::CreateBuffers( uint32_t width, uint32_t height )
{
	
	if( m_accumulationBuffer->IsValid() )
	{
		m_accumulationBuffer->Destroy();
	}
	if( m_velocityBuffer->IsValid() )
	{
		m_velocityBuffer->Destroy();
	}
	if( m_distortionBuffer->IsValid() )
	{
		m_distortionBuffer->Destroy();
	}

	// TODO - this could break in dx9
	m_accumulationBuffer->Create( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R11G11B10_FLOAT );
	m_velocityBuffer->Create( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_FLOAT, 8U, 0U );
	m_distortionBuffer->Create( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 1 );
}


void Tr2PostProcessManager::Update()
{
	m_finalPostProcess = m_basePostProcess;

	auto signalLoss = m_finalPostProcess->GetSignalLoss();
}


void Tr2PostProcessManager::Render( Tr2ShaderBufferPtr psData, Tr2RenderContext& renderContext )
{
	Tr2Renderer::PushRenderTarget( *m_sourceBuffer, renderContext);
	Tr2Renderer::PushDepthStencilBuffer( nullDS, renderContext );

	auto signalLoss = m_finalPostProcess->GetSignalLoss();
	auto godRays = m_finalPostProcess->GetGodRays();

	if( godRays )
	{
		godRays->Render( renderContext, m_rt1, m_rt2, m_sourceBuffer, psData );
	}

	if( signalLoss )
	{
		signalLoss->Render( renderContext );
	}

	Tr2Renderer::PopDepthStencilBuffer( renderContext );
	Tr2Renderer::PopRenderTarget( renderContext );
}
