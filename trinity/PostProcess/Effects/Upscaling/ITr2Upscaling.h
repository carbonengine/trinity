////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#pragma once
#include "../../Tr2PostProcessRenderInfo.h"
#include "../trinityal/include/Tr2Streamline.h"

namespace Tr2Upscaling
{
	enum Setting
	{
		ULTRA_PERFORMANCE,
		PERFORMANCE,
		BALANCED,
		QUALITY,
		ULTRA_QUALITY,
		COUNT
	};

	enum Technique
	{
		UPSCALING_TECHNIQUE_NONE,
		UPSCALING_TECHNIQUE_FSR1,
		UPSCALING_TECHNIQUE_FSR2,
		UPSCALING_TECHNIQUE_DLSS,
		UPSCALING_TECHNIQUE_XESS,
		UPSCALING_TECHNIQUE_METALFX_SPATIAL,
		UPSCALING_TECHNIQUE_METALFX_TEMPORAL
	};

	struct Textures
	{
		ITr2TextureProvider* input;
		ITr2TextureProvider* opaqueOnly;
		ITr2TextureProvider* output;
		ITr2TextureProvider* depth;
		ITr2TextureProvider* motion;
		ITr2TextureProvider* exposure;
		ITr2TextureProvider* reactive;
	};

	struct SceneInformation
	{
		Matrix projection;
		Matrix reprojection;
		Matrix view;
		Vector3 cameraPos;
		float frontClip;
		float backClip;
		float fieldOfView;
		float aspectRatio; 
		bool reset;
	};

	enum UpscalingType
	{
		UT_SPATIAL,
		UT_TEMPORAL,
		UT_UNKNOWN, // special case for instances where we actually don't know what we will get...
		UT_NOT_APPLICABLE
	};

    struct UpscalingSetupContext{
        Tr2RenderContextEnum::PixelFormat sourcePixelFormat;
        Tr2RenderContextEnum::PixelFormat motionVectorPixelFormat;
        Tr2RenderContextEnum::PixelFormat depthPixelFormat;
    };

	extern Be::VarChooser UpscalingTypeChooser[];
}

BLUE_INTERFACE( ITr2Upscaling ) : IRoot
{
public:
	virtual void GetJitter( float& x, float& y ) = 0;
	virtual void GetJitterOffset( float& x, float& y ) = 0;

	virtual float GetMipLevelBias() const = 0;
	virtual void GetRenderSize(uint32_t& width, uint32_t& height) const = 0;

	virtual Tr2Upscaling::UpscalingType GetUpscalingType() const = 0;
	virtual bool IsDirty() const { return false; }

	virtual void ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight ) = 0;
	virtual void Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext ) = 0;
	virtual void Dispatch( Tr2RenderContext & renderContext, Tr2PostProcessRenderInfo & renderInfo, Tr2Upscaling::Textures & textures, const Tr2Upscaling::SceneInformation& sceneInformation ) = 0;
	
	virtual void SetUseFrameGeneration( bool enable ){};
	virtual bool GetUseFrameGeneration() const { return false; };
	
	virtual bool IsApplicable() const = 0;
	virtual bool NeedsExposureTexture() const { return false; }
	virtual bool NeedsReactiveTexture() const { return false; }
	virtual const std::vector<Tr2Upscaling::Setting> GetAvailableSettings() const = 0;

	virtual bool SupportsFrameGeneration() const { return false; };
};

