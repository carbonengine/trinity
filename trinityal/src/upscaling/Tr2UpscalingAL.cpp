////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"
#include "include/upscaling/Tr2UpscalingAL.h"

bool g_upscalingDebug = false;

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
        case CONTEXT_SETUP_FAILED:
            CCP_LOGWARN( "Tr2Upscaling: Context setup failed" );
            break;
        case INCORRECT_INPUT:
            CCP_LOGWARN( "Tr2Upscaling: Incorrect input" );
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

	UpscalingInfo::UpscalingInfo():
		displayWidth( 0 ),
		displayHeight( 0 ),
		renderWidth( 0 ),
		renderHeight( 0 ),
		technique( Tr2UpscalingAL::Technique::NONE ),
		setting( Tr2UpscalingAL::Setting::NATIVE ),
		frameGeneration( false ),
		temporal(false),
		upscalingAmount( 1.0f ),
		jitterX( 0.0f ),
		jitterY( 0.0f ),
		mipLevelBias( 0.0f )
	{}


	const char* GetTechniqueName( Technique technique )
	{
		switch( technique )
		{
		case NONE:
			return "None";
		case FSR1:
			return "FSR1";
		case FSR2:
			return "FSR2";
		case FSR3:
			return "FSR3";
		case DLSS:
			return "DLSS";
		case METALFX:
			return "MetalFx";
		case XESS:
			return "XeSS";
		}
		return "Unknown";
	}
	
	const char* GetSettingName( Setting setting )
	{
		switch( setting )
		{
		case NATIVE:
			return "Native";
		case ULTRA_QUALITY:
			return "UltraQuality";
		case QUALITY:
			return "Quality";
		case BALANCED:
			return "Balanced";
		case PERFORMANCE:
			return "Performance";
		case ULTRA_PERFORMANCE:
			return "UltraPerformance";
		}
		return "Unknown";
	}

}


Tr2UpscalingTechniqueAL::Tr2UpscalingTechniqueAL( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration ):
	m_technique( technique ),
	m_setting(setting),
	m_frameGeneration( frameGeneration )
{
	m_contexts = std::map<uint32_t, std::unique_ptr<Tr2UpscalingContextAL>>();
}

Tr2UpscalingTechniqueAL::~Tr2UpscalingTechniqueAL()
{
    for( auto& context : m_contexts )
    {
        context.second.release();
    }
    
    m_contexts.clear();
}

void Tr2UpscalingTechniqueAL::SanitizeState()
{
	const auto& availableSettings = GetAvailableSettings();
	m_frameGeneration = m_frameGeneration && SupportsFrameGeneration();
	if( availableSettings.size() > 0 && std::find( availableSettings.begin(), availableSettings.end(), m_setting ) == availableSettings.end() )
	{
		// find the closest setting, without going into higher quality
		// start by setting the bestCandidate as the lowest quality setting
		Tr2UpscalingAL::Setting bestCandidate = *availableSettings.begin();
		uint32_t leastDistance = std::numeric_limits<uint32_t>().max();
		for( auto& availableSetting : availableSettings )
		{
			uint32_t currentDistance = abs(int(m_setting) - int(availableSetting));
			if( leastDistance > currentDistance )
			{
				bestCandidate = availableSetting;
				leastDistance = currentDistance;
			}
		}
		m_setting = bestCandidate;
	}
}

std::vector<Tr2UpscalingAL::Setting> Tr2UpscalingTechniqueAL::GetAvailableSettings() const
{
	return std::vector<Tr2UpscalingAL::Setting>();
}

void Tr2UpscalingTechniqueAL::GetState( Tr2UpscalingAL::Technique& technique, Tr2UpscalingAL::Setting& setting, bool& frameGeneration )
{
	technique = m_technique;
	setting = m_setting;
	frameGeneration = m_frameGeneration;
}

void Tr2UpscalingTechniqueAL::MarkFrameEvent( Tr2RenderContextAL& renderContext, Tr2RenderContextEnum::FrameEvent& frameEvent )
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


Tr2UpscalingContextAL* Tr2UpscalingTechniqueAL::GetContext( Tr2RenderContextAL& renderContext, uint32_t upscalingContextID )
{
	if( m_contexts.find( upscalingContextID ) == m_contexts.end() )
	{
		return nullptr;
	}

	return m_contexts[upscalingContextID].get();
}

Tr2UpscalingContextAL* Tr2UpscalingTechniqueAL::CreateContext( Tr2RenderContextAL& renderContext, uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat )
{
	auto context = CreateContextInstance( displayWidth, displayHeight, sourceFormat, depthFormat );
	context->Setup( renderContext );
	m_contexts[context->GetID()].reset( context );

	return context;
}

void Tr2UpscalingTechniqueAL::DeleteContext( Tr2RenderContextAL& renderContext, uint32_t contextID )
{
	auto context = m_contexts.find( contextID );
	if( context == m_contexts.end() )
	{
		// cant find it...
		return;
	}

	context->second.get()->Destroy( renderContext );
	m_contexts.erase( context );
}

bool Tr2UpscalingTechniqueAL::IsAvailable( Tr2RenderContextAL& renderContext ) const
{
	return true;
}

bool Tr2UpscalingTechniqueAL::SupportsFrameGeneration( ) const
{
	return false;
}

Tr2UpscalingContextAL::Tr2UpscalingContextAL( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) :
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
	m_jitterXScale( 2.0f ),
	m_jitterYScale( -2.0f ), 
	m_sourceFormat( sourceFormat ),
	m_depthFormat( depthFormat )
{
	static uint32_t CONTEXT_ID = 0; 
	m_id = CONTEXT_ID++;
}

Tr2UpscalingContextAL::~Tr2UpscalingContextAL()
{
}

void Tr2UpscalingContextAL::Destroy( Tr2RenderContextAL& renderContext )
{
}

uint32_t Tr2UpscalingContextAL::GetID() const
{
	return m_id;
}

void Tr2UpscalingContextAL::GetRenderDimensions( uint32_t& width, uint32_t& height ) const
{
	width = m_renderWidth;
	height = m_renderHeight;
}

void Tr2UpscalingContextAL::GetDisplayDimensions( uint32_t& width, uint32_t& height ) const
{
	width = m_displayWidth;
	height = m_displayHeight;
}

void Tr2UpscalingContextAL::GetJitter( float& x, float& y ) const
{
	x = m_jitterXScale * m_jitterX / (float)m_renderWidth;
	y = m_jitterYScale * m_jitterY / (float)m_renderHeight;
}

void Tr2UpscalingContextAL::SetHudLessTexture( Tr2TextureAL* texture )
{
}

float Tr2UpscalingContextAL::GetMipLevelBias() const
{
	return log2( 1.0f / m_upscaling ) - 1.0f;
}

float Tr2UpscalingContextAL::GetUpscalingAmount() const
{
	return m_upscaling;
}

void Tr2UpscalingContextAL::Reset()
{
	m_reset = true;
}

bool Tr2UpscalingContextAL::AreDispatchParametersValid( Tr2UpscalingAL::DispatchParameters& dispatchParameters ) const
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
