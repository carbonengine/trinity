// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionModifierTransformOffset.h"

BLUE_DEFINE( EveDistributionModifierTransformOffset );

const Be::ClassInfo* EveDistributionModifierTransformOffset::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionModifierTransformOffset, ":jessica-icon: clock-rotate-left\n" )
		MAP_INTERFACE( EveDistributionModifierTransformOffset )
		MAP_INTERFACE( IEveDistributionModifier )

		MAP_ATTRIBUTE( "translation", m_translation, "use for static or animated value that doesn't change over lifetime", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "rotation", m_rotation, "use for static or animated value that doesn't change over lifetime", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "scaling", m_scale, "use for static or animated value that doesn't change over lifetime", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "translationCurve", m_translationCurve, "Vector function slot for translating object over lifetime", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotationCurve", m_rotationCurve, "Quaternion function slot for rotating object over lifetime", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaleCurve", m_scaleCurve, "Vector function slot for scaling object over Lifetime", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
