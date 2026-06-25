// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2TexturePipelineStep.h"

BLUE_DECLARE( Tr2HostBitmap );
BLUE_DECLARE( TriTextureRes );

BLUE_CLASS( Tr2TexturePipeline ) :
	public IRoot
{
public:
	Tr2TexturePipeline( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void GetResourceDependencies( std::set<std::wstring> & resources ) const;
	bool Execute( ImageIO::HostBitmap & result, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>& inputs, const Tr2TexturePipelineParams& params ) const;

private:
	std::vector<std::wstring> GetResourceDependenciesFromScript() const;
	Be::Result<std::string> ExecuteFromScript( uint32_t maxWidth, uint32_t maxHeight, Tr2HostBitmapPtr& result ) const;

	std::string m_pipelineType;
	PITr2TexturePipelineStepVector m_steps;

	TrackableStdHashMap<std::wstring, TriTextureResPtr> m_inputs;
	ImageIO::HostBitmap m_output;
};

TYPEDEF_BLUECLASS( Tr2TexturePipeline );