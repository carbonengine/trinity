////////////////////////////////////////////////////////////////////////////////
//
// Created:        May 2023
// Copyright:      CCP 2023
//
#include "StdAfx.h"
#include "Tr2MetalFxUpscaling.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include <MetalFx/MetalFX.h>
#include "../trinityal/metal/Tr2RenderContextMetal.h"
#include "../trinityal/metal/MetalContext.h"
#include "../trinityal/metal/Tr2TextureALMetal.h"
#include "Tr2UpscalingUtils.h"

namespace MetalUpscalingUtils {
    id<MTLTexture> GetMetalTexture(ITr2TextureProvider* texture)
    {
        if( texture && texture->GetTexture() && texture->GetTexture()->TrinityALImpl_GetObject())
        {
            return texture->GetTexture()->TrinityALImpl_GetObject()->GetMetalTexture();
        }
        return nil;
    }
    
    float GetUpscalingBasedOnSetting( const Tr2Upscaling::Setting& setting )
    {
        switch(setting){
            case  Tr2Upscaling::ULTRA_PERFORMANCE:
                return 2.0f;
                break;
            case Tr2Upscaling::PERFORMANCE:
                return 1.75f;
                break;
            case Tr2Upscaling::BALANCED:
                return 1.5f;
                break;
            case Tr2Upscaling::QUALITY:
                return 1.25f;
                break;
            case Tr2Upscaling::ULTRA_QUALITY:
               // ultra quality is 1.0
            default:
                return 1.0f;
                break;
        }
    }
}

Tr2MetalFxSpatialUpscaling::Tr2MetalFxSpatialUpscaling( IRoot* lockobj ):
    m_upscaling(1.0)
{
    if( @available(macOS 13.0, *))
    {
        m_mfxSpatialScaler = nil;
    }
}

Tr2MetalFxSpatialUpscaling::~Tr2MetalFxSpatialUpscaling()
{
    if( @available(macOS 13.0, *))
    {
        m_mfxSpatialScaler = nil;
    }
}

bool Tr2MetalFxSpatialUpscaling::IsApplicable() const
{
    // Metal is only available on 13.0 and above
    if( @available(macOS 13.0, *))
    {
        // Completely inverse to the temporal one, if we support temporal upscaling, then spatial is not available
        USE_MAIN_THREAD_RENDER_CONTEXT();
        TrinityALImpl::MetalContext* metalContext = renderContext.GetPrimaryRenderContext().GetMetalContext();
        auto device = metalContext->GetDevice();

        return ![MTLFXTemporalScalerDescriptor supportsDevice:device];
    }
    return false;
}

Tr2Upscaling::UpscalingType Tr2MetalFxSpatialUpscaling::GetUpscalingType() const
{
    return Tr2Upscaling::UT_SPATIAL;
}

void Tr2MetalFxSpatialUpscaling::GetJitter( float& x, float& y )
{
    x = 0;
    y = 0;
}

void Tr2MetalFxSpatialUpscaling::GetJitterOffset( float& x, float& y )
{
	x = 0;
	y = 0;
}

float Tr2MetalFxSpatialUpscaling::GetMipLevelBias() const
{
    return log2f(1.0/m_upscaling);
}

float Tr2MetalFxSpatialUpscaling::GetUpscaling() const
{
    return m_upscaling;
}

void Tr2MetalFxSpatialUpscaling::GetRenderSize(uint32_t& width, uint32_t& height) const{
    width = m_renderWidth;
    height = m_renderHeight;
}

const std::vector<Tr2Upscaling::Setting> Tr2MetalFxSpatialUpscaling::GetAvailableSettings() const
{
    static std::vector<Tr2Upscaling::Setting> spatialSettings {
        Tr2Upscaling::ULTRA_PERFORMANCE,
        Tr2Upscaling::PERFORMANCE,
        Tr2Upscaling::BALANCED,
        Tr2Upscaling::QUALITY};
    return spatialSettings;
}

void Tr2MetalFxSpatialUpscaling::ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight )
{
    m_displayWidth = displayWidth;
    m_displayHeight = displayHeight;
    m_upscaling = MetalUpscalingUtils::GetUpscalingBasedOnSetting(setting);
    m_renderWidth = UpscalingUtils::ConvertDisplaySizeToRenderSize(m_displayWidth, m_upscaling);
    m_renderHeight = UpscalingUtils::ConvertDisplaySizeToRenderSize(m_displayHeight, m_upscaling);
}

void Tr2MetalFxSpatialUpscaling::Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext )
{
    if( @available(macOS 13.0, *))
    {
        TrinityALImpl::MetalContext* metalContext = renderContext.GetPrimaryRenderContext().GetMetalContext();
        auto pixelFormat = metalContext->m_utils->GetMTLPixelFormat(setupContext.sourcePixelFormat);
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

void Tr2MetalFxSpatialUpscaling::Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation )
{
    if( @available(macOS 13.0, *))
    {
        if( m_mfxSpatialScaler == nil )
        {
            CCP_LOGERR("Trying to render with a nil MetalFX Spatial Scaler");
            return;
        }
        auto queue = renderContext.GetPrimaryRenderContext().GetMetalWorkQueue();
        
        m_mfxSpatialScaler.colorTexture = MetalUpscalingUtils::GetMetalTexture(textures.input);
        m_mfxSpatialScaler.outputTexture = MetalUpscalingUtils::GetMetalTexture(textures.output);
        
        [m_mfxSpatialScaler encodeToCommandBuffer:queue->GetCommandBuffer()];
    }
}


Tr2MetalFxTemporalUpscaling::Tr2MetalFxTemporalUpscaling( IRoot* lockobj ):
    m_upscaling(1.0),
    m_jitterIndex(0),
    m_jitterX(0.0f),
    m_jitterY(0.0f),
    m_renderWidth(0.0f),
    m_renderHeight(0.0f),
    m_reset(false)
{
    if( @available(macOS 13.0, *))
    {
        m_mfxTemporalScaler = nil;
    }

}

Tr2MetalFxTemporalUpscaling::~Tr2MetalFxTemporalUpscaling()
{
    if( @available(macOS 13.0, *))
    {
        m_mfxTemporalScaler = nil;
    }
}

bool Tr2MetalFxTemporalUpscaling::IsApplicable() const
{
    // Metal is only available on 13.0 and above
    if( @available(macOS 13.0, *))
    {
        // temporal scalers are only available on silicon hardware, need to check if it supported
        USE_MAIN_THREAD_RENDER_CONTEXT();
        TrinityALImpl::MetalContext* metalContext = renderContext.GetPrimaryRenderContext().GetMetalContext();
        auto device = metalContext->GetDevice();

        return [MTLFXTemporalScalerDescriptor supportsDevice:device];
    }
    return false;
}

Tr2Upscaling::UpscalingType Tr2MetalFxTemporalUpscaling::GetUpscalingType() const
{
    return Tr2Upscaling::UT_TEMPORAL;
}

void Tr2MetalFxTemporalUpscaling::GetJitter( float& x, float& y )
{
    m_jitterX = m_jitterSequence[m_jitterIndex].first;
    m_jitterY = m_jitterSequence[m_jitterIndex].second;
	m_jitterIndex = ++m_jitterIndex % m_jitterSequence.size();
    
    x = m_jitterX / ( float ) m_renderWidth;
    y = -m_jitterY / ( float ) m_renderHeight;
}


void Tr2MetalFxTemporalUpscaling::GetJitterOffset( float& x, float& y )
{
	x = m_jitterX;
	y = m_jitterY;
}

float Tr2MetalFxTemporalUpscaling::GetMipLevelBias() const
{
    return log2f(1.0/m_upscaling) - 1.0f;
}

float Tr2MetalFxTemporalUpscaling::GetUpscaling() const
{
    return m_upscaling;
}

void Tr2MetalFxTemporalUpscaling::GetRenderSize(uint32_t& width, uint32_t& height) const{
    width = m_renderWidth;
    height = m_renderHeight;
}

const std::vector<Tr2Upscaling::Setting> Tr2MetalFxTemporalUpscaling::GetAvailableSettings() const
{
    static std::vector<Tr2Upscaling::Setting> spatialSettings {
        Tr2Upscaling::ULTRA_PERFORMANCE,
        Tr2Upscaling::PERFORMANCE,
        Tr2Upscaling::BALANCED,
        Tr2Upscaling::QUALITY,
        Tr2Upscaling::ULTRA_QUALITY,
    };
    return spatialSettings;
}

void Tr2MetalFxTemporalUpscaling::ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight)
{
    m_displayWidth = displayWidth;
    m_displayHeight = displayHeight;
    m_upscaling = MetalUpscalingUtils::GetUpscalingBasedOnSetting(setting);
    m_renderWidth = UpscalingUtils::ConvertDisplaySizeToRenderSize(m_displayWidth, m_upscaling);
    m_renderHeight = UpscalingUtils::ConvertDisplaySizeToRenderSize(m_displayHeight, m_upscaling);
    m_jitterSequence = Jitter::GenerateHaltonSequence(32, 2, 3);
}

void Tr2MetalFxTemporalUpscaling::Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext )
{
    if( @available(macOS 13.0, *))
    {
        TrinityALImpl::MetalContext* metalContext = renderContext.GetPrimaryRenderContext().GetMetalContext();
        auto pixelFormat = metalContext->m_utils->GetMTLPixelFormat(setupContext.sourcePixelFormat);
        auto depthPixelFormat = metalContext->m_utils->GetMTLPixelFormat(setupContext.depthPixelFormat);
        auto motionPixelFormat = metalContext->m_utils->GetMTLPixelFormat(setupContext.motionVectorPixelFormat);
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


void Tr2MetalFxTemporalUpscaling::Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation )
{
    if( @available(macOS 13.0, *))
    {
        
        if( m_mfxTemporalScaler == nil )
        {
            CCP_LOGERR("Trying to render with a nil MetalFX Temporal Scaler");
            return;
        }
        
        auto queue = renderContext.GetPrimaryRenderContext().GetMetalWorkQueue();
        
        m_mfxTemporalScaler.reset = m_reset;
        m_mfxTemporalScaler.colorTexture = MetalUpscalingUtils::GetMetalTexture(textures.input);
        m_mfxTemporalScaler.depthTexture = MetalUpscalingUtils::GetMetalTexture(textures.depth);
        m_mfxTemporalScaler.motionTexture = MetalUpscalingUtils::GetMetalTexture(textures.motion);
        m_mfxTemporalScaler.outputTexture = MetalUpscalingUtils::GetMetalTexture(textures.output);
        m_mfxTemporalScaler.depthReversed = true;
        m_mfxTemporalScaler.jitterOffsetX = m_jitterX;
        m_mfxTemporalScaler.jitterOffsetY = m_jitterY;
        
        [m_mfxTemporalScaler encodeToCommandBuffer:queue->GetCommandBuffer()];
        
        m_reset = false;
    }
}


#else
bool Tr2MetalFxSpatialUpscaling::IsApplicable() const
{
    return false;
}

bool Tr2MetalFxTemporalUpscaling::IsApplicable() const
{
    return false;
}
#endif
