// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2TexturePipelineStep.h"

BLUE_DECLARE( Tr2TexturePipelineStepPack );

BLUE_CLASS( Tr2TexturePackChannel ) :
	public IRoot
{
public:
	Tr2TexturePackChannel( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

private:
	std::wstring m_path;
	uint8_t m_channel;
	uint8_t m_fill;

	friend class Tr2TexturePipelineStepPack;
};

TYPEDEF_BLUECLASS( Tr2TexturePackChannel );


BLUE_CLASS( Tr2TexturePipelineStepPack ) :
	public ITr2TexturePipelineStep
{
public:
	Tr2TexturePipelineStepPack( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void GetResourceDependencies( std::set<std::wstring> & resources ) const;
	virtual bool Execute( ImageIO::HostBitmap & bitmap, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>& inputs, const Tr2TexturePipelineParams& params ) const;

private:
	struct BitmapSource
	{
		const uint8_t* data;
		uint32_t pixelStride;
		uint32_t rowStride;
	};

	static void Pack( ImageIO::HostBitmap & bitmap, BitmapSource * channels, uint32_t channelCount, uint32_t mip );

	Tr2RenderContextEnum::PixelFormat m_format;
	PTr2TexturePackChannel m_r;
	PTr2TexturePackChannel m_g;
	PTr2TexturePackChannel m_b;
	PTr2TexturePackChannel m_a;
};

TYPEDEF_BLUECLASS( Tr2TexturePipelineStepPack );
