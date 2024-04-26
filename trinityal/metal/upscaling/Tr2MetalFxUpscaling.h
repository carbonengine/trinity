////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL
#include "include/upscaling/Tr2UpscalingAL.h"
#include <MetalFx/MetalFX.h>

class Tr2MetalFxUpscalingTechnique : public Tr2UpscalingTechniqueAL
{
public:
    Tr2MetalFxUpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration );
    ~Tr2MetalFxUpscalingTechnique();
    virtual void Destroy( Tr2RenderContextAL& renderContext ) override;

    virtual Tr2UpscalingAL::Result Setup() override;
    virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;

private:
    virtual Tr2UpscalingContextAL* CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) override;
};

class Tr2MetalFxUpscalingContext : public Tr2UpscalingContextAL
{
public:
    Tr2MetalFxUpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat );
    ~Tr2MetalFxUpscalingContext();

    virtual Tr2UpscalingAL::Result Setup( Tr2RenderContextAL& renderContext ) override;
    virtual bool IsTemporal() const override;
    virtual void UpdateJitter() override;
    virtual uint32_t GetDispatchRequirements() const override;
    
    virtual Tr2UpscalingAL::Result Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
    void CreateTemporalScaler( Tr2RenderContextAL& renderContext );
    void CreateSpatialScaler( Tr2RenderContextAL& renderContext );
    
    API_AVAILABLE(macos(13.0))
    id<MTLFXTemporalScaler> m_mfxTemporalScaler;
    
    API_AVAILABLE(macos(13.0))
    id<MTLFXSpatialScaler> m_mfxSpatialScaler;
    
    bool m_temporal;
    Tr2UpscalingAL::JitterSequence m_jitterSequence;

    void Destroy( Tr2RenderContextAL& renderContext );
    friend class Tr2MetalFxUpscalingTechnique;
};
#endif
