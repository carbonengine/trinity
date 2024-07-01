////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//

#if TRINITY_PLATFORM != TRINITY_STUB

#include "StdAfx.h"
#include "include/upscaling/Tr2Fsr1Upscaling.h"
#include "Tr2TextureAL.h"

#define A_CPU
#include "ffx_a.h"
#include "ffx_fsr1.h"








Tr2Fsr1UpscalingTechnique::Tr2Fsr1UpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	TECHNIQUE_PARENT_CLASS( technique, setting, frameGeneration, adapter )
{
}

Tr2Fsr1UpscalingTechnique::~Tr2Fsr1UpscalingTechnique()
{
}

std::vector<Tr2UpscalingAL::Setting> Tr2Fsr1UpscalingTechnique::GetAvailableSettings() const
{
	return {
		Tr2UpscalingAL::Setting::ULTRA_QUALITY,
		Tr2UpscalingAL::Setting::QUALITY,
		Tr2UpscalingAL::Setting::BALANCED,
		Tr2UpscalingAL::Setting::PERFORMANCE,
	};
}

void Tr2Fsr1UpscalingTechnique::Destroy( Tr2RenderContextAL& renderContext )
{
}

Tr2UpscalingContextAL* Tr2Fsr1UpscalingTechnique::CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) 
{
	return new Tr2Fsr1UpscalingContext( displayWidth, displayHeight, m_setting, sourceFormat, depthFormat );
}

Tr2Fsr1UpscalingContext::Tr2Fsr1UpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) :
	Tr2UpscalingContextAL( displayWidth, displayHeight, setting, false, sourceFormat, depthFormat )
{
	m_upscaling = 1.0f;
	switch( setting )
	{
	case Tr2UpscalingAL::Setting::ULTRA_QUALITY:
		m_upscaling = 1.3f;
		break;
	case Tr2UpscalingAL::Setting::QUALITY:
		m_upscaling = 1.5f;
		break;
	case Tr2UpscalingAL::Setting::BALANCED:
		m_upscaling = 1.7f;
		break;
	case Tr2UpscalingAL::Setting::PERFORMANCE:
		m_upscaling = 2.0f;
		break;
	default:
		CCP_LOGERR( "Invalid upscaling setting applied: %d. Upscaling amount will be forced to 1.0" );
	}

	m_renderWidth = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayWidth, m_upscaling );
	m_renderHeight = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayHeight, m_upscaling );
}

Tr2Fsr1UpscalingContext::~Tr2Fsr1UpscalingContext()
{

}

const uint8_t SHADER_BYTECODE[] = {

#if TRINITY_PLATFORM == TRINITY_METAL
	#include "include/upscaling/Fsr1Metal.h"
#else
	#include "include/upscaling/Fsr1DX.h"
#endif
};

struct FSRConstants
{
	float Const0[4];
	float Const1[4];
	float Const2[4];
	float Const3[4];
};

Tr2UpscalingAL::Result Tr2Fsr1UpscalingContext::Setup(Tr2RenderContextAL& renderContext)
{

    //Load the upscaling shader program
    auto signature = Tr2ShaderSignatureAL()
						 .Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 )
						 .Add( Tr2ShaderRegisterAL::SRV_TEXTURE2D, 0 )
						 .Add( Tr2ShaderRegisterAL::SAMPLER, 0 )
						 .Add( Tr2ShaderRegisterAL::UAV_TEXTURE2D, 0 );
	Tr2ShaderAL easuShader;
	CR_RETURN( easuShader.Create( Tr2RenderContextEnum::COMPUTE_SHADER, SHADER_BYTECODE, signature, "", renderContext.GetPrimaryRenderContext() ) );
	CR_RETURN( m_easuProgram.Create( &easuShader, 1, renderContext.GetPrimaryRenderContext() ) );


    //Create the sampler for the input
    Tr2SamplerDescription desc( Tr2RenderContextEnum::TextureFilter::TF_LINEAR, Tr2RenderContextEnum::TextureAddressMode::TA_CLAMP );
	m_sampler.Create( desc, renderContext.GetPrimaryRenderContext() );


    //Create the FSR1 EASU constant buffer
    float renderWidth_f = float( m_renderWidth );
	float renderHeight_f = float( m_renderHeight );
	float displayWidth_f = float( m_displayWidth );
	float displayHeight_f = float( m_displayHeight );

	FSRConstants constants;
	FsrEasuCon(

		(uint32_t*)constants.Const0,
		(uint32_t*)constants.Const1,
		(uint32_t*)constants.Const2,
		(uint32_t*)constants.Const3,

		renderWidth_f,
		renderHeight_f,

		renderWidth_f,
		renderHeight_f,

		displayWidth_f,
		displayHeight_f 
    );
	m_constantBuffer.Create( sizeof( FSRConstants ), Tr2ConstantUsageAL::IMMUTABLE, &constants, renderContext.GetPrimaryRenderContext() );


	return Tr2UpscalingAL::Result::OK;
}

void Tr2Fsr1UpscalingContext::Destroy( Tr2RenderContextAL& renderContext )
{
}

bool Tr2Fsr1UpscalingContext::IsTemporal() const
{
	return false;
}

bool Tr2Fsr1UpscalingContext::HasSharpening() const
{
	return false;
}

void Tr2Fsr1UpscalingContext::UpdateJitter()
{
}

uint32_t Tr2Fsr1UpscalingContext::GetDispatchRequirements() const
{
	return 0;
}




Tr2UpscalingAL::Result Tr2Fsr1UpscalingContext::Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{

	GPU_REGION_AL( renderContext, "FSR1 Upscaling" );

	renderContext.SetShaderProgram(m_easuProgram);
		
	Tr2ResourceSetDescriptionAL desc( m_easuProgram );
	desc.SetSrv( Tr2RenderContextEnum::ShaderType::COMPUTE_SHADER, 0, *dispatchParameters.input );
	desc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, *dispatchParameters.output );
	desc.SetSampler( Tr2RenderContextEnum::COMPUTE_SHADER, 0, m_sampler );

	Tr2ResourceSetAL resourceSet;
	resourceSet.Create( desc, m_easuProgram, renderContext.GetPrimaryRenderContext() );

	renderContext.SetResourceSet( resourceSet );
		
	renderContext.SetConstants( m_constantBuffer, Tr2RenderContextEnum::COMPUTE_SHADER, 0 );
		
		
    int workgroupSize = 16;
	renderContext.RunComputeShader((m_displayWidth + workgroupSize - 1) / workgroupSize, (m_displayHeight + workgroupSize - 1) / workgroupSize, 1);

	return Tr2UpscalingAL::Result::OK;
}

#endif
