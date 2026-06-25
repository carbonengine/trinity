// Copyright © 2024 CCP ehf.

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
	const Tr2TextureAL* input;
	const Tr2TextureAL* opaqueOnly;
	const Tr2TextureAL* output;
	const Tr2TextureAL* depth;
	const Tr2TextureAL* velocity;
	const Tr2TextureAL* exposure;
	const Tr2TextureAL* reactive;
	const Tr2TextureAL* transparency;

	unsigned long long currentFrameIndex;
	float frontClip;
	float backClip;
	float fieldOfView;
	float aspectRatio;
	float frameTimeDelta;
	float preExposure;
	float cameraForward[3];
	float cameraRight[3];
	float cameraUp[3];
	float cameraPos[3];
	float projection[16];
	float invProjection[16];
	float clipToPrevClip[16];
	float prevClipToClip[16];
	bool reset;
	bool upscalingDebugView;
	bool frameGenDebugView;
};

enum DispatchRequirements
{
	VELOCITY = 1 << 0,
	OPAQUE_ONLY = 1 << 1,
	DEPTH = 1 << 2,
	REACTIVE = 1 << 3,
	OPTIONAL_EXPOSURE = 1 << 4,
	TRANSPARENCY = 1 << 5,
};

void LogResult( Result result );

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
	bool hasSharpening;
	float upscalingAmount;
	float jitterX;
	float jitterY;
	float mipLevelBias;
};

struct UpscalingContextParams
{
	UpscalingContextParams( Tr2RenderContextAL& renderContext );
	uint32_t displayWidth;
	uint32_t displayHeight;
	Tr2RenderContextEnum::PixelFormat sourceFormat;
	Tr2RenderContextEnum::DepthStencilFormat depthFormat;
	bool allowFramegen;
	Tr2RenderContextAL& renderContext;

	bool operator==( const UpscalingContextParams& other ) const;
};

const char* GetTechniqueName( Technique technique );
const char* GetSettingName( Setting setting );
const uint32_t INVALID_CONTEXT_ID = std::numeric_limits<uint32_t>::max();
}

// forward
class Tr2UpscalingContextAL;

class Tr2UpscalingTechniqueAL
{
public:
	Tr2UpscalingTechniqueAL( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	virtual ~Tr2UpscalingTechniqueAL() = default;

	// Called by the device to mark events if we need to trigger stuff at some point in time
	virtual void MarkFrameEvent( Tr2RenderContextEnum::FrameEvent& frameEvent );
	// makes the state correct based on what is supported (f.ex if we pass in framegen to fsr1 it should not be "enabled")
	void SanitizeState();

	virtual void ReleaseResources();

	virtual bool IsAvailable() const;
	virtual bool SupportsFrameGeneration() const;
	virtual bool IsTemporal() const = 0;
	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const = 0;

	Tr2UpscalingContextAL* GetContext( uint32_t upscalingContextID );
	Tr2UpscalingContextAL* CreateContext( Tr2UpscalingAL::UpscalingContextParams params, uint32_t existingContext = Tr2UpscalingAL::INVALID_CONTEXT_ID );
	void DeleteContext( uint32_t contextID );

	void GetState( Tr2UpscalingAL::Technique& technique, Tr2UpscalingAL::Setting& setting, bool& frameGeneration ) const;

protected:
	virtual Tr2UpscalingContextAL* CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params ) = 0;

	std::map<uint32_t, std::unique_ptr<Tr2UpscalingContextAL>> m_contexts;
	bool m_frameGeneration;
	Tr2UpscalingAL::Setting m_setting;
	Tr2UpscalingAL::Technique m_technique;
	Tr2RenderContextAL& m_renderContext;
};


class Tr2UpscalingContextAL
{
public:
	Tr2UpscalingContextAL( Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2UpscalingAL::UpscalingContextParams params );
	virtual ~Tr2UpscalingContextAL();
	bool Reuse( Tr2UpscalingAL::UpscalingContextParams params );
	virtual void SetupForReuse();
	// at what render resolution are we drawing to
	void GetRenderDimensions( uint32_t& width, uint32_t& height ) const;
	// What is the output dimensions
	void GetDisplayDimensions( uint32_t& width, uint32_t& height ) const;

	void GetJitter( float& x, float& y ) const;

	// the requirements of the dispatch (as in the textures needed)
	virtual uint32_t GetDispatchRequirements() const = 0;
	virtual void UpdateJitter() = 0;
	virtual bool HasSharpening() const = 0;

	float GetUpscalingAmount() const;
	float GetMipLevelBias( bool temporal ) const;

	uint32_t GetID() const;

	virtual void SetHudLessTexture( const Tr2TextureAL* texture );

	virtual Tr2UpscalingAL::Result Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters ) = 0;

protected:
	bool AreDispatchParametersValid( Tr2UpscalingAL::DispatchParameters& dispatchParameters ) const;

	Tr2UpscalingAL::Setting m_setting;
	bool m_frameGeneration;

	uint32_t m_displayWidth;
	uint32_t m_displayHeight;
	uint32_t m_renderWidth;
	uint32_t m_renderHeight;
	float m_upscaling;

	uint32_t m_jitterIndex;
	float m_jitterX;
	float m_jitterY;
	float m_jitterXScale;
	float m_jitterYScale;

	Tr2UpscalingAL::UpscalingContextParams m_params;

private:
	uint32_t m_id;
};


// clang-format off
#include TRINITY_AL_PLATFORM_INCLUDE( upscaling/Tr2UpscalingAL )
// clang-format on
