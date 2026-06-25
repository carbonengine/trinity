// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2TexturePipelineStep.h"

BLUE_CLASS( Tr2TexturePipelineStepCompress ) :
	public ITr2TexturePipelineStep
{
public:
	Tr2TexturePipelineStepCompress( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	bool Execute( ImageIO::HostBitmap & bitmap, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>& inputs, const Tr2TexturePipelineParams& params ) const;

private:
	Tr2RenderContextEnum::PixelFormat m_format;
	float m_rWeight;
	float m_gWeight;
	float m_bWeight;
};

TYPEDEF_BLUECLASS( Tr2TexturePipelineStepCompress );