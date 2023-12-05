#include "StdAfx.h"
#include "./include/Tr2Streamline.h"

#if TRINITY_PLATFORM != TRINITY_DIRECTX11 && TRINITY_PLATFORM != TRINITY_DIRECTX12
namespace Tr2Streamline
{

	bool IsAvailable()
	{
		return false;
	}
	bool Initialize()
	{
		return false;
	}

	void Shutdown()
	{
	}

	void SetSwapchainCreator( std::function<void()> creator )
	{
	}

	void SetSwapchainDestroyer( std::function<void()> destroyer )
	{
	}

	bool IsAvailable( StreamlineUtils::StreamlinePlugin plugin )
	{
		return false;
	}


	void MarkFrameStart()
	{
	}

	void MarkFrameEnd()
	{
	}

	void MarkPresentStart()
	{
	}

	void MarkPresentEnd()
	{
	}
	void MarkUpdateStart()
	{
	}
	void MarkUpdateEnd()
	{
	}

	bool IsRunning()
	{
		return false;
	}

	bool GetDlssPlugin( Tr2DlssPlugin& plugin )
	{
		return false;
	}

	bool GetNisPlugin( Tr2NisPlugin& plugin )
	{
		return false;
	}

	void Toggle( StreamlineUtils::StreamlinePlugin plugin, bool enable )
	{
	}

	void SetCommonConstants( StreamlineUtils::CommonConstants constants )
	{
	}
}
Tr2DlssPlugin::Tr2DlssPlugin()
{
}

Tr2DlssPlugin::Tr2DlssPlugin( uint32_t id, bool framegeneration ) :
	m_id( id ),
	m_framegeneration( framegeneration )
{
}

Tr2DlssPlugin::~Tr2DlssPlugin()
{
}

DlssUtils::DlssOptimalSetting Tr2DlssPlugin::GetOptimalSettings( DlssUtils::DlssMode mode, uint32_t outputWidth, uint32_t outputHeight )
{
	return DlssUtils::DlssOptimalSetting{};
}

void Tr2DlssPlugin::UpdateState()
{
}

void Tr2DlssPlugin::EnableFrameGeneration( bool enable )
{
	m_framegeneration = enable;
}

void Tr2DlssPlugin::SetSettings( DlssUtils::DlssOptions options )
{
}

void Tr2DlssPlugin::SetResources( DlssUtils::DlssOptions options, DlssUtils::DlssResources resources, Tr2RenderContextAL& renderContext )
{
}

void Tr2DlssPlugin::SetHudLessResource( Tr2TextureAL* hudless, Tr2RenderContextAL& renderContext )
{
}

void Tr2DlssPlugin::SetUiAndAlphaResource( Tr2TextureAL* uiAlpha, Tr2RenderContextAL& renderContext )
{
}

void Tr2DlssPlugin::Dispatch( Tr2RenderContextAL& renderContext )
{
}

uint64_t Tr2DlssPlugin::GetEstimatedVRAMUsageInBytes()
{
	return 0;
}

uint32_t Tr2DlssPlugin::GetMinWidthOrHeight()
{
	return 0;
}

uint32_t Tr2DlssPlugin::GetNumFramesActuallyPresented()
{
	return 0;
}

Tr2NisPlugin::Tr2NisPlugin()
{
}

Tr2NisPlugin::Tr2NisPlugin( uint32_t id ) :
	m_id( id )
{
}

Tr2NisPlugin::~Tr2NisPlugin()
{
}

void Tr2NisPlugin::SetSettings( NisUtils::NisOptions options )
{
}


void Tr2NisPlugin::SetResources( NisUtils::NisResources resources, Tr2RenderContextAL& renderContext )
{
}

void Tr2NisPlugin::Dispatch( Tr2RenderContextAL& renderContext )
{
}

#endif
