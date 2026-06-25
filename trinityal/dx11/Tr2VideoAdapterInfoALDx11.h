// Copyright © 2023 CCP ehf.

#pragma once

#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"


struct Tr2AdapterInfo;
struct Tr2DisplayModeInfo;


#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

class Tr2VideoAdapterInfo
{
public:
	static const unsigned DEFAULT_ADAPTER = 0;

	static ALResult GetAdapterCount( unsigned& count );
	static ALResult GetAdapterInfo( unsigned adapterIndex, Tr2AdapterInfo& info );
	static ALResult GetAdapterMonitor( unsigned adapterIndex, void*& monitor );
	static ALResult GetAdapterDisplayMode( unsigned adapterIndex, Tr2DisplayModeInfo& mode );
	static ALResult GetAdapterModeCount( unsigned adapterIndex,
										 Tr2RenderContextEnum::PixelFormat backBufferFormat,
										 unsigned& count );
	static ALResult GetAdapterMode( unsigned adapterIndex,
									Tr2RenderContextEnum::PixelFormat backBufferFormat,
									unsigned modeIndex,
									Tr2DisplayModeInfo& mode );

	static bool SupportsBackBufferFormat( unsigned adapterIndex, Tr2RenderContextEnum::PixelFormat backBufferFormat );
	static bool SupportsRenderTargetFormat( unsigned adapterIndex, Tr2RenderContextEnum::PixelFormat format );
	static ALResult GetAdapterMaxTextureWidth( unsigned adapterIndex, unsigned& maxWidth );

	static bool AreAdaptersDifferent( unsigned adapter1, unsigned adapter2 );

	static ALResult RefreshData();

	static ALResult GetVideoAdapterDX11( unsigned adapterIndex,
										 IDXGIAdapter1** adapter,
										 IDXGIOutput** output );
};

#endif
