////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "include/upscaling/Tr2UpscalingAL.h"
#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>
#include <sl_dlss_g.h>
#include <sl_reflex.h>
#include <sl_pcl.h>
#include <sl_nis.h>

namespace DlssUtils
{
	void Log( sl::LogType type, const char* msg );
	sl::Resource GenerateTextureResource( Tr2TextureAL* texture );
	const char* GetPluginName( sl::Feature feature );
	sl::float4x4 AsFloat4x4( float f[16] );
}

class Tr2DlssUpscalingTechnique : public TrinityALImpl::Tr2UpscalingTechniqueDx12
{
public:
	Tr2DlssUpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	~Tr2DlssUpscalingTechnique();

	// Tr2UpscalingTechniqueAL overrides
	virtual bool IsAvailable( Tr2RenderContextAL& renderContext ) const override;
	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;
	virtual bool SupportsFrameGeneration( ) const override;

	virtual void MarkFrameEvent( Tr2RenderContextAL& renderContext, Tr2RenderContextEnum::FrameEvent& frameEvent ) override;
	virtual void Destroy( Tr2RenderContextAL& renderContext ) override;

	// TrinityALImpl::Tr2UpscalingTechniqueDx12 overrides
	virtual bool ReplacesDevice() const override;
	virtual bool ReplacesCommandQueue() const override;
	virtual bool ReplacesFactory() const override;

	virtual CComPtr<ID3D12Device> ReplaceDevice( CComPtr<ID3D12Device>& device ) override;
	virtual CComPtr<ID3D12CommandQueue> ReplaceCommandQueue( CComPtr<ID3D12CommandQueue>& commandQueue ) override;
	virtual CComPtr<IDXGIFactory4> ReplaceFactory( CComPtr<IDXGIFactory4>& factory ) override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) override;

	void TogglePlugin( sl::Feature feature, bool enable );

	uint32_t m_adapter;
	
	HMODULE m_streamlineModule;
	bool m_isAvailable;
	bool m_supportsFrameGeneration;

	bool m_streamlineSetup;
	sl::FrameToken* m_frameToken;

	// a few functions from the streamline module.
	PFun_slGetFeatureFunction* m_slGetFeatureFunction;
	PFun_slReflexSetOptions* m_slReflexSetOptions;
	PFun_slSetFeatureLoaded* m_slSetFeatureLoaded;
	PFun_slPCLSetMarker* m_slPCLSetMarker;
	PFun_slGetNewFrameToken* m_slGetNewFrameToken;
	PFun_slUpgradeInterface* m_slUpgradeInterface;

	bool m_attachedToDevice;

	uint32_t m_contextIndex;
};

class Tr2DlssUpscalingContext : public Tr2UpscalingContextAL
{
public:
	Tr2DlssUpscalingContext( 
		uint32_t displayWidth, 
		uint32_t displayHeight, 
		Tr2UpscalingAL::Setting setting, 
		bool frameGeneration, 
		Tr2RenderContextEnum::PixelFormat sourceFormat, 
		Tr2RenderContextEnum::DepthStencilFormat depthFormat,
		uint32_t contextNumber, 
		HMODULE streamlineModule, 
		sl::FrameToken* frameToken );
	~Tr2DlssUpscalingContext();
	virtual void Destroy(Tr2RenderContextAL& renderContext ) override;

	virtual Tr2UpscalingAL::Result Setup( Tr2RenderContextAL& renderContext ) override;
	virtual bool IsTemporal() const override;
	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
	void SetFrameToken( sl::FrameToken* token );
	sl::Result ReadyResources( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters );
	void SetCommonConstants( Tr2UpscalingAL::DispatchParameters& dispatchParameters );

	sl::Result UpdateDlssG();

	Tr2UpscalingAL::JitterSequence m_jitterSequence;

	sl::DLSSMode m_dlssMode;
	sl::DLSSOptions m_options;
	sl::DLSSGOptions m_dlssgOptions;
	sl::DLSSOptimalSettings m_optimalSettings;

	sl::ViewportHandle m_viewHandle;
	sl::DLSSGState m_dlssgState;
	sl::Constants m_commonConstants;

	HMODULE m_streamlineModule;
	sl::FrameToken* m_frameToken;

	bool m_setup;

	uint64_t m_vramUsage;
	uint32_t m_minWidthHeight;
	uint32_t m_actualFrames;

	// a few functions from the streamline module.
	PFun_slDLSSGetOptimalSettings* m_slDLSSGetOptimalSettings;
	PFun_slDLSSSetOptions* m_slDLSSSetOptions;
	PFun_slDLSSGSetOptions* m_slDLSSGSetOptions;
	PFun_slSetConstants* m_slSetConstants;
	PFun_slEvaluateFeature* m_slEvaluateFeature;
	PFun_slFreeResources* m_slFreeResources;
	PFun_slDLSSGGetState* m_slDLSSGGetState;
	PFun_slSetTag* m_slSetTag;
	PFun_slAllocateResources* m_slAllocateResources;

	friend class Tr2DlssUpscalingTechnique;
};
#endif