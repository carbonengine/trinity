////////////////////////////////////////////////////////////
//
//    Created:   August 2021
//    Copyright: CCP 2021
//
#include "StdAfx.h"
#include "EveEntity.h"

BLUE_DEFINE( EveEntity );

namespace EntityComponents
{
	const Be::VarChooser ReflectionModeChooser[] = {
		{ "Never", BeCast( REFLECT_NEVER ), "Never render into the reflection map" },
		{ "LowMediumAndHigh", BeCast( REFLECT_LOW_MEDIUM_HIGH ), "Render into the reflection map when reflection settings is set to low, medium or high" },
		{ "MediumAndHigh", BeCast( REFLECT_MEDIUM_AND_HIGH ), "Render into the reflection map when reflection settings is set to medium or high" },
		{ "High", BeCast( REFLECT_HIGH ), "Only render into the reflection map when reflection settings is set to high" },
		{ 0 }
	};

	BLUE_REGISTER_ENUM_EX( "ReflectionModeType", ReflectionMode, ReflectionModeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

}


const Be::ClassInfo* EveEntity::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveEntity, "" )
		MAP_INTERFACE( EveEntity )
	EXPOSURE_END()
}
