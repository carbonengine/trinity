// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "EveChildBulletStorm.h"

BLUE_DEFINE( EveChildBulletStorm );

const Be::ClassInfo* EveChildBulletStorm::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildBulletStorm, "" )
		MAP_INTERFACE( EveChildBulletStorm )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE )

		MAP_ATTRIBUTE( "objectCount", m_objectCount, "", Be::READ )
		MAP_ATTRIBUTE( "clipSphere", m_clipSphere, "", Be::READ )
		MAP_ATTRIBUTE( "multiplier", m_multiplier, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "speed", m_speed, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "sourceLocatorSet", m_sourceLocatorSet, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "range", m_range, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "sourceObject", m_sourceObject, "", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "sourceRadius", m_sourceRadius, "", Be::READ )

		MAP_ATTRIBUTE( "targetObjects", m_targetObjects, "", Be::READ | Be::NOTIFY )

		MAP_ATTRIBUTE( "effect", m_effect, "The shader", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "Rebuild", Rebuild, "Rebuild internal buffers" )
		MAP_METHOD_AND_WRAP( "StartEffect", StartEffect, "Start Effect" )
		MAP_METHOD_AND_WRAP( "StopEffect", StopEffect, "Stop Effect" )
		MAP_METHOD_AND_WRAP( "CanChangeState", CanChangeState, "Can start or stop the effect without visual errors?" )

	EXPOSURE_END()
}