// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2TexturePipelineStep.h"

BLUE_CLASS( Tr2TexturePipelineStepLimitSize ) :
	public ITr2TexturePipelineStep
{
public:
	Tr2TexturePipelineStepLimitSize( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	bool Execute( ImageIO::HostBitmap & bitmap, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>& inputs, const Tr2TexturePipelineParams& params ) const;

	static bool LimitSize( ImageIO::HostBitmap & bitmap, uint32_t maxWidth, uint32_t maxHeight );

private:
	uint32_t m_maxWidth;
	uint32_t m_maxHeight;
};

TYPEDEF_BLUECLASS( Tr2TexturePipelineStepLimitSize );