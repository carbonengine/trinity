// Copyright © 2024 CCP ehf.

#include "StdAfx.h"
#include "include/upscaling/Tr2UpscalingAL.h"

bool g_upscalingDebug = false;
uint32_t g_streamlineAppID = 0;

CCP_STATS_DECLARE( generatedFrames, "GeneratedFrames", false, CST_COUNTER_LOW, "Generated Frames between presents" );
bool g_force_fsr1_availability = false;


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
	uint32_t renderSize = uint32_t( (float)displaySize / (float)upscaling );
	uint32_t addition = displaySize % 2 != renderSize % 2;
	return renderSize + addition;
}

UpscalingInfo::UpscalingInfo() :
	displayWidth( 0 ),
	displayHeight( 0 ),
	renderWidth( 0 ),
	renderHeight( 0 ),
	technique( Tr2UpscalingAL::Technique::NONE ),
	setting( Tr2UpscalingAL::Setting::NATIVE ),
	frameGeneration( false ),
	temporal( false ),
	hasSharpening( false ),
	upscalingAmount( 1.0f ),
	jitterX( 0.0f ),
	jitterY( 0.0f ),
	mipLevelBias( 0.0f )
{
}

UpscalingContextParams::UpscalingContextParams( Tr2RenderContextAL& renderContext ) :
	displayWidth( 0 ),
	displayHeight( 0 ),
	sourceFormat( Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_UNKNOWN ),
	depthFormat( Tr2RenderContextEnum::DepthStencilFormat::DSFMT_UNKNOWN ),
	allowFramegen( false ),
	renderContext( renderContext )
{
}

//implement the equal operator
bool UpscalingContextParams::operator==( const UpscalingContextParams& other ) const
{
	return displayWidth == other.displayWidth &&
		displayHeight == other.displayHeight &&
		sourceFormat == other.sourceFormat &&
		depthFormat == other.depthFormat &&
		allowFramegen == other.allowFramegen;
}

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
		return "Ultra Quality";
	case QUALITY:
		return "Quality";
	case BALANCED:
		return "Balanced";
	case PERFORMANCE:
		return "Performance";
	case ULTRA_PERFORMANCE:
		return "Ultra Performance";
	}
	return "Unknown";
}

}


Tr2UpscalingTechniqueAL::Tr2UpscalingTechniqueAL( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	m_frameGeneration( frameGeneration ),
	m_setting( setting ),
	m_technique( technique ),
	m_renderContext( renderContext )
{
	m_contexts = std::map<uint32_t, std::unique_ptr<Tr2UpscalingContextAL>>();
}

void Tr2UpscalingTechniqueAL::ReleaseResources()
{
}

void Tr2UpscalingTechniqueAL::SanitizeState()
{
	const auto& availableSettings = GetAvailableSettings();
	m_frameGeneration = m_frameGeneration && SupportsFrameGeneration();
	if( !availableSettings.empty() && std::find( availableSettings.begin(), availableSettings.end(), m_setting ) == availableSettings.end() )
	{
		// find the closest setting, without going into higher quality
		// start by setting the bestCandidate as the lowest quality setting
		Tr2UpscalingAL::Setting bestCandidate = *availableSettings.begin();
		uint32_t leastDistance = std::numeric_limits<uint32_t>().max();
		for( auto& availableSetting : availableSettings )
		{
			uint32_t currentDistance = abs( int( m_setting ) - int( availableSetting ) );
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

void Tr2UpscalingTechniqueAL::GetState( Tr2UpscalingAL::Technique& technique, Tr2UpscalingAL::Setting& setting, bool& frameGeneration ) const
{
	technique = m_technique;
	setting = m_setting;
	frameGeneration = m_frameGeneration;
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


Tr2UpscalingContextAL* Tr2UpscalingTechniqueAL::GetContext( uint32_t upscalingContextID )
{
	if( m_contexts.find( upscalingContextID ) == m_contexts.end() )
	{
		return nullptr;
	}

	return m_contexts[upscalingContextID].get();
}

Tr2UpscalingContextAL* Tr2UpscalingTechniqueAL::CreateContext( Tr2UpscalingAL::UpscalingContextParams params, uint32_t existingContext )
{
	if( existingContext != Tr2UpscalingAL::INVALID_CONTEXT_ID )
	{
		auto context = m_contexts.find( existingContext );
		if( context != m_contexts.end() )
		{
			if( context->second->Reuse( params ) )
			{
				context->second->SetupForReuse();
				return context->second.get();
			}
		}
		DeleteContext( existingContext );
	}
	// safety harness, displayWidth and displayHeight need to be bigger than 1 since it is hard to render into something
	// that is smaller than a pixel...
	if( params.displayWidth < 2 || params.displayHeight < 2 )
	{
		CCP_LOGWARN( "Cannot create an upscaler for output that is less than 2x2 pixels. Ignoring context creation" );
		return nullptr;
	}
	auto context = CreateContextInstance( params );
	m_contexts[context->GetID()].reset( context );

	return context;
}

void Tr2UpscalingTechniqueAL::DeleteContext( uint32_t contextID )
{
	auto context = m_contexts.find( contextID );
	if( context == m_contexts.end() )
	{
		// cant find it...
		return;
	}

	m_contexts.erase( context );
}

bool Tr2UpscalingTechniqueAL::IsAvailable() const
{
	return true;
}

bool Tr2UpscalingTechniqueAL::SupportsFrameGeneration() const
{
	return false;
}

Tr2UpscalingContextAL::Tr2UpscalingContextAL( Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2UpscalingAL::UpscalingContextParams params ) :
	m_setting( setting ),
	m_displayWidth( params.displayWidth ),
	m_displayHeight( params.displayHeight ),
	m_renderWidth( 0 ),
	m_renderHeight( 0 ),
	m_upscaling( 0 ),
	m_jitterIndex( 0 ),
	m_jitterX( 0.0f ),
	m_jitterY( 0.0f ),
	m_jitterXScale( 2.0f ),
	m_jitterYScale( -2.0f ),
	m_params( params )
{
	static uint32_t CONTEXT_ID = 0;
	m_id = CONTEXT_ID++;

	if( m_id == Tr2UpscalingAL::INVALID_CONTEXT_ID )
	{
		// Tr2UpscalingAL::INVALID_CONTEXT_ID is reserved, don't use it as an actual context id!
		m_id = 0;
		CONTEXT_ID = 0;
	}

	m_frameGeneration = frameGeneration && m_params.allowFramegen;
}

Tr2UpscalingContextAL::~Tr2UpscalingContextAL()
{
}

bool Tr2UpscalingContextAL::Reuse( Tr2UpscalingAL::UpscalingContextParams params )
{
	return m_params == params;
}

void Tr2UpscalingContextAL::SetupForReuse()
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
	width = m_params.displayWidth;
	height = m_params.displayHeight;
}

void Tr2UpscalingContextAL::GetJitter( float& x, float& y ) const
{
	x = m_jitterXScale * m_jitterX / (float)m_renderWidth;
	y = m_jitterYScale * m_jitterY / (float)m_renderHeight;
}

void Tr2UpscalingContextAL::SetHudLessTexture( const Tr2TextureAL* texture )
{
}

float Tr2UpscalingContextAL::GetMipLevelBias( bool temporal ) const
{
	float mipBias = log2( 1.0f / m_upscaling );
	if( temporal )
	{
		mipBias -= 1;
	}
	return mipBias;
}

float Tr2UpscalingContextAL::GetUpscalingAmount() const
{
	return m_upscaling;
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
