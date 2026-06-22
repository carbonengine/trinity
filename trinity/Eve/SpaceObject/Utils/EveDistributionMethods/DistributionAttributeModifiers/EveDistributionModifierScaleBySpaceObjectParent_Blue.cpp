// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionModifierScaleBySpaceObjectParent.h"


BLUE_DEFINE( EveDistributionModifierScaleBySpaceObjectParent );

const Be::ClassInfo* EveDistributionModifierScaleBySpaceObjectParent::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionModifierScaleBySpaceObjectParent, ":jessica-icon: hands-holding-child\n" )
		MAP_INTERFACE( EveDistributionModifierScaleBySpaceObjectParent )
		MAP_INTERFACE( IEveDistributionModifier )

		MAP_ATTRIBUTE( "authoredForBoundingRadius", m_authoredForBoundingRadius, "insert the perfect boundingSphereSize of the asset the effect is authored for", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "scaleFactor", m_scaleFactor, "how much should the effect adapt. 1.2 -> if the asset is 2x bigger the effect will be 2x1.2 = 2.4 times larger\n:jessica-numeric-range: (0.1,2.0)", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "scaleCurve", m_scaleCurve, "optional Curve if you just want to hardCode size per bounding sphere. X-axis is the bounding size", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
