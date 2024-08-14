////////////////////////////////////////////////////////////
//
//    Created:   September 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2VideoAdapters_H
#define Tr2VideoAdapters_H


BLUE_DECLARE ( Tr2VideoAdapter );
BLUE_DECLARE ( Tr2DisplayMode );
BLUE_DECLARE ( Tr2VideoDriver );

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed functionality of AL Tr2VideoAdapterInfo.
// See Also:
//   Tr2VideoAdapterInfo
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2VideoAdapters ): public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2VideoAdapters();

	ALResult GetAdapterCount( unsigned& count );
	ALResult GetAdapterInfo( unsigned adapterIndex,
							 Tr2VideoAdapter** info );
	ALResult GetCurrentDisplayMode( unsigned adapterIndex,
									Tr2DisplayMode** mode );
	ALResult GetDisplayModeCount( unsigned adapterIndex,
								  int /*Tr2RenderContextEnum::PixelFormat*/ backBufferFormat,
								  unsigned& count );
	ALResult GetDisplayMode( unsigned adapterIndex,
							 int /*Tr2RenderContextEnum::PixelFormat*/ backBufferFormat,
							 unsigned modeIndex,
							 Tr2DisplayMode** mode );
	bool SupportsBackBufferFormat( unsigned adapterIndex,
								   int /*Tr2RenderContextEnum::PixelFormat*/ backBufferFormat );
	bool SupportsRenderTargetFormat( unsigned adapterIndex,
									 int /*Tr2RenderContextEnum::PixelFormat*/ format );
	ALResult GetMaxTextureSize( unsigned adapterIndex,
								unsigned& maxWidth );

	ALResult RefreshData();

	const unsigned DEFAULT_ADAPTER;
};

TYPEDEF_BLUECLASS ( Tr2VideoAdapters );


// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed functionality of AL Tr2AdapterInfo structure with all fields being
//   read-only.
// See Also:
//   Tr2AdapterInfo
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2VideoAdapter ): public IRoot
{
public:
	EXPOSE_TO_BLUE();

	// Index for Tr2VideoAdapterInfo::GetAdapterInfo
	unsigned m_index;
	Tr2AdapterInfo m_info;

	std::string GetDeviceIdentifierString() const;

	ALResult GetDriverInfo( Tr2VideoDriver** info );
};

TYPEDEF_BLUECLASS ( Tr2VideoAdapter );


// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed functionality of AL Tr2DisplayModeInfo structure with all fields being
//   read-only.
// See Also:
//   Tr2DisplayModeInfo
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2DisplayMode ): public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2DisplayModeInfo m_mode;
};

TYPEDEF_BLUECLASS ( Tr2DisplayMode );


// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed functionality of AL Tr2VideoDriverInfo structure with all fields being
//   read-only.
// See Also:
//   Tr2VideoDriverInfo
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2VideoDriver ): public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2VideoDriverInfo m_info;
};

TYPEDEF_BLUECLASS ( Tr2VideoDriver );

#endif // Tr2VideoAdapters_H
