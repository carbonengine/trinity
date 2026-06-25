// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2TexturePipeline.h"
#include "Tr2TexturePipelineStepLimitSize.h"
#include "Tr2HostBitmap.h"


Tr2TexturePipeline::Tr2TexturePipeline( IRoot* lockobj ) :
	PARENTLOCK( m_steps ),
	m_inputs( "Tr2TexturePipeline::m_inputs" )
{
}

void Tr2TexturePipeline::GetResourceDependencies( std::set<std::wstring>& resources ) const
{
	resources.clear();
	for( auto it = begin( m_steps ); it != end( m_steps ); ++it )
	{
		( *it )->GetResourceDependencies( resources );
	}
}

bool Tr2TexturePipeline::Execute( ImageIO::HostBitmap& result, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>& inputs, const Tr2TexturePipelineParams& params ) const
{
	result.Destroy();

	if( m_steps.empty() )
	{
		CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipeline: no steps" );
		return false;
	}
	for( auto it = begin( m_steps ); it != end( m_steps ); ++it )
	{
		( *it )->Execute( result, inputs, params );
	}

	if( params.maxHeight || params.maxWidth )
	{
		if( !Tr2TexturePipelineStepLimitSize::LimitSize( result, params.maxWidth, params.maxHeight ) )
		{
			return false;
		}
	}
	return true;
}

std::vector<std::wstring> Tr2TexturePipeline::GetResourceDependenciesFromScript() const
{
	std::set<std::wstring> resources;
	GetResourceDependencies( resources );
	std::vector<std::wstring> result;
	result.insert( result.end(), resources.begin(), resources.end() );
	return result;
}

Be::Result<std::string> Tr2TexturePipeline::ExecuteFromScript( uint32_t maxWidth, uint32_t maxHeight, Tr2HostBitmapPtr& result ) const
{
	std::set<std::wstring> resources;
	GetResourceDependencies( resources );
	std::unordered_map<std::wstring, Tr2HostBitmapPtr> bitmaps;
	std::unordered_map<std::wstring, const ImageIO::HostBitmap*> inputs;
	for( auto it = begin( resources ); it != end( resources ); ++it )
	{
		Tr2HostBitmapPtr bmp;
		bmp.CreateInstance();
		if( !bmp )
		{
			return Be::Result<std::string>( "out of memory" );
		}
		if( !bmp->CreateFromFile( *it ) )
		{
			return Be::Result<std::string>( std::string( "failed to load texture " ) + (const char*)CW2A( it->c_str() ) );
		}
		bitmaps.insert( std::make_pair( *it, bmp ) );
		inputs[*it] = bmp;
	}

	result.CreateInstance();
	if( !result )
	{
		return Be::Result<std::string>( "out of memory" );
	}
	if( !Execute( *result, inputs, Tr2TexturePipelineParams( maxWidth, maxHeight ) ) )
	{
		result = nullptr;
		return Be::Result<std::string>( "error executing pipeline" );
	}
	return Be::Result<std::string>();
}
