// Copyright © 2018 CCP ehf.

#pragma once

struct Tr2TexturePipelineParams
{
	uint32_t maxWidth;
	uint32_t maxHeight;

	Tr2TexturePipelineParams( uint32_t _maxWidth = 0, uint32_t _maxHeight = 0 ) :
		maxWidth( _maxWidth ),
		maxHeight( _maxHeight )
	{
	}
};

BLUE_INTERFACE( ITr2TexturePipelineStep ) :
	public IRoot
{
	virtual void GetResourceDependencies( std::set<std::wstring> & resources ) const {};
	virtual bool Execute( ImageIO::HostBitmap & bitmap, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>& inputs, const Tr2TexturePipelineParams& params ) const = 0;
};

BLUE_DECLARE_IVECTOR( ITr2TexturePipelineStep );

static CcpLogChannel_t s_texturePipelineChannel = CCP_LOG_DEFINE_CHANNEL( "TexturePipeline" );
