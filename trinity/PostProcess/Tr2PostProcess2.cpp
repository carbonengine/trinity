////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PostProcess2.h"
#include "Tr2Renderer.h"
#include "Tr2PostProcessRenderInfo.h"


Tr2PostProcess2::Tr2PostProcess2( IRoot* lockobj ):
	PARENTLOCK( m_luts )
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

	return taa_bias;
}

void Tr2PostProcess2::GetJitter( uint32_t renderWidth, uint32_t renderHeight, float& x, float& y )
{
	if( m_taa )
	{
		m_taa->GetJitter( renderWidth, renderHeight, x, y );		
	}
	else
	{
		x = 0;
		y = 0;
	}
}

void Tr2PostProcess2::GetLuts( std::vector<const Tr2PPLutEffect*>& container ) const
{
	container.clear();
	container.reserve( m_luts.size() );
	for( const auto& lut : m_luts )
	{	
		if( lut->IsActive() )
		{
			container.push_back( lut );
		}
	}
}