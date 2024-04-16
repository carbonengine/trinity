////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"
#include "include/upscaling/Tr2UpscalingAL.h"

namespace Tr2UpscalingAL
{
	void LogResult( Result result )
	{
		switch( result )
		{
		case OK:
			CCP_LOGWARN( "Tr2Upscaling: OK" );
			break;

		case TECHNIQUE_NOT_SUPPORTED:
			CCP_LOGWARN( "Tr2Upscaling: Platform is not supported for the selected upscaling technique" );
			break;

		case HARDWARE_NOT_SUPPORTED:
			CCP_LOGWARN( "Tr2Upscaling: Hardware is not supported" );
			break;
		}
	}

	JitterSequence GenerateHaltonSequence( uint32_t totalPhases, uint32_t xBase, uint32_t yBase )
	{
		auto result = JitterSequence();
		result.reserve( totalPhases );

		for( uint32_t i = 0; i < totalPhases; ++i )
		{
			result.push_back( { Halton( i, xBase ), Halton( i, yBase ) } );
		}

		return result;
	}

	float Halton( uint32_t index, uint32_t base )
	{
		float result = 0.0;
		float fractional = 1.0;
		while( index > 0 )
		{
			fractional /= float( base );
			result += fractional * float( index % base );
			index /= base;
		}
		return result - 0.5f;
	}

	uint32_t ConvertDisplaySizeToRenderSize( uint32_t displaySize, float upscaling )
	{
		uint32_t renderSize = uint32_t( displaySize / (float)upscaling );
		uint32_t addition = displaySize % 2 != renderSize % 2;
		return renderSize + addition;
	}
}


Tr2UpscalingTechniqueAL::Tr2UpscalingTechniqueAL( Tr2UpscalingAL::Setting setting, bool frameGeneration ):
	m_setting(setting),
	m_frameGeneration( frameGeneration )
{
	
}

void Tr2UpscalingTechniqueAL::MarkFrameEvent( Tr2RenderContextEnum::FrameEvent& frameEvent )
{
	if( frameEvent == Tr2RenderContextEnum::FrameEvent::FRAME_EVENT_RENDERING_STARTED )
	{
		// update all the jitter when rendering starts
		for( auto& keyAndContext : m_contexts )
		{
			keyAndContext.second->UpdateJitter();
		}
	}
}


Tr2UpscalingContext* Tr2UpscalingTechniqueAL::GetContext( Tr2RenderContextAL& renderContext, uint32_t displayWidth, uint32_t displayHeight )
{
	uint32_t key = displayWidth + displayHeight;

	if( m_contexts.find( key ) == m_contexts.end() )
	{
		CCP_LOGERR( "Tr2UpscalingTechniqueAL:GetContext Context does not exist for (%d, %d)", displayWidth, displayHeight );
		return nullptr;
	}

	return m_contexts[key].get();
}

Tr2UpscalingContext* Tr2UpscalingTechniqueAL::CreateContext( Tr2RenderContextAL& renderContext, uint32_t displayWidth, uint32_t displayHeight )
{
	uint32_t key = displayWidth + displayHeight;

	if( m_contexts.find( key ) == m_contexts.end() )
	{
		auto context = CreateContextInstance( displayWidth, displayHeight );
		context->Setup( renderContext );
		m_contexts[key].reset( context );
	}
	else
	{
		CCP_LOGERR( "Tr2UpscalingTechniqueAL:CreateContext Context already existed for (%d, %d)", displayWidth, displayHeight );
	}

	return m_contexts[key].get();
}

Tr2UpscalingContext::Tr2UpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration ) :
	m_setting(setting),
	m_frameGeneration( frameGeneration ),
	m_displayWidth( displayWidth ),
	m_displayHeight( displayHeight ),
	m_renderWidth( 0 ),
	m_renderHeight( 0 ),
	m_upscaling( 0 ),
	m_reset( true ),
	m_jitterIndex( 0 ),
	m_jitterX( 0.0f ),
	m_jitterY( 0.0f ),
	m_jitterXScale( 1.0f ),
	m_jitterYScale( -1.0f )
{
}

void Tr2UpscalingContext::GetRenderDimensions( uint32_t& width, uint32_t& height ) const
{
	width = m_renderWidth;
	height = m_renderHeight;
}

void Tr2UpscalingContext::GetDisplayDimensions( uint32_t& width, uint32_t& height ) const
{
	width = m_displayWidth;
	height = m_displayHeight;
}

void Tr2UpscalingContext::GetJitter( float& x, float& y ) const
{
	x = m_jitterXScale * m_jitterX / (float)m_renderWidth;
	y = m_jitterYScale * m_jitterY / (float)m_renderHeight;
}

float Tr2UpscalingContext::GetMipLevelBias() const
{
	return log2( 1.0f / m_upscaling ) - 1.0f;
}

float Tr2UpscalingContext::GetUpscalingAmount() const
{
	return m_upscaling;
}

void Tr2UpscalingContext::Reset()
{
	m_reset = true;
}

bool Tr2UpscalingContext::AreDisplayParametersValid( Tr2UpscalingAL::DispatchParameters& dispatchParameters ) const
{
	bool valid = true;
	if( dispatchParameters.input == nullptr )
	{
		CCP_LOGERR( "Tr2UpscalingContext: \"input\" is a required parameter but is missing from dispatch parameters" );
		valid = false;
	}
	if( dispatchParameters.output == nullptr )
	{
		CCP_LOGERR( "Tr2UpscalingContext: \"output\" is a required parameter but is missing from dispatch parameters" );
		valid = false;
	}
	auto requirements = GetDispatchRequirements();
	if( requirements & Tr2UpscalingAL::DispatchRequirements::DEPTH && dispatchParameters.depth == nullptr )
	{
		CCP_LOGERR( "Tr2UpscalingContext: \"depth\" is a required parameter but is missing from dispatch parameters" );
		valid = false;
	}
	if( requirements & Tr2UpscalingAL::DispatchRequirements::OPAQUE_ONLY && dispatchParameters.opaqueOnly == nullptr )
	{
		CCP_LOGERR( "Tr2UpscalingContext: \"opqaqueOnly\" is a required parameter but is missing from dispatch parameters" );
		valid = false;
	}
	if( requirements & Tr2UpscalingAL::DispatchRequirements::REACTIVE && dispatchParameters.reactive == nullptr )
	{
		CCP_LOGERR( "Tr2UpscalingContext: \"reactive\" is a required parameter but is missing from dispatch parameters" );
		valid = false;
	}
	if( requirements & Tr2UpscalingAL::DispatchRequirements::VELOCITY && dispatchParameters.velocity == nullptr )
	{
		CCP_LOGERR( "Tr2UpscalingContext: \"velocity\" is a required parameter but is missing from dispatch parameters" );
		valid = false;
	}

	return valid;
}
