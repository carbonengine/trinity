////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2NoopUpscaling.h"

Tr2NoopUpscaling::Tr2NoopUpscaling( IRoot* lockobj ):
	m_renderWidth( 0 ),
	m_renderHeight( 0 ),
	m_displayWidth( 0 ),
	m_displayHeight( 0 )
{
}

Tr2NoopUpscaling::~Tr2NoopUpscaling()
{
}

bool Tr2NoopUpscaling::IsApplicable() const
{
	// No-op is available on all platforms
	return true;
}

Tr2Upscaling::UpscalingType Tr2NoopUpscaling::GetUpscalingType() const
{
	return Tr2Upscaling::UT_NOT_APPLICABLE;
}

void Tr2NoopUpscaling::GetJitter( float& x, float& y )
{
	x = 0;
	y = 0;
}

void Tr2NoopUpscaling::GetJitterOffset( float& x, float& y )
{
	x = 0;
	y = 0;
}

float Tr2NoopUpscaling::GetMipLevelBias() const
{
	return 0.0f;
}

void Tr2NoopUpscaling::GetRenderSize(uint32_t& width, uint32_t& height) const
{
	width = m_displayWidth;
	height = m_displayHeight;
}

const std::vector<Tr2Upscaling::Setting> Tr2NoopUpscaling::GetAvailableSettings() const
{
	static std::vector<Tr2Upscaling::Setting> availableSettings = { };
	return availableSettings;
}

void Tr2NoopUpscaling::ApplySetting( const Tr2Upscaling::Setting& settingm, uint32_t displayWidth, uint32_t displayHeight )
{
	m_renderWidth = displayWidth;
	m_renderHeight = displayHeight;
	m_displayWidth = displayWidth;
	m_displayHeight = displayHeight;
}

void Tr2NoopUpscaling::Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext )
{
}

void Tr2NoopUpscaling::Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation )
{
}
