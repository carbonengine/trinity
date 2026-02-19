////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PostProcess2.h"
#include "Tr2Renderer.h"


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
		taa_bias = m_taa->IsActive() ? -1.0f : 0.0f;
	}

	return taa_bias;
}

void Tr2PostProcess2::MarkAllDirty()
{
	auto SetDirtyIfNotNull = []( const auto& effect ) {
		if( effect )
		{
			effect->SetDirty( true );
		}
	};

	SetDirtyIfNotNull( m_signalLoss );
	SetDirtyIfNotNull( m_godRays );
	SetDirtyIfNotNull( m_bloom );
	SetDirtyIfNotNull( m_dynamicExposure );
	SetDirtyIfNotNull( m_filmGrain );
	SetDirtyIfNotNull( m_desaturate );
	SetDirtyIfNotNull( m_fade );
	SetDirtyIfNotNull( m_vignette );
	SetDirtyIfNotNull( m_fog );
	SetDirtyIfNotNull( m_taa );
	SetDirtyIfNotNull( m_depthOfField );
	SetDirtyIfNotNull( m_tonemapping );
	SetDirtyIfNotNull( m_colorCorrection );
	SetDirtyIfNotNull( m_generic );
	for( auto& lut : m_luts )
	{
		SetDirtyIfNotNull( lut );
	}
}

void Tr2PostProcess2::GetLuts( std::vector<const Tr2PPLutEffect*>& container ) const
{
	container.clear();
	container.reserve( m_luts.size() );
	if( m_lut && m_lut->IsActive() )
	{
		container.push_back( m_lut );
	}
	for( const auto& lut : m_luts )
	{	
		if( lut->IsActive() )
		{
			container.push_back( lut );
		}
	}
}


void Tr2PostProcess2::ClearLuts()
{
	m_luts.Clear();
}
