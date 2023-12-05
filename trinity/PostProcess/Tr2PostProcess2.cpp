////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PostProcess2.h"
#include "Tr2Renderer.h"
#include "Tr2PostProcessRenderInfo.h"


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

	float upscaling_bias = 0.0f;

	if( m_upscaling )
	{
		// If upscaling is enabled, we need to provide a mip lod bias for certain textures (the ones that use it in shaders)
		upscaling_bias = m_upscaling->GetMipLodBias();
	}

	return min( taa_bias, upscaling_bias );
}

void Tr2PostProcess2::GetJitter( uint32_t renderWidth, uint32_t renderHeight, float& x, float& y )
{
	if( m_upscaling && m_upscaling->IsTemporal() )
	{
		m_upscaling->GetJitter( x, y );
	}
	else if( m_taa )
	{
		m_taa->GetJitter( renderWidth, renderHeight, x, y );		
	}
	else
	{
		x = 0;
		y = 0;
	}
}

void Tr2PostProcess2::GetJitterOffset( float& x, float& y )
{
	if( m_upscaling && m_upscaling->IsTemporal() )
	{
		m_upscaling->GetJitterOffset( x, y );
	}
	else
	{
		x = 0;
		y = 0;
	}
}

float Tr2PostProcess2::GetUpscalingAmount() const
{
	if( m_upscaling )
	{
		return m_upscaling->GetUpscalingAmount();
	}
	return 1.0f;
}