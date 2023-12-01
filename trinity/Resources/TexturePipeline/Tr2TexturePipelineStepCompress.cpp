////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2TexturePipelineStepCompress.h"
#include "Tr2HostBitmap.h"

Tr2TexturePipelineStepCompress::Tr2TexturePipelineStepCompress( IRoot* )
	:m_format( Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM ),
	m_rWeight( 1 ),
	m_gWeight( 1 ),
	m_bWeight( 1 )
{
}

bool Tr2TexturePipelineStepCompress::Execute( ImageIO::HostBitmap& bitmap, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>&, const Tr2TexturePipelineParams& ) const
{
	if( !bitmap.IsValid() )
	{
		return false;
	}
	return true;
}