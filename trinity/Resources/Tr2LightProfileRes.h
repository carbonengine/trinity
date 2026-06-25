// Copyright © 2022 CCP ehf.

#pragma once

#include "../Tr2TextureArray.h"

BLUE_DECLARE( Tr2HostBitmap );


BLUE_CLASS( Tr2LightProfileRes ) :
	public BlueAsyncRes,
	public ICacheable
{
public:
	EXPOSE_TO_BLUE();

	Tr2LightProfileRes();

	bool IsMemoryUsageKnown() override;
	size_t GetMemoryUsage() override;

	uint16_t GetTextureIndex() const;

	static BlueStdResult BakeLightProfile( const wchar_t* path, Tr2HostBitmapPtr& output );
	BlueStdResult GetThumbnail( uint32_t width, uint32_t height, Tr2HostBitmapPtr& output ) const;

protected:
	LoadingResult DoLoad() override;
	bool DoPrepare() override;

private:
	bool ParseIes();
	static bool ParseIes( const std::string& contents, ImageIO::HostBitmap& bitmap );

	ImageIO::HostBitmap m_bitmap;
	Tr2TextureArrayElement m_element;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2LightProfileRes );