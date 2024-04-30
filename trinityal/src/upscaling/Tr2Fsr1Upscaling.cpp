////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"
#include "include/upscaling/Tr2Fsr1Upscaling.h"
#include "Tr2TextureAL.h"

Tr2Fsr1UpscalingTechnique::Tr2Fsr1UpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration ) :
	Tr2UpscalingTechniqueAL( technique, setting, frameGeneration )
{
}

Tr2Fsr1UpscalingTechnique::~Tr2Fsr1UpscalingTechnique()
{
}

std::vector<Tr2UpscalingAL::Setting> Tr2Fsr1UpscalingTechnique::GetAvailableSettings() const
{
	return std::vector<Tr2UpscalingAL::Setting>();
}

Tr2UpscalingAL::Result Tr2Fsr1UpscalingTechnique::Setup()
{
	return Tr2UpscalingAL::Result::OK;
}

void Tr2Fsr1UpscalingTechnique::Destroy( Tr2RenderContextAL& renderContext )
{
}

Tr2UpscalingContextAL* Tr2Fsr1UpscalingTechnique::CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) 
{
	return nullptr;
}

Tr2Fsr1UpscalingContext::Tr2Fsr1UpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) :
	Tr2UpscalingContextAL( displayWidth, displayHeight, setting, frameGeneration, sourceFormat, depthFormat )
{

}

Tr2Fsr1UpscalingContext::~Tr2Fsr1UpscalingContext()
{

}

Tr2UpscalingAL::Result Tr2Fsr1UpscalingContext::Setup(Tr2RenderContextAL& renderContext)
{
	// create a rendertarget for the intermediate render
	return Tr2UpscalingAL::Result::TECHNIQUE_NOT_SUPPORTED;
}

void Tr2Fsr1UpscalingContext::Destroy( Tr2RenderContextAL& renderContex )
{
}

bool Tr2Fsr1UpscalingContext::IsTemporal() const
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

	return Tr2UpscalingAL::Result::OK;
}
