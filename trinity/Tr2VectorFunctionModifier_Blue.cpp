// Copyright © 2025 CCP ehf.

#include "Tr2VectorFunctionModifier.h"

BLUE_DEFINE( Tr2VectorFunctionModifier );

const Be::ClassInfo* Tr2VectorFunctionModifier::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VectorFunctionModifier, "Wrapper object for ITriVectorFunction implementer that adds world or view space offset and scaling" )
		MAP_INTERFACE( ITriVectorFunction )
		MAP_INTERFACE( Tr2VectorFunctionModifier )

		MAP_ATTRIBUTE(
			"clientBall",
			m_clientBall,
			"Object to be manipulated by offset",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"offsetPosition",
			m_offsetPosition,
			"offset before camera manipulation",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"scaleModifier",
			m_scaleModifier,
			"Scales position by a constant",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"useViewSpace",
			m_useViewSpace,
			"Calcuates offset in view space",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"useSystemCoordinates",
			m_useSystemCoordinates,
			"Whether to use the system coordinates for the given position or the default of ship coordinates",
			Be::READWRITE )
	EXPOSURE_END()
}
