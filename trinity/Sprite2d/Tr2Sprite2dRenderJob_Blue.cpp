////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2011
// Copyright:	CCP 2011
//

#include "StdAfx.h"
#include "Tr2Sprite2dRenderJob.h"

BLUE_DEFINE( Tr2Sprite2dRenderJob );

const Be::ClassInfo* Tr2Sprite2dRenderJob::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dRenderJob, "" )
		MAP_INTERFACE( ITr2SpriteObject )
		MAP_INTERFACE( Tr2Sprite2dRenderJob )

		MAP_ATTRIBUTE
		(
			"renderJob",
			m_renderJob,
			"The render job to be executed",
			Be::READWRITE
		)
	EXPOSURE_CHAINTO( Tr2SpriteObjectBase )
}