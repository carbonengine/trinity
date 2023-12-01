////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2TexturePipelineStepGenerateMips.h"


bool Tr2TexturePipelineStepGenerateMips::Execute( ImageIO::HostBitmap& bitmap, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>&, const Tr2TexturePipelineParams& ) const
{
	if( !bitmap.IsValid() )
	{
		CCP_LOGERR_CH( s_texturePipelineChannel, "Tr2TexturePipelineStepGenerateMips: invalid input bitmap" );
		return false;
	}

	return bitmap.GenerateMipMaps();
}
