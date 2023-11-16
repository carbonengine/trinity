////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2TexturePipelineStepLoad.h"


void Tr2TexturePipelineStepLoad::GetResourceDependencies( std::set<std::wstring>& resources ) const
{
	resources.insert( m_path );
}

bool Tr2TexturePipelineStepLoad::Execute( ImageIO::HostBitmap& bitmap, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>& inputs, const Tr2TexturePipelineParams& ) const
{
	if( bitmap.IsValid() )
	{
		CCP_LOGWARN_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepLoad: output bitmap is not empty" );
	}
	auto found = inputs.find( m_path );
	if( found == inputs.end() || !found->second )
	{
		CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepLoad: failed to get input texture %S", m_path.c_str() );
		return false;
	}
	auto& input = *found->second;
	if( !bitmap.CreateFromBitmapDimensions( input ) )
	{
		CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepLoad: failed to create output for input texture %S", m_path.c_str() );
		return false;
	}
	memcpy( bitmap.GetRawData(), input.GetRawData(), input.GetRawDataSize() );
	return true;
}
