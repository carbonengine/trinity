////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX11
#include "Tr2UpscalingAlDx11.h"
#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>
#include <sl_dlss_g.h>
#include <sl_reflex.h>
#include <sl_nis.h>

namespace DlssUtils
{
	void Log( sl::LogType type, const char* msg );
	sl::Resource GenerateTextureResource( Tr2TextureAL* texture );
	const char* GetPluginName( sl::Feature feature );
	sl::float4x4 AsFloat4x4( float f[16] );
}

class Tr2DlssUpscalingTechnique : public TrinityALImpl::Tr2UpscalingTechniqueDx11
{
public:
	Tr2DlssUpscalingTechnique( Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	~Tr2DlssUpscalingTechnique();

	virtual void MarkFrameEvent( Tr2RenderContextEnum::FrameEvent& frameEvent ) override;
	virtual Tr2UpscalingAL::Result Setup() override;
	virtual void Destroy( Tr2RenderContextAL& renderContext ) override;

	virtual void AttachToDevice( CComPtr<ID3D11Device>& device ) override;

private:
	virtual Tr2UpscalingContext* CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight ) override;

	bool InitializeStreamline();
	bool IsPluginAvailable( sl::Feature feature, uint32_t adapter );
	bool TogglePlugin( sl::Feature feature, bool enable );

	bool m_available;

	uint32_t m_adapter;
	HMODULE m_streamlineModule;
	sl::FrameToken* m_frameToken;

	uint32_t m_contextIndex;
};

class Tr2DlssUpscalingContext : public Tr2UpscalingContext
{
public:
	Tr2DlssUpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t contextNumber, HMODULE streamlineModule, sl::FrameToken* frameToken );
	~Tr2DlssUpscalingContext();

	virtual Tr2UpscalingAL::Result Setup( Tr2RenderContextAL& renderContext ) override;
	void Destroy();
	virtual bool IsTemporal() const override;
	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
	void SetFrameToken( sl::FrameToken* token );

	sl::Result ReadyResources( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters );
	void SetCommonConstants( Tr2UpscalingAL::DispatchParameters& dispatchParameters );

	Tr2UpscalingAL::JitterSequence m_jitterSequence;

	sl::DLSSMode m_dlssMode;
	sl::DLSSOptions m_options;
	sl::DLSSOptimalSettings m_optimalSettings;

	sl::ViewportHandle m_viewHandle;
	sl::Constants m_commonConstants;

	HMODULE m_streamlineModule;
	sl::FrameToken* m_frameToken;

	bool m_setup;

	friend class Tr2DlssUpscalingTechnique;
};
#endif