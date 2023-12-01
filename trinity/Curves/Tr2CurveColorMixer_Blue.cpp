#include "Tr2CurveColorMixer.h"

BLUE_DEFINE( Tr2CurveColorMixer );

const Be::ClassInfo* Tr2CurveColorMixer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveColorMixer, ":jessica-icon: tree/tricolor.png" )
		MAP_INTERFACE( Tr2CurveColorMixer )
		MAP_INTERFACE( ITriColorFunction )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( ITriCurveLength )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"color1",
			m_color1,
			"",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"color2",
			m_color2,
			"",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"lerpValue",
			m_lerpValue,
			"color1 + (1 - x) color2",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"saturation",
			m_saturation,
			"[0:inf] 0=grayscale 1=normal (output capped so feel free to over-saturate)",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"brightness",
			m_brightness,
			"[0:inf] 0=black 1=normal",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"currentValue",
			m_currentValue,
			"Curve value after the last update",
			Be::READ )

		MAP_ATTRIBUTE(
			"convertedLinearValue",
			m_convertedLinearValue,
			"Curve value after applying SRGB transformation to the current value, this is used for the cases in" 
			"\n which a linear transformation to the values is occurring such as fog and light colors",
			Be::READ )

		MAP_METHOD_AND_WRAP(
			"GetValueAt",
			GetValue,
			"Returns curve value at specified time"
			":param time: input time"
		)

		EXPOSURE_END()
}