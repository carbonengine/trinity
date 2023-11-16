////////////////////////////////////////////////////////////
//
//    Created:   August 2016
//    Copyright: CCP 2016
//
#include "StdAfx.h"
#include "EveChildQuad.h"

BLUE_DEFINE( EveChildQuad );

const Be::ClassInfo* EveChildQuad::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildQuad, "" )
        MAP_INTERFACE( EveChildQuad )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST	)
		MAP_ATTRIBUTE( "display", m_display, "Show/hide object", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "editMode", m_editMode, "Turn on when tweaking effect", Be::READWRITE )
		MAP_ATTRIBUTE( "effect", m_effect, "Effect to be used for rendering a quad", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "Quad color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "brightness", m_brightness, "Quad brightness", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling,"", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "viewRotation", m_viewRotation, "Quad rotation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"minScreenSize", 
			m_minScreenSize, 
			"Minimal size of object on screen, objects smaller than this size are not rendered.\n:jessica-group: LOD", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentScreenSize", m_currentScreenSize, "Screen size for last frame\n:jessica-group: LOD", Be::READ )
		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform if useSRT is set." )
    EXPOSURE_END()
}