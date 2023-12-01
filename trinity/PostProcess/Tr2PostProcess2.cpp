////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PostProcess2.h"
#include "Tr2Renderer.h"
#include "Tr2PostProcessRenderInfo.h"
#include "Effects/Tr2PPFidelityFXEffect.h"


Tr2PostProcess2::Tr2PostProcess2( IRoot* lockobj )
{
	
}


Tr2PostProcess2::~Tr2PostProcess2()
{
}


float Tr2PostProcess2::GetMipLodBias() const
{

	float taa_bias = 0.0f;
	if( m_taa )
	{
		taa_bias = m_taa->IsActive() && m_taa->m_applyMipBias ? -1.0f : 0.0f;
	}

	float fsr_bias = 0.0f;

	if( m_fidelityFX )
	{
		// If FSR is enabled, we need to provide a mip lod bias for certain textures (the ones that use it in shaders)
		fsr_bias =  m_fidelityFX->GetFSRMipLodBias();
	}

	return min( taa_bias, fsr_bias );
}
