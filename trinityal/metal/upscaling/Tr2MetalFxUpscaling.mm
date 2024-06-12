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

Tr2MetalFxUpscalingTechnique::Tr2MetalFxUpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration ):
    Tr2UpscalingTechniqueAL( technique, setting, frameGeneration )
{
    this->SanitizeState();
}

Tr2MetalFxUpscalingTechnique::~Tr2MetalFxUpscalingTechnique()
{
    
}

void Tr2MetalFxUpscalingTechnique::Destroy( Tr2RenderContextAL& renderContext )
{
}

 bool Tr2MetalFxUpscalingTechnique::IsAvailable( Tr2RenderContextAL& renderContext ) const
{
    if( @available(macOS 13.0, *) )
    {
        return true;
    }
    CCP_LOGNOTICE( "MetalFX upscaling is not supported for MacOS < 13.0" );
    return false;
} 

Tr2UpscalingContextAL* Tr2MetalFxUpscalingTechnique::CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat )
{
    return new Tr2MetalFxUpscalingContext( displayWidth, displayHeight, m_setting, m_frameGeneration, sourceFormat, depthFormat );
}

std::vector<Tr2UpscalingAL::Setting> Tr2MetalFxUpscalingTechnique::GetAvailableSettings() const {
    return {
        Tr2UpscalingAL::Setting::QUALITY,
        Tr2UpscalingAL::Setting::BALANCED,
        Tr2UpscalingAL::Setting::PERFORMANCE,
        Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE
    };
}


Tr2MetalFxUpscalingContext::Tr2MetalFxUpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) : Tr2UpscalingContextAL(displayWidth, displayHeight, setting, frameGeneration, sourceFormat, depthFormat),
    m_temporal( false ),
    m_setup( false )
{
    if( @available(macOS 13.0, *))
    {
        m_mfxTemporalScaler = nil;
        m_mfxSpatialScaler = nil;
    }
    m_jitterXScale = 1.0f;
    m_jitterYScale = -1.0f;
}

Tr2MetalFxUpscalingContext::~Tr2MetalFxUpscalingContext()
{
    if( @available( macos 13.0, * ) )
    {
        m_mfxSpatialScaler = nil;
        m_mfxTemporalScaler = nil;
    }
}

Tr2UpscalingAL::Result Tr2MetalFxUpscalingContext::Setup( Tr2RenderContextAL& renderContext )
{
    if( @available(macOS 13.0, *) )
    {
        TrinityALImpl::MetalContext* metalContext = renderContext.GetPrimaryRenderContext().GetMetalContext();
        auto device = metalContext->GetDevice();
     
        // temporal scalers are only available on silicon hardware, need to check if it supported
        m_temporal = [MTLFXTemporalScalerDescriptor supportsDevice:device];
        
        switch( m_setting ){
            case Tr2UpscalingAL::Setting::QUALITY:
                m_upscaling = m_temporal ? 1.5 : 1.25;
                break;
            case Tr2UpscalingAL::Setting::BALANCED:
                m_upscaling = m_temporal ? 2.0 : 1.5;
                break;
            case Tr2UpscalingAL::Setting::PERFORMANCE:
                m_upscaling = m_temporal ? 2.5 : 1.75;
                break;
            case Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE:
                m_upscaling = m_temporal ? 3.0 : 2.0;
                break;
            default:
                m_upscaling = 1.0;
                break;
        }
        
        m_renderWidth = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayWidth, m_upscaling );
        m_renderHeight = Tr2UpscalingAL::ConvertDisplaySizeToRenderSize( m_displayHeight, m_upscaling );
        
        if( m_temporal )
        {
            CreateTemporalScaler( renderContext );
            m_jitterSequence = Tr2UpscalingAL::GenerateHaltonSequence( 8 * m_upscaling * m_upscaling, 2, 3 );
        }
        else
        {
            CreateSpatialScaler( renderContext );
        }
        
        if( m_mfxSpatialScaler == nil && m_mfxTemporalScaler == nil )
        {
            return Tr2UpscalingAL::Result::TECHNIQUE_NOT_SUPPORTED;
        }
        m_setup = true;
        return Tr2UpscalingAL::Result::OK;
    }
    return Tr2UpscalingAL::Result::TECHNIQUE_NOT_SUPPORTED;
}


void Tr2MetalFxUpscalingContext::CreateTemporalScaler( Tr2RenderContextAL& renderContext )
{
    if( @available(macOS 13.0, *) )
    {
        TrinityALImpl::MetalContext* metalContext = renderContext.GetPrimaryRenderContext().GetMetalContext();
        auto pixelFormat = metalContext->m_utils->GetMTLPixelFormat( m_sourceFormat );
        auto depthPixelFormat = metalContext->m_utils->GetMTLPixelFormat( Tr2RenderContextEnum::ConvertDepthStencilFormat( m_depthFormat ) );        
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
        
        m_mfxTemporalScaler = [desc newTemporalScalerWithDevice:device];
        
        if( m_mfxTemporalScaler == nil )
        {
            CCP_LOGERR("Could not create a MetalFX Temporal Scaler");
            return;
        }
        
        m_mfxTemporalScaler.motionVectorScaleX = m_renderWidth;
        m_mfxTemporalScaler.motionVectorScaleY = m_renderHeight;
        m_reset = true;
    }
}

void Tr2MetalFxUpscalingContext::CreateSpatialScaler( Tr2RenderContextAL& renderContext )
{
    if( @available(macOS 13.0, *) )
    {
        TrinityALImpl::MetalContext* metalContext = renderContext.GetPrimaryRenderContext().GetMetalContext();
        auto pixelFormat = metalContext->m_utils->GetMTLPixelFormat( m_sourceFormat );
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

bool Tr2MetalFxUpscalingContext::IsTemporal() const
{
    return m_temporal;
}

bool Tr2MetalFxUpscalingContext::HasSharpening() const {
    return false;
}

void Tr2MetalFxUpscalingContext::UpdateJitter()
{
    if( m_temporal )
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

Tr2UpscalingAL::Result Tr2MetalFxUpscalingContext::Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters )
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

        if( m_mfxSpatialScaler != nil )
        {
            auto queue = renderContext.GetPrimaryRenderContext().GetMetalWorkQueue();
            queue->EndEncoder();
            
            m_mfxSpatialScaler.colorTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.input );
            m_mfxSpatialScaler.outputTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.output );
            
            [m_mfxSpatialScaler encodeToCommandBuffer:queue->GetCommandBuffer()];
        }
        else if( m_mfxTemporalScaler != nil )
        {
            auto queue = renderContext.GetPrimaryRenderContext().GetMetalWorkQueue();
            queue->EndEncoder();
            
            m_mfxTemporalScaler.reset = m_reset;
            m_mfxTemporalScaler.colorTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.input );
            m_mfxTemporalScaler.depthTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.depth );
            m_mfxTemporalScaler.motionTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.velocity );
            m_mfxTemporalScaler.outputTexture = MetalUpscalingUtils::GetMetalTexture( dispatchParameters.output );
            m_mfxTemporalScaler.depthReversed = true;
            m_mfxTemporalScaler.jitterOffsetX = m_jitterX;
            m_mfxTemporalScaler.jitterOffsetY = m_jitterY;
            
            [m_mfxTemporalScaler encodeToCommandBuffer:queue->GetCommandBuffer()];
            
            m_reset = false;
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
