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

namespace FSR1
{
	const uint8_t SHADER_BYTECODE[] = {

	#if TRINITY_PLATFORM == TRINITY_METAL
	#include "include/upscaling/Fsr1Metal.h"
	#else
	#include "include/upscaling/Fsr1DX.h"
	#endif
	};

	//For some reason, metal uses the same register space for SRVs and UAVs, so the index is different for the Metal shader.
	#if TRINITY_PLATFORM == TRINITY_METAL
	const uint32_t SRV_REGISTER_INDEX = 1;
	#else
	const uint32_t SRV_REGISTER_INDEX = 0;
	#endif
    
	struct FSRConstants
	{
		float Const0[4];
		float Const1[4];
		float Const2[4];
		float Const3[4];
	};
}

Tr2Fsr1UpscalingTechnique::Tr2Fsr1UpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	TECHNIQUE_PARENT_CLASS( renderContext, technique, setting, frameGeneration, adapter )
{
    SanitizeState();
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

bool Tr2Fsr1UpscalingTechnique::IsTemporal() const
{
	return false;
}

bool Tr2Fsr1UpscalingTechnique::IsAvailable() const
{
	extern bool g_force_fsr1_availability;

#if TRINITY_PLATFORM == TRINITY_METAL
	if( @available( macOS 13.0, * ) )
	{
		return g_force_fsr1_availability;
	}
#endif
	return true;
} 

Tr2UpscalingContextAL* Tr2Fsr1UpscalingTechnique::CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params )
{
	return new Tr2Fsr1UpscalingContext( m_setting, params );
}

Tr2Fsr1UpscalingContext::Tr2Fsr1UpscalingContext( Tr2UpscalingAL::Setting setting, Tr2UpscalingAL::UpscalingContextParams params ) :
	Tr2UpscalingContextAL( setting, false, params )
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

	//Load the upscaling shader program
	auto signature = Tr2ShaderSignatureAL()
						 .Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 )
						 .Add( Tr2ShaderRegisterAL::SRV_TEXTURE2D, FSR1::SRV_REGISTER_INDEX )
						 .Add( Tr2ShaderRegisterAL::SAMPLER, 0 )
						 .Add( Tr2ShaderRegisterAL::UAV_TEXTURE2D, 0 )
						 .Add( Tr2ShaderThreadGroupSizeAL( 64, 1, 1 ) );

	auto& renderContext = params.renderContext;
	Tr2ShaderAL easuShader;
	easuShader.Create( Tr2RenderContextEnum::COMPUTE_SHADER, FSR1::SHADER_BYTECODE, signature, "", renderContext.GetPrimaryRenderContext() );
	m_easuProgram.Create( &easuShader, 1, renderContext.GetPrimaryRenderContext() );


	//Create the sampler for the input
	Tr2SamplerDescription desc( Tr2RenderContextEnum::TextureFilter::TF_LINEAR, Tr2RenderContextEnum::TextureAddressMode::TA_CLAMP );
	m_sampler.Create( desc, renderContext.GetPrimaryRenderContext() );


	//Create the FSR1 EASU constant buffer
	float renderWidth_f = float( m_renderWidth );
	float renderHeight_f = float( m_renderHeight );
	float displayWidth_f = float( m_displayWidth );
	float displayHeight_f = float( m_displayHeight );

	FSR1::FSRConstants constants;
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
		displayHeight_f );
	m_constantBuffer.Create( sizeof( FSR1::FSRConstants ), Tr2ConstantUsageAL::IMMUTABLE, &constants, renderContext.GetPrimaryRenderContext() );
}

Tr2Fsr1UpscalingContext::~Tr2Fsr1UpscalingContext()
{

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

Tr2UpscalingAL::Result Tr2Fsr1UpscalingContext::Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	if( !AreDispatchParametersValid( dispatchParameters ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}

	auto& renderContext = m_params.renderContext;

	GPU_REGION_AL( renderContext, "FSR1 Upscaling" );

	renderContext.SetShaderProgram(m_easuProgram);
		
	Tr2ResourceSetDescriptionAL desc( m_easuProgram );
	desc.SetSrv( Tr2RenderContextEnum::COMPUTE_SHADER, FSR1::SRV_REGISTER_INDEX, *dispatchParameters.input );
	desc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, *dispatchParameters.output );
	desc.SetSampler( Tr2RenderContextEnum::COMPUTE_SHADER, 0, m_sampler );

	Tr2ResourceSetAL resourceSet;
	resourceSet.Create( desc, m_easuProgram, m_params.renderContext.GetPrimaryRenderContext() );

	renderContext.SetResourceSet( resourceSet );
		
	renderContext.SetConstants( m_constantBuffer, Tr2RenderContextEnum::COMPUTE_SHADER, 0 );
    
    
    int workgroupSize = 16;
	renderContext.RunComputeShader((m_displayWidth + workgroupSize - 1) / workgroupSize, (m_displayHeight + workgroupSize - 1) / workgroupSize, 1);

	return Tr2UpscalingAL::Result::OK;
}

#endif
