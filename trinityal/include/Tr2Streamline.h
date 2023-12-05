#pragma once


#if TRINITY_PLATFORM == TRINITY_DIRECTX11 
#include "../dx11/Tr2StreamlineALDx11.h"
#elif TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "../dx12/Tr2StreamlineALDx12.h"
#else
	#include "Tr2TextureAL.h"
	#include "Tr2BufferAL.h"
	#include "Tr2RenderContextAL.h"

namespace StreamlineUtils
{
	struct CommonConstants
	{
		float projection[16];
		float invprojection[16];
		float clipToPrevClip[16];
		float prevClipToClip[16];
		float jitterOffset[2];
		float motionScale[2];
		float cameraPos[3];
		float cameraUp[3];
		float cameraRight[3];
		float cameraForward[3];
		float cameraNear;
		float cameraFar;
		float cameraFOV;
		float cameraAspectRatio;
		bool reset;
	};

	enum StreamlinePlugin
	{
		SP_DLSS,
		SP_DLSSG,
		SP_REFLEX,
		SP_NIS,
		SP_DEBUG,
		SP_COUNT
	};
}

namespace DlssUtils
{
	enum DlssMode
	{
		dmUltraPerformance,
		dmPerformance,
		dmBalanced,
		dmQuality,
		dmUltraQuality,
	};

	struct DlssOptions
	{
		DlssMode mode;
		uint32_t outputWidth;
		uint32_t outputHeight;
		uint32_t renderWidth;
		uint32_t renderHeight;
		Tr2RenderContextEnum::PixelFormat renderPixelFormat;
		Tr2RenderContextEnum::PixelFormat outputPixelFormat;
		Tr2RenderContextEnum::PixelFormat motionVectorPixelFormat;
		Tr2RenderContextEnum::PixelFormat depthPixelFormat;
		bool hdr;
	};

	struct DlssOptimalSetting
	{
		float sharpness;
		uint32_t renderWidth;
		uint32_t renderHeight;
	};

	struct DlssResources
	{
		Tr2TextureAL* input;
		Tr2TextureAL* depth;
		Tr2TextureAL* velocity;
		Tr2TextureAL* output;
		Tr2TextureAL* opaqueOnly;
		Tr2TextureAL* exposure;
		Tr2TextureAL* reactive;
	};
}

namespace NisUtils
{
	struct NisResources
	{
		Tr2TextureAL* input;
		Tr2TextureAL* output;
	};

	struct NisOptions
	{
		float sharpness;
		bool hdr;
	};
}
	class Tr2DlssPlugin
	{
	public:
		Tr2DlssPlugin();
		Tr2DlssPlugin( uint32_t id, bool framegeneration );
		~Tr2DlssPlugin();

		DlssUtils::DlssOptimalSetting GetOptimalSettings( DlssUtils::DlssMode mode, uint32_t outputWidth, uint32_t outputHeight );
		void SetSettings( DlssUtils::DlssOptions options );
		void SetResources( DlssUtils::DlssOptions options, DlssUtils::DlssResources resources, Tr2RenderContextAL& renderContext );
		void SetHudLessResource( Tr2TextureAL* hudless, Tr2RenderContextAL& renderContext );
		void SetUiAndAlphaResource( Tr2TextureAL* uiAlpha, Tr2RenderContextAL& renderContext );
		void Dispatch( Tr2RenderContextAL& renderContext );

		uint64_t GetEstimatedVRAMUsageInBytes();
		uint32_t GetMinWidthOrHeight();
		uint32_t GetNumFramesActuallyPresented();
		void EnableFrameGeneration( bool enable );

	protected:
		uint32_t m_id;
		bool m_framegeneration;

		void UpdateState();
	};


	class Tr2NisPlugin
	{
	public:
		Tr2NisPlugin();
		Tr2NisPlugin( uint32_t id );
		~Tr2NisPlugin();

		void SetSettings( NisUtils::NisOptions options );
		void SetResources( NisUtils::NisResources resources, Tr2RenderContextAL& renderContext );
		void Dispatch( Tr2RenderContextAL& renderContext );

	private:
		uint32_t m_id;
	};

	namespace Tr2Streamline
	{
		// static things
		bool IsAvailable();
		bool IsAvailable( StreamlineUtils::StreamlinePlugin plugin );
		bool IsInitialized();

		void MarkFrameStart();
		void MarkFrameEnd();
		void MarkPresentStart();
		void MarkPresentEnd();
		void MarkUpdateStart();
		void MarkUpdateEnd();
		void SetSwapchainRecreator( std::function<void()> recreator );

		void UpdateFrameToken();

		template <typename T>
		void CheckForProxy( T thing, const std::string& message )
		{
		};

	
		bool Initialize();
		void Shutdown();

		bool IsRunning();
		bool GetDlssPlugin( Tr2DlssPlugin& plugin );
		bool GetNisPlugin( Tr2NisPlugin& plugin );
		void Toggle( StreamlineUtils::StreamlinePlugin plugin, bool enable );
		void SetCommonConstants( StreamlineUtils::CommonConstants constants );
	};
#endif