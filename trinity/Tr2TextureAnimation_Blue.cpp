////////////////////////////////////////////////////////////
//
//    Created:   March 2023
//    Copyright: CCP 2023
//

#include "StdAfx.h"
#include "Tr2TextureAnimation.h"


BLUE_DEFINE( Tr2TextureAnimation );

const Be::ClassInfo* Tr2TextureAnimation::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TextureAnimation, "Cloud space object child" )
		MAP_INTERFACE( Tr2TextureAnimation )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( 
			"resPath", 
			m_filename, 
			"Path to the animation file\n"
			":jessica-file-filter: Texture animations (*.vta)|*.vta", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "fps", m_fps, "Animation speed: frames per second", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "time", m_time, "Current animation time", Be::READ )
		MAP_ATTRIBUTE( "frame", m_frame, "Current animation frame", Be::READ )

		MAP_ATTRIBUTE( "looped", m_looped, "The animation is looped or does it stop at the last frame", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "paused", m_paused, "Is the animation paused or playing", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "updateOnlyWhenRendered", m_updateOnlyWhenRendered, "Only tick/update the animation if the object that is using the animation is being visible on the screen", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "RestartAnimation", RestartAnimation, "Starts playing the animation from the beginning\n:jessica-favorite:\n" )

		MAP_METHOD_AND_WRAP( "GetChannelNames", GetChannelNames, "Returns a list of channels in the animation file" )
	EXPOSURE_END()
}
