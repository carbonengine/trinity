////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "include/upscaling/Tr2UpscalingAL.h"
#include "dx12/Tr2TextureALDx12.h"

#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>
#include <sl_dlss_g.h>
#include <sl_reflex.h>
#include <sl_pcl.h>


namespace DlssUtils
{
sl::Resource GenerateTextureResource( Tr2TextureAL* texture );
}

class Tr2DlssUpscalingTechnique : public TrinityALImpl::Tr2UpscalingTechniqueDx12
{
public:
	Tr2DlssUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	~Tr2DlssUpscalingTechnique();

	// Tr2UpscalingTechniqueAL overrides
	virtual bool IsAvailable( ) const override;
	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;
	virtual bool SupportsFrameGeneration( ) const override;
	virtual bool IsTemporal() const override;

	virtual void MarkFrameEvent( Tr2RenderContextEnum::FrameEvent& frameEvent ) override;

	// TrinityALImpl::Tr2UpscalingTechniqueDx12 overrides
	virtual bool ReplacesDevice() const override;
	virtual bool ReplacesCommandQueue() const override;
	virtual bool ReplacesFactory() const override;

	virtual CComPtr<ID3D12Device> ReplaceDevice( CComPtr<ID3D12Device>& device ) override;
	virtual CComPtr<ID3D12CommandQueue> ReplaceCommandQueue( CComPtr<ID3D12CommandQueue>& commandQueue ) override;
	virtual CComPtr<IDXGIFactory4> ReplaceFactory( CComPtr<IDXGIFactory4>& factory ) override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params ) override;

	uint32_t m_adapter;
	
	bool m_isAvailable;
	bool m_supportsFrameGeneration;

	bool m_streamlineSetup;

	uint32_t m_contextIndex;
	sl::FrameToken* m_frameToken;

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

	bool HasSharpening() const override;
	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;
	virtual void SetHudLessTexture( Tr2TextureAL* texture ) override;
	virtual void SetupForReuse() override;

private:
	void SetFrameToken( sl::FrameToken* token );
	sl::Result EvaluateDLSS( Tr2UpscalingAL::DispatchParameters& dispatchParameters );
	void SetCommonConstants( Tr2UpscalingAL::DispatchParameters& dispatchParameters );

	sl::Result UpdateDlssG();
	Tr2UpscalingAL::JitterSequence m_jitterSequence;

	sl::DLSSMode m_dlssMode;
	sl::DLSSOptions m_dlssOptions;
	sl::DLSSGOptions m_dlssgOptions;

	sl::DLSSOptimalSettings m_optimalSettings;

	sl::ViewportHandle m_viewHandle;
	sl::DLSSGState m_dlssgState;
	sl::Constants m_commonConstants;

	sl::FrameToken* m_frameToken;

	bool m_setup;

	uint64_t m_vramUsage;
	uint32_t m_minWidthHeight;
	uint32_t m_actualFrames;

	friend class Tr2DlssUpscalingTechnique;
};
#endif