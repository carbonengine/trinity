#pragma once

#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"

struct Tr2AdapterInfo;
struct Tr2DisplayModeInfo;


class Tr2VideoAdapterInfo
{
public:
	static const unsigned DEFAULT_ADAPTER = 0;

	static ALResult GetAdapterCount( unsigned& count );
	static ALResult GetAdapterInfo( unsigned adapterIndex,
									Tr2AdapterInfo& info );
	static ALResult GetAdapterMonitor( unsigned adapterIndex,
									   void*& monitor );
	static ALResult GetAdapterDisplayMode( unsigned adapterIndex,
										   Tr2DisplayModeInfo& mode );
	static ALResult GetAdapterModeCount( unsigned adapterIndex,
										 Tr2RenderContextEnum::PixelFormat backBufferFormat,
										 unsigned& count );
	static ALResult GetAdapterMode( unsigned adapterIndex,
									Tr2RenderContextEnum::PixelFormat backBufferFormat,
									unsigned modeIndex,
									Tr2DisplayModeInfo& mode );
	static ALResult GetAdapterMsaaSupport( unsigned adapterIndex,
										   Tr2RenderContextEnum::PixelFormat format,
										   bool windowed,
										   unsigned msaaType,
										   unsigned& msaaQuality );
	static ALResult GetAdapterMsaaSupport( unsigned adapterIndex,
										   Tr2RenderContextEnum::DepthStencilFormat format,
										   bool windowed,
										   unsigned msaaType,
										   unsigned& msaaQuality );

	static ALResult GetAdapterShaderVersion( unsigned adapterIndex,
											 unsigned& version );
	static bool SupportsBackBufferFormat( unsigned adapterIndex,
										  Tr2RenderContextEnum::PixelFormat backBufferFormat,
										  bool windowed );
	static bool SupportsRenderTargetFormat( unsigned adapterIndex,
											Tr2RenderContextEnum::PixelFormat backBufferFormat,
											Tr2RenderContextEnum::PixelFormat format,
											bool withAutoGenMipmap );
	static bool SupportsDepthStencilFormat( unsigned adapterIndex,
											Tr2RenderContextEnum::PixelFormat backBufferFormat,
											Tr2RenderContextEnum::DepthStencilFormat format );
	static bool SupportsVertexTextureFormat( unsigned adapterIndex,
											 Tr2RenderContextEnum::PixelFormat backBufferFormat,
											 Tr2RenderContextEnum::PixelFormat format );
	static ALResult GetAdapterMaxTextureWidth( unsigned adapterIndex,
											   unsigned& maxWidth );

	static bool AreAdaptersDifferent( unsigned adapter1,
									  unsigned adapter2 );

	static ALResult RefreshData();
};

#endif
