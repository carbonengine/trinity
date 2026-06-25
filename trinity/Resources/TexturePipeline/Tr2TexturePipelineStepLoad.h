// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2TexturePipelineStep.h"

BLUE_CLASS( Tr2TexturePipelineStepLoad ) :
	public ITr2TexturePipelineStep
{
public:
	EXPOSE_TO_BLUE();

	virtual void GetResourceDependencies( std::set<std::wstring> & resources ) const;
	virtual bool Execute( ImageIO::HostBitmap & bitmap, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>& inputs, const Tr2TexturePipelineParams& params ) const;

private:
	std::wstring m_path;
};

TYPEDEF_BLUECLASS( Tr2TexturePipelineStepLoad );