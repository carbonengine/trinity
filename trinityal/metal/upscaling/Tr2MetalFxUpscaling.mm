////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"

#if( TRINITY_PLATFORM == TRINITY_METAL )
#include <MetalFx/MetalFX.h>

#include "Tr2MetalFxUpscaling.h"
#include "include/upscaling/Tr2UpscalingAL.h"
#include "../Tr2RenderContextMetal.h"
#include "../MetalContext.h"
#include "../Tr2TextureALMetal.h"

namespace MetalUpscalingUtils
{
    id<MTLTexture> GetMetalTexture(Tr2TextureAL* texture)
    {
        if( texture && texture->TrinityALImpl_GetObject())
        {
            return texture->TrinityALImpl_GetObject()->GetMetalTexture();
        }
        return nil;
    }
}

namespace
{

template <typename Item, typename Processor>
class WorkQueue
{
public:
    WorkQueue( Processor processor )
    :   m_processor( processor )
    {
        std::thread( [this]() {
            Worker();
        } ).detach();
    }
    
    void Add( const std::shared_ptr<Item>& item )
    {
        std::unique_lock lock( m_mutex );
        m_queue.push_back( item );
        m_added.notify_one();
    }
    
    void Process( const std::shared_ptr<Item>& item )
    {
        m_processor( item );
    }
private:
    void Worker()
    {
        for( ;; )
        {
            m_processor( GetItem() );
        }
    }
    
    std::shared_ptr<Item> GetItem()
    {
        for( ;; )
        {
            std::unique_lock scope( m_mutex );
            if( !m_queue.empty() )
            {
                auto item = m_queue.front();
                m_queue.erase( m_queue.begin() );
                return item;
            }
            m_added.wait( scope, [this] { return !m_queue.empty(); } );
        }
    }
    
    std::mutex m_mutex;
    std::condition_variable m_added;
    std::vector<std::shared_ptr<Item>> m_queue;
    Processor m_processor;
};


}

Tr2MetalFxUpscalingTechnique::Tr2MetalFxUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ):
    Tr2UpscalingTechniqueAL( renderContext, technique, setting, frameGeneration, adapter ),
    m_temporal( false )
{
    if( @available(macOS 13.0, *) )
    {
        TrinityALImpl::MetalContext* metalContext = m_renderContext.GetPrimaryRenderContext().GetMetalContext();
        auto device = metalContext->GetDevice();
     
        // need to check if system supports temporal upscalers
        m_temporal = [MTLFXTemporalScalerDescriptor supportsDevice:device];
    }
    this->SanitizeState();
}

Tr2MetalFxUpscalingTechnique::~Tr2MetalFxUpscalingTechnique()
{
    
}

bool Tr2MetalFxUpscalingTechnique::IsTemporal() const
{
	return m_temporal;
}

 bool Tr2MetalFxUpscalingTechnique::IsAvailable() const
{
    if( @available(macOS 13.0, *) )
    {
        return true;
    }
    CCP_LOGNOTICE( "MetalFX upscaling is not supported for MacOS < 13.0" );
    return false;
} 

Tr2UpscalingContextAL* Tr2MetalFxUpscalingTechnique::CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params )
{
    return new Tr2MetalFxUpscalingContext( m_setting, m_temporal, params );
}

std::vector<Tr2UpscalingAL::Setting> Tr2MetalFxUpscalingTechnique::GetAvailableSettings() const {
    if( m_temporal )
	{
		return {
			Tr2UpscalingAL::Setting::ULTRA_QUALITY,
			Tr2UpscalingAL::Setting::QUALITY,
			Tr2UpscalingAL::Setting::BALANCED,
			Tr2UpscalingAL::Setting::PERFORMANCE,
			Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE
		};
	}
    return {
        Tr2UpscalingAL::Setting::QUALITY,
        Tr2UpscalingAL::Setting::BALANCED,
        Tr2UpscalingAL::Setting::PERFORMANCE,
        Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE
    };
}

Tr2MetalFxUpscalingContext::Tr2MetalFxUpscalingContext( Tr2UpscalingAL::Setting setting, bool temporal, Tr2UpscalingAL::UpscalingContextParams params ) :
    Tr2UpscalingContextAL( setting, false, params ),
    m_setup( false ),
    m_renderContext( params.renderContext )
{
    if( @available(macOS 13.0, *) )
	{
		m_mfxSpatialScaler = nil;
		
		m_jitterXScale = 1.0f;
		m_jitterYScale = -1.0f;
		m_temporal = temporal;
        auto queue = m_renderContext.GetPrimaryRenderContext().GetMetalWorkQueue();
        if( queue->IsSilicon() )
        {
            SetSiliconUpscalingAmount();
        }
        else
        {
            SetIntelUpscalingAmount();
        }
		
		m_renderWidth = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayWidth, m_upscaling );
		m_renderHeight = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayHeight, m_upscaling );
		
		if( m_temporal )
		{
			CreateTemporalScaler();
			m_jitterSequence = Tr2UpscalingAL::GenerateHaltonSequence( 8 * m_upscaling * m_upscaling, 2, 3 );
		}
		CreateSpatialScaler();
		
		if( m_mfxSpatialScaler == nil )
		{
			return;
		}
		m_setup = true;
	}
}

Tr2MetalFxUpscalingContext::~Tr2MetalFxUpscalingContext() {
	
	if( @available(macos 13.0, *) )
	{
		if( m_temporalScaler )
		{
			m_temporalScaler->canceled = true;
            if( m_temporalScaler->created )
            {
                m_renderContext.ReleaseLater( m_temporalScaler->temporalScaler );
            }
		}
		m_mfxSpatialScaler = nil;
	}
}

void Tr2MetalFxUpscalingContext::SetSiliconUpscalingAmount() 
{
    switch( m_setting )
    {
    case Tr2UpscalingAL::Setting::ULTRA_QUALITY:
        m_upscaling = 1.1; // ultra quality is only available on temporal upscaler
        break;
    case Tr2UpscalingAL::Setting::QUALITY:
        m_upscaling = 1.4;
        break;
    case Tr2UpscalingAL::Setting::BALANCED:
        m_upscaling = 1.7;
        break;
    case Tr2UpscalingAL::Setting::PERFORMANCE:
        m_upscaling = 2.0;
        break;
    case Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE:
        m_upscaling = 2.3;
        break;
    default:
        m_upscaling = 1.0;
        break;
    }
}

void Tr2MetalFxUpscalingContext::SetIntelUpscalingAmount() 
{
    if( m_temporal )
    {
        switch( m_setting )
        {
        case Tr2UpscalingAL::Setting::ULTRA_QUALITY:
            m_upscaling = 1.1; // ultra quality is only available on temporal upscaler
            break;
        case Tr2UpscalingAL::Setting::QUALITY:
            m_upscaling = 1.4;
            break;
        case Tr2UpscalingAL::Setting::BALANCED:
            m_upscaling = 1.6;
            break;
        case Tr2UpscalingAL::Setting::PERFORMANCE:
            m_upscaling = 1.8;
            break;
        case Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE:
            m_upscaling = 2.0;
            break;
        default:
            m_upscaling = 1.0;
            break;
        }
    }
    else
    {
        switch( m_setting )
        {
        case Tr2UpscalingAL::Setting::QUALITY:
            m_upscaling = 1.25;
            break;
        case Tr2UpscalingAL::Setting::BALANCED:
            m_upscaling =  1.5;
            break;
        case Tr2UpscalingAL::Setting::PERFORMANCE:
            m_upscaling =  1.75;
            break;
        case Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE:
            m_upscaling =  2.0;
            break;
        default:
            m_upscaling = 1.0;
            break;
        }
    }
}

void Tr2MetalFxUpscalingContext::CreateTemporalScaler()
{
    if( @available(macOS 13.0, *) )
    {
        static WorkQueue<TemporalScaler, void (*)( const std::shared_ptr<TemporalScaler>& )> s_queue( []( auto scaler ) {
            if( !scaler->canceled )
            {
                scaler->temporalScaler = [scaler->desc newTemporalScalerWithDevice:scaler->device];
                if( scaler->temporalScaler == nil )
                {
                    CCP_LOGERR("Could not create a MetalFX Temporal Scaler");
                    return;
                }
                scaler->temporalScaler.motionVectorScaleX = scaler->desc.inputWidth;
                scaler->temporalScaler.motionVectorScaleY = scaler->desc.inputHeight;
                scaler->created = true;
            }
        } );
        
        auto& renderContext = m_params.renderContext;

        TrinityALImpl::MetalContext* metalContext = renderContext.GetPrimaryRenderContext().GetMetalContext();
        auto pixelFormat = metalContext->m_utils->GetMTLPixelFormat( m_params.sourceFormat );
        auto depthPixelFormat = metalContext->m_utils->GetMTLPixelFormat( Tr2RenderContextEnum::ConvertDepthStencilFormat( m_params.depthFormat ) );
        // motion pixel format is always the same, true as of April 2024
        auto motionPixelFormat = metalContext->m_utils->GetMTLPixelFormat( Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_FLOAT );
        
        auto device = metalContext->GetDevice();
        
        MTLFXTemporalScalerDescriptor* desc = [MTLFXTemporalScalerDescriptor new];
        desc.inputWidth = m_renderWidth;
        desc.inputHeight = m_renderHeight;
        desc.outputWidth = m_displayWidth;
        desc.outputHeight = m_displayHeight;
        desc.colorTextureFormat = pixelFormat;
        desc.depthTextureFormat = depthPixelFormat;
        desc.motionTextureFormat = motionPixelFormat;
        desc.outputTextureFormat = pixelFormat;
        
        m_temporalScaler = std::make_shared<TemporalScaler>();
        m_temporalScaler->desc = desc;
        m_temporalScaler->device = device;
        
        s_queue.Add( m_temporalScaler );
    }
}

void Tr2MetalFxUpscalingContext::CreateSpatialScaler()
{
    if( @available(macOS 13.0, *) )
    {
        auto& renderContext = m_params.renderContext;

        TrinityALImpl::MetalContext* metalContext = renderContext.GetPrimaryRenderContext().GetMetalContext();
        auto pixelFormat = metalContext->m_utils->GetMTLPixelFormat( m_params.sourceFormat );
        auto device = metalContext->GetDevice();
        
        MTLFXSpatialScalerDescriptor* desc = [MTLFXSpatialScalerDescriptor new];
        
        desc.inputWidth = m_renderWidth;
        desc.inputHeight = m_renderHeight;
        desc.outputWidth = m_displayWidth;
        desc.outputHeight = m_displayHeight;
        
        desc.colorTextureFormat = pixelFormat;
        desc.outputTextureFormat = pixelFormat;
        desc.colorProcessingMode = MTLFXSpatialScalerColorProcessingMode::MTLFXSpatialScalerColorProcessingModeLinear;
        
        m_mfxSpatialScaler = [desc newSpatialScalerWithDevice:device];
        
        if( m_mfxSpatialScaler == nil )
        {
            CCP_LOGERR("Could not create a MetalFX Spatial Scaler");
        }
    }
}

bool Tr2MetalFxUpscalingContext::HasSharpening() const {
    return false;
}

void Tr2MetalFxUpscalingContext::UpdateJitter()
{
    if( m_temporal && m_temporalScaler && m_temporalScaler->created )
    {
        m_jitterX = m_jitterSequence[m_jitterIndex].first;
        m_jitterY = m_jitterSequence[m_jitterIndex].second;

        m_jitterIndex = ++m_jitterIndex % m_jitterSequence.size();
    }
}

uint32_t Tr2MetalFxUpscalingContext::GetDispatchRequirements() const
{
    if( m_temporal )
    {
        return Tr2UpscalingAL::DispatchRequirements::VELOCITY & Tr2UpscalingAL::DispatchRequirements::DEPTH;
    }
    return 0;
}

Tr2UpscalingAL::Result Tr2MetalFxUpscalingContext::Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
    if( @available( macOS 13.0, * ) )
    {
        if( !m_setup )
        {
            return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
        }

        if( !AreDispatchParametersValid( dispatchParameters ) )
        {
            return Tr2UpscalingAL::Result::INCORRECT_INPUT;
        }

        bool reset = dispatchParameters.reset;
        
        auto& renderContext = m_params.renderContext;

        if( m_temporalScaler && m_temporalScaler->created )
        {
            if( m_mfxSpatialScaler )
            {
                m_reset = true;
                m_mfxSpatialScaler = nil;
            }
            
            auto scaler = m_temporalScaler->temporalScaler;
            auto queue = renderContext.GetPrimaryRenderContext().GetMetalWorkQueue();
            queue->EndEncoder();
            
            scaler.reset = m_reset;
            scaler.colorTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.input );
            scaler.depthTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.depth );
            scaler.motionTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.velocity );
            scaler.outputTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.output );
            scaler.depthReversed = true;
            scaler.jitterOffsetX = m_jitterX;
            scaler.jitterOffsetY = m_jitterY;
            
            [scaler encodeToCommandBuffer:queue->GetCommandBuffer()];
            
            m_reset = false;
            return Tr2UpscalingAL::Result::OK;
        }
        
        if( m_mfxSpatialScaler != nil )
        {
            auto queue = renderContext.GetPrimaryRenderContext().GetMetalWorkQueue();
            queue->EndEncoder();
            
            m_mfxSpatialScaler.colorTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.input );
            m_mfxSpatialScaler.outputTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.output );
            
            [m_mfxSpatialScaler encodeToCommandBuffer:queue->GetCommandBuffer()];
        }
        else
        {
            CCP_LOGERR( "Trying to dispatch upscaling without temporal or spatial scaler..." );
            return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
        }
    }
    return Tr2UpscalingAL::Result::OK;
}

#endif
