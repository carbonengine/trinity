// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2PrimitiveSet.h"

BLUE_DEFINE_ABSTRACT( Tr2PrimitiveSet );

const Be::ClassInfo* Tr2PrimitiveSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PrimitiveSet, "" )
		MAP_INTERFACE( Tr2PrimitiveSet )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( ITr2Pickable )
		MAP_INTERFACE( INotify )
		MAP_ATTRIBUTE( "effect", m_effect, "The effect to use to draw the primitive", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pickEffect", m_pickEffect, "The effect used to pick the primitive", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "The name of the primitive", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "viewOriented", m_viewOriented, "Rotate the lineset towards the camera\n"
													   "It assumes that the lineset is setup in a way,\n"
													   "that the normal of it is the world z and the up vector the world Y.",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaleByDistanceToView", m_scaleByDistanceToView, "Scale the primitive dependent on the distance to the view position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scale", m_scale, "The current scale of the primitive", Be::READ )
		MAP_ATTRIBUTE( "color", m_color, "The color of the primitive", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "primitive sets local transform", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "primitive sets world transform", Be::READ )
#if BLUE_WITH_PYTHON
		MAP_ATTRIBUTE( "_userData", m_pythonUserData, "Pure python user data attached to the primitive set", Be::READWRITE )
#endif
		MAP_METHOD_AND_WRAP( "SetCurrentColor", SetCurrentColor, "( color )\nSets the color of the primitive\n"
																 ":param color: primitive color" )
	EXPOSURE_END()
}