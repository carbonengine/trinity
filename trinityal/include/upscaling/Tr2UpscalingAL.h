////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#include "../Tr2RenderContextEnum.h"

class Tr2RenderContextAL;
class Tr2TextureAL;

namespace Tr2UpscalingAL
{
	enum Technique
	{
		NONE,
		FSR1,
		FSR2,
		FSR3,
		DLSS,
		XESS,
		METALFX
	};	

	enum Setting
	{
		NATIVE = 1 << 0,
		ULTRA_QUALITY = 1 << 1,
		QUALITY = 1 << 2,
		BALANCED = 1 << 3,
		PERFORMANCE = 1 << 4,
		ULTRA_PERFORMANCE = 1 << 5
	};

	enum Result
	{
		OK,
		TECHNIQUE_NOT_SUPPORTED,
		HARDWARE_NOT_SUPPORTED,
		CONTEXT_SETUP_FAILED,
		INCORRECT_INPUT
	};

	struct DispatchParameters
	{
		Tr2TextureAL* input;
		Tr2TextureAL* opaqueOnly;
		Tr2TextureAL* output;
		Tr2TextureAL* depth;
		Tr2TextureAL* velocity;
		Tr2TextureAL* exposure;
		Tr2TextureAL* reactive;

		float frontClip;
		float backClip;
		float fieldOfView;
		float aspectRatio;
		float frameTimeDelta;
		float preExposure;
		float view[16];
		float projection[16];
		float invProjection[16];
		float clipToPrevClip[16];
		float prevClipToClip[16];
	};
	
	enum DispatchRequirements
	{
		VELOCITY = 1 << 0,
		OPAQUE_ONLY = 1 << 1,
		DEPTH = 1 << 2,
		REACTIVE = 1 << 3, 
		OPTIONAL_EXPOSURE = 1 << 4,
	};

	typedef std::vector<std::pair<float, float>> JitterSequence;
	JitterSequence GenerateHaltonSequence( uint32_t totalPhases, uint32_t xBase, uint32_t yBase );
	float Halton( uint32_t index, uint32_t base );

	uint32_t ConvertDisplaySizeToRenderSize( uint32_t displaySize, float upscaling );

	struct UpscalingInfo
	{
		UpscalingInfo();

		uint32_t displayWidth;
		uint32_t displayHeight;
		uint32_t renderWidth;
		uint32_t renderHeight;
		Technique technique;
		Setting setting;
		bool frameGeneration;
		bool temporal;
		float upscalingAmount;
		float jitterX;
		float jitterY;
		float mipLevelBias;
	};

	const char* GetTechniqueName( Technique technique );
	const char* GetSettingName( Setting setting );

}

// forward
class Tr2UpscalingContextAL;

class Tr2UpscalingTechniqueAL
{
public:
	Tr2UpscalingTechniqueAL( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration );
    virtual ~Tr2UpscalingTechniqueAL();

	// Destroy is called when the rendercontext is being destroyed. Everything needs to be clean after this call!
	virtual void Destroy( Tr2RenderContextAL& renderContext ) = 0;
	// Called by the device to mark events if we need to trigger stuff at some point in time
	virtual void MarkFrameEvent( Tr2RenderContextAL& renderContext, Tr2RenderContextEnum::FrameEvent& frameEvent );
	// makes the state correct based on what is supported (f.ex if we pass in framegen to fsr1 it should not be "enabled")
	void SanitizeState();

	virtual bool IsAvailable( Tr2RenderContextAL& renderContext ) const;
	virtual bool SupportsFrameGeneration( ) const;
	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const = 0;

	Tr2UpscalingContextAL* GetContext( Tr2RenderContextAL& renderContext, uint32_t upscalingContextID );
	Tr2UpscalingContextAL* CreateContext( Tr2RenderContextAL& renderContext, uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat );
	void DeleteContext( Tr2RenderContextAL& renderContext, uint32_t contextID );

	void GetState( Tr2UpscalingAL::Technique& technique, Tr2UpscalingAL::Setting& setting, bool& frameGeneration );

protected:
	virtual Tr2UpscalingContextAL* CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) = 0;

	std::map<uint32_t, std::unique_ptr<Tr2UpscalingContextAL>> m_contexts;
	bool m_frameGeneration;
	Tr2UpscalingAL::Setting m_setting;
	Tr2UpscalingAL::Technique m_technique;
};


class Tr2UpscalingContextAL
{
public:
	Tr2UpscalingContextAL( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat );
    virtual ~Tr2UpscalingContextAL();

	// after setup is called, we must know the size of the render targets!
	virtual Tr2UpscalingAL::Result Setup( Tr2RenderContextAL& renderContext ) = 0;
	// when a window is closed or when we change devices this will be called
	virtual void Destroy( Tr2RenderContextAL& renderContext );
	// at what render resolution are we drawing to
	void GetRenderDimensions( uint32_t& width, uint32_t& height ) const;
	// What is the outpu dimensions
	void GetDisplayDimensions( uint32_t& width, uint32_t& height ) const;
	
	void GetJitter( float& x, float& y ) const;

	// the requirements of the dispatch (as in the textures needed)
	virtual uint32_t GetDispatchRequirements() const = 0;
	virtual void UpdateJitter() = 0;
	virtual bool IsTemporal() const = 0;
	
	float GetUpscalingAmount() const;
	float GetMipLevelBias() const;
	void Reset();

	uint32_t GetID() const;

	virtual void SetHudLessTexture( Tr2TextureAL* texture );

	virtual Tr2UpscalingAL::Result Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters ) = 0;

protected:
	bool AreDispatchParametersValid( Tr2UpscalingAL::DispatchParameters& dispatchParameters ) const;

	Tr2UpscalingAL::Setting m_setting;
	bool m_frameGeneration;

	uint32_t m_displayWidth;
	uint32_t m_displayHeight;
	uint32_t m_renderWidth;
	uint32_t m_renderHeight;
	float m_upscaling;
	bool m_reset;

	uint32_t m_jitterIndex;
	float m_jitterX;
	float m_jitterY;
	float m_jitterXScale;
	float m_jitterYScale;

	Tr2RenderContextEnum::PixelFormat m_sourceFormat;
	Tr2RenderContextEnum::DepthStencilFormat m_depthFormat;

private:
	uint32_t m_id;
};


#include TRINITY_AL_PLATFORM_INCLUDE( upscaling/Tr2UpscalingAL )
