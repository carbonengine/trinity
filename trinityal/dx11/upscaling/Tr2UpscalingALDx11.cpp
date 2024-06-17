////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX11
#include "Tr2UpscalingALDx11.h"
#include <upscaling/Tr2Fsr1Upscaling.h>
#include "Tr2DlssUpscaling.h"

namespace TrinityALImpl
{

	Tr2UpscalingTechniqueDx11::Tr2UpscalingTechniqueDx11( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) : 
	Tr2UpscalingTechniqueAL( technique, setting, frameGeneration, adapter )
	{
	}

	Tr2UpscalingTechniqueDx11::~Tr2UpscalingTechniqueDx11()
	{
	}	
	
	void Tr2UpscalingTechniqueDx11::AttachToDevice( CComPtr<ID3D11Device>& device )
	{
	}

	Tr2UpscalingTechniqueDx11* CreateUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter )
	{
		TrinityALImpl::Tr2UpscalingTechniqueDx11* techniqueImpl = nullptr;
		switch( technique )
		{
		case Tr2UpscalingAL::Technique::FSR1:
			techniqueImpl = new Tr2Fsr1UpscalingTechnique( technique, setting, frameGeneration, adapter );
			break;
		case Tr2UpscalingAL::DLSS:
			techniqueImpl = new Tr2DlssUpscalingTechnique( technique, setting, frameGeneration, adapter );
			break;
		}
		if( techniqueImpl && techniqueImpl->IsAvailable( renderContext ) )
		{
			return techniqueImpl;
		}
		delete techniqueImpl;
		techniqueImpl = nullptr;
		return nullptr;
	}
}
#endif
