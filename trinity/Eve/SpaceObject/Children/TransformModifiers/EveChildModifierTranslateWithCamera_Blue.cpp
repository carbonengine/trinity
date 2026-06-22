// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierTranslateWithCamera.h"

BLUE_DEFINE( EveChildModifierTranslateWithCamera );

const Be::ClassInfo* EveChildModifierTranslateWithCamera::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierTranslateWithCamera, "" )
		MAP_INTERFACE( EveChildModifierTranslateWithCamera )
		MAP_INTERFACE( IEveChildTransformModifier )

		MAP_ATTRIBUTE( "attachedToCamera", m_attachedToCamera, "Ignores the position of the parent, and attaches this child to the camera", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}