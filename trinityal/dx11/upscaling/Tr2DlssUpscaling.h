// Copyright © 2024 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX11
#include "Tr2UpscalingAlDx11.h"
#include "dx11/Tr2TextureALDx11.h"

#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>
#include <sl_nis.h>

namespace DlssUtils
{
sl::Resource GenerateTextureResource( const Tr2TextureAL* texture );
const char* GetPluginName( sl::Feature feature );
sl::float4x4 AsFloat4x4( float f[16] );
}

class Tr2DlssUpscalingTechnique : public TrinityALImpl::Tr2UpscalingTechniqueDx11
{
public:
	Tr2DlssUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	~Tr2DlssUpscalingTechnique();

	// Tr2UpscalingTechniqueAL overrides
	virtual bool IsAvailable() const override;
	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;
	virtual bool IsTemporal() const override;

	virtual void MarkFrameEvent( Tr2RenderContextEnum::FrameEvent& frameEvent ) override;

	// TrinityALImpl::Tr2UpscalingTechniqueDx11 overrides
	virtual void AttachToDevice( CComPtr<ID3D11Device>& device ) override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params ) override;

	uint32_t m_adapter;
	bool m_isAvailable;
	bool m_streamlineSetup;

	sl::FrameToken* m_frameToken;

	uint32_t m_contextIndex;
};

class Tr2DlssUpscalingContext : public Tr2UpscalingContextAL
{
public:
	Tr2DlssUpscalingContext(
		Tr2UpscalingAL::Setting setting,
		bool frameGeneration,
		Tr2UpscalingAL::UpscalingContextParams params,
		uint32_t contextNumber,
		sl::FrameToken* frameToken );
	~Tr2DlssUpscalingContext();

	virtual bool HasSharpening() const override;

	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
	void SetFrameToken( sl::FrameToken* token );

	sl::Result ReadyDLSSResources( Tr2UpscalingAL::DispatchParameters& dispatchParameters );
	sl::Result ReadyNISResources( Tr2UpscalingAL::DispatchParameters& dispatchParameters );
	void SetCommonConstants( Tr2UpscalingAL::DispatchParameters& dispatchParameters );

	Tr2UpscalingAL::JitterSequence m_jitterSequence;

	sl::DLSSMode m_dlssMode;
	sl::DLSSOptions m_dlssOptions;
	sl::NISOptions m_nisOptions;
	sl::DLSSOptimalSettings m_optimalSettings;

	Tr2TextureAL m_dlssOutput;

	sl::ViewportHandle m_viewHandle;
	sl::Constants m_commonConstants;

	sl::FrameToken* m_frameToken;

	bool m_setup;

	friend class Tr2DlssUpscalingTechnique;
};
#endif