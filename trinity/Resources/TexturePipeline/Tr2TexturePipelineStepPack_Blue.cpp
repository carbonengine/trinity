// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2TexturePipelineStepPack.h"

namespace
{

Be::VarChooser ChannelChooser[] = {
	{ "RED",
	  BeCast( 2 ),
	  "" },
	{ "GREEN",
	  BeCast( 1 ),
	  "" },
	{ "BLUE",
	  BeCast( 0 ),
	  "" },
	{ "ALPHA",
	  BeCast( 3 ),
	  "" },
	{ 0 }
};
}


BLUE_DEFINE( Tr2TexturePackChannel );

const Be::ClassInfo* Tr2TexturePackChannel::ExposeToBlue(){
	EXPOSURE_BEGIN( Tr2TexturePackChannel, "" )
		MAP_INTERFACE( Tr2TexturePackChannel )

			MAP_ATTRIBUTE( "path", m_path, "Res path to the input texture", Be::READWRITE | Be::PERSIST )
				MAP_ATTRIBUTE_WITH_CHOOSER( "channel", m_channel, "Channel to take from the input texture", Be::READWRITE | Be::PERSIST | Be::ENUM, ChannelChooser )
					MAP_ATTRIBUTE( "fill", m_fill, "Constant color fill when no input texture is provided", Be::READWRITE | Be::PERSIST )

						EXPOSURE_END()
}

BLUE_DEFINE( Tr2TexturePipelineStepPack );

const Be::ClassInfo* Tr2TexturePipelineStepPack::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TexturePipelineStepPack, "" )
		MAP_INTERFACE( Tr2TexturePipelineStepPack )
		MAP_INTERFACE( ITr2TexturePipelineStep )

		MAP_ATTRIBUTE_WITH_CHOOSER( "format", m_format, "Resulting pixel format", Be::READWRITE | Be::PERSIST | Be::ENUM, Tr2RenderContextEnum_PixelFormat_Chooser )
		MAP_ATTRIBUTE( "r", m_r, "Red channel source", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "g", m_g, "Green channel source", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "b", m_b, "Blue channel source", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "a", m_a, "Alpha channel source", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
