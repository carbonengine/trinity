#include "StdAfx.h"
#include "TriTransformParameter.h"
#include "TriConstants.h"

BLUE_DEFINE( TriTransformParameter );

const Be::ClassInfo* TriTransformParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriTransformParameter, "Effect parameter used for 4x4 transforms" )
		MAP_INTERFACE( ITriEffectParameter )

		MAP_ATTRIBUTE( "name", m_name, "Effect parameter name from .fx file", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "Scaling in transform", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotationCenter", m_rotationCenter, "Center of rotation in transform", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "Rotation in transform", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "Translation in transform", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform,	
			"World transform. This needs to be filled in from the outside,\n"
			"for example with a TriValueBinding from the worldTransform of the\n"
			"object holding the effect this is used in.", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE_WITH_CHOOSER( "transformBase", m_transformBase, "Funky transform base from old Trinity", Be::READWRITE | Be::PERSIST | Be::ENUM, TriTransformBase )
	EXPOSURE_END()
}

