////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2DynamicBinding.h"

BLUE_DEFINE( Tr2DynamicBinding );

const Be::ClassInfo* Tr2DynamicBinding::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2DynamicBinding, "" )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "Name of the binding", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "destinationObjectPath", m_destinationObjectPath, "Path to the destination object", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "destinationObjectAttribute", m_destinationObjectAttribute, "Path to the destination object attribute", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "destination", m_destination, "Destination object", Be::READ )
		MAP_PROPERTY_READONLY( "isDestinationValid", IsDestinationValid, "Is destination parameter valid" )

		MAP_ATTRIBUTE( "sourceObjectPath", m_sourceObjectPath, "Path to the source object", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "sourceObjectAttribute", m_sourceObjectAttribute, "Path to the source object attribute", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "source", m_source, "source object", Be::READ )
		MAP_ATTRIBUTE( "scale", m_scale, "scale of the source value", Be::READWRITE| Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "bindingDelay", m_bindingDelay, "delay in ms until binding starts", Be::READWRITE | Be::PERSIST )
		
		MAP_PROPERTY_READONLY( "isSourceValid", IsSourceValid, "Is source parameter valid" )

		MAP_ATTRIBUTE( "binding", m_binding, "The binding", Be::READ )

		EXPOSURE_END();
}