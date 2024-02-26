////////////////////////////////////////////////////////////////////////////////
//
// Created:        May 2023
// Copyright:      CCP 2023
//
#pragma once

#include "ITr2Upscaling.h"

#if TRINITY_PLATFORM == TRINITY_METAL
#include "Tr2UpscalingUtils.h"
#include <MetalFx/MetalFX.h>


namespace MetalUpscalingUtils {
    id<MTLTexture> GetMetalTexture(ITr2TextureProvider* texture);
    float GetUpscalingBasedOnSetting( const Tr2Upscaling::Setting& setting );
}

BLUE_CLASS( Tr2MetalFxSpatialUpscaling ) : public ITr2Upscaling
{
public:
    EXPOSE_TO_BLUE();

    Tr2MetalFxSpatialUpscaling( IRoot* lockobj = NULL );
    ~Tr2MetalFxSpatialUpscaling();
    
    bool IsApplicable() const;
    Tr2Upscaling::UpscalingType GetUpscalingType() const;
    const std::vector<Tr2Upscaling::Setting> GetAvailableSettings() const;

    void GetJitter( float& x, float& y );
	void GetJitterOffset( float& x, float& y );
    float GetMipLevelBias() const;
    float GetUpscaling() const;
    void GetRenderSize(uint32_t& width, uint32_t& height) const;

    void ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight );
    void Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext );
    void Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation );

private:
    API_AVAILABLE(macos(13.0))
    id<MTLFXSpatialScaler> m_mfxSpatialScaler;
    float m_upscaling;
    
    // have this static so it it persisted during the session
    static float s_debugUpscaling[Tr2Upscaling::COUNT];
    static bool s_debugMode;
    
    void SetDebugMode(bool m){
        Tr2MetalFxSpatialUpscaling::s_debugMode = m;
    }
    
    bool GetDebugMode(){
        return Tr2MetalFxSpatialUpscaling::s_debugMode;
    }
    
    void SetUltraQualityDebugValue(float v) {
        SetValue(Tr2Upscaling::Setting::ULTRA_QUALITY, v);
    }
    float GetUltraQualityDebugValue() {
        return Tr2MetalFxSpatialUpscaling::s_debugUpscaling[Tr2Upscaling::Setting::ULTRA_QUALITY];
    }
    void SetQualityDebugValue(float v) {
        SetValue(Tr2Upscaling::Setting::QUALITY, v);
    }
    float GetQualityDebugValue() {
        return Tr2MetalFxSpatialUpscaling::s_debugUpscaling[Tr2Upscaling::Setting::QUALITY];
    }
    void SetBalancedDebugValue(float v) {
        SetValue(Tr2Upscaling::Setting::BALANCED, v);
    }
    float GetBalancedDebugValue() {
        return Tr2MetalFxSpatialUpscaling::s_debugUpscaling[Tr2Upscaling::Setting::BALANCED];
    }
    void SetPerformanceDebugValue(float v) {
        SetValue(Tr2Upscaling::Setting::PERFORMANCE, v);
    }
    float GetPerformanceDebugValue() {
        return Tr2MetalFxSpatialUpscaling::s_debugUpscaling[Tr2Upscaling::Setting::PERFORMANCE];
    }
    void SetUltraPerformanceDebugValue(float v) {
        SetValue(Tr2Upscaling::Setting::ULTRA_PERFORMANCE, v);
    }
    float GetUltraPerformanceDebugValue() {
        return Tr2MetalFxSpatialUpscaling::s_debugUpscaling[Tr2Upscaling::Setting::ULTRA_PERFORMANCE];
    }
    
    void SetValue(Tr2Upscaling::Setting s, float value) {
        Tr2MetalFxSpatialUpscaling::s_debugUpscaling[s] = value;
        if( s == m_setting){
            m_upscaling = value;
        }
    }
    
    uint32_t m_displayWidth;
    uint32_t m_displayHeight;
    uint32_t m_renderWidth;
    uint32_t m_renderHeight;
    Tr2Upscaling::Setting m_setting;
};

BLUE_CLASS( Tr2MetalFxTemporalUpscaling ) : public ITr2Upscaling
{
public:
    EXPOSE_TO_BLUE();

    Tr2MetalFxTemporalUpscaling( IRoot* lockobj = NULL );
    ~Tr2MetalFxTemporalUpscaling();
    
    bool IsApplicable() const;
    Tr2Upscaling::UpscalingType GetUpscalingType() const;
    const std::vector<Tr2Upscaling::Setting> GetAvailableSettings() const;

    void GetJitter( float& x, float& y );
    void GetJitterOffset( float& x, float& y );
    float GetMipLevelBias() const;
    void GetRenderSize(uint32_t& width, uint32_t& height) const;

    void ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight );
    void Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext );
    void Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation );

private:
    API_AVAILABLE(macos(13.0))
    id<MTLFXTemporalScaler> m_mfxTemporalScaler;
    float m_upscaling;
    
    // have this static so it it persisted during the session
    static float s_debugUpscaling[Tr2Upscaling::Setting::COUNT];
    static bool s_debugMode;
    
    void SetDebugMode(bool m){
        Tr2MetalFxTemporalUpscaling::s_debugMode = m;
    }
    
    bool GetDebugMode(){
        return Tr2MetalFxTemporalUpscaling::s_debugMode;
    }
    
    void SetUltraQualityDebugValue(float v) {
        SetValue(Tr2Upscaling::Setting::ULTRA_QUALITY, v);
    }
    float GetUltraQualityDebugValue() {
        return Tr2MetalFxTemporalUpscaling::s_debugUpscaling[Tr2Upscaling::Setting::ULTRA_QUALITY];
    }
    void SetQualityDebugValue(float v) {
        SetValue(Tr2Upscaling::Setting::QUALITY, v);
    }
    float GetQualityDebugValue() {
        return Tr2MetalFxTemporalUpscaling::s_debugUpscaling[Tr2Upscaling::Setting::QUALITY];
    }
    void SetBalancedDebugValue(float v) {
        SetValue(Tr2Upscaling::Setting::BALANCED, v);
    }
    float GetBalancedDebugValue() {
        return Tr2MetalFxTemporalUpscaling::s_debugUpscaling[Tr2Upscaling::Setting::BALANCED];
    }
    void SetPerformanceDebugValue(float v) {
        SetValue(Tr2Upscaling::Setting::PERFORMANCE, v);
    }
    float GetPerformanceDebugValue() {
        return Tr2MetalFxTemporalUpscaling::s_debugUpscaling[Tr2Upscaling::Setting::PERFORMANCE];
    }
    void SetUltraPerformanceDebugValue(float v) {
        SetValue(Tr2Upscaling::Setting::ULTRA_PERFORMANCE, v);
    }
    float GetUltraPerformanceDebugValue() {
        return Tr2MetalFxTemporalUpscaling::s_debugUpscaling[Tr2Upscaling::Setting::ULTRA_PERFORMANCE];
    }
    
    void SetValue(Tr2Upscaling::Setting s, float value) {
        Tr2MetalFxTemporalUpscaling::s_debugUpscaling[s] = value;
        if( s == m_setting){
            m_upscaling = value;
        }
    }
    
    uint32_t m_jitterIndex;
    std::vector<std::pair<float, float>> m_jitterSequence;
    float m_jitterX;
    float m_jitterY;
    uint32_t m_renderWidth;
    uint32_t m_renderHeight;
    uint32_t m_displayWidth;
    uint32_t m_displayHeight;
    bool m_reset;
    Tr2Upscaling::Setting m_setting;
};
#else

#include "Tr2NoopUpscaling.h"
BLUE_CLASS( Tr2MetalFxSpatialUpscaling ) : public Tr2NoopUpscaling
{
public:
    EXPOSE_TO_BLUE();
    Tr2MetalFxSpatialUpscaling( IRoot* lockobj = NULL ){};
    ~Tr2MetalFxSpatialUpscaling(){};
    bool IsApplicable() const;
};

BLUE_CLASS( Tr2MetalFxTemporalUpscaling ) : public Tr2NoopUpscaling
{
public:
    EXPOSE_TO_BLUE();
    Tr2MetalFxTemporalUpscaling( IRoot* lockobj = NULL ){};
    ~Tr2MetalFxTemporalUpscaling(){};
    bool IsApplicable() const;
};
#endif
TYPEDEF_BLUECLASS( Tr2MetalFxSpatialUpscaling );
TYPEDEF_BLUECLASS( Tr2MetalFxTemporalUpscaling );
