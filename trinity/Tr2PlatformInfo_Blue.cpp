// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PlatformInfo.h"

BLUE_DEFINE( Tr2PlatformInfo );

Be::VarChooser Tr2PlatformInfoStaticCapChooser[] = {
	{ "NON_SYNCHRONIZED_LOCKS",
	  BeCast( Tr2PlatformInfo::NON_SYNCHRONIZED_LOCKS ),
	  "Can map buffers without plaform synchronization" },
	{ "BUFFER_SHADER_RESOURCES",
	  BeCast( Tr2PlatformInfo::BUFFER_SHADER_RESOURCES ),
	  "Can access buffers as shader resources" },
	{ "UNORDERED_ACCESS",
	  BeCast( Tr2PlatformInfo::UNORDERED_ACCESS ),
	  "Can write to textures and buffers from shaders using unordered access views" },
	{ "COMPUTE",
	  BeCast( Tr2PlatformInfo::COMPUTE ),
	  "Has compute shader support" },
	{ "TEXTURE_ARRAYS",
	  BeCast( Tr2PlatformInfo::TEXTURE_ARRAYS ),
	  "Supports texture arrays" },
	{ "MSAA_SAMPLE",
	  BeCast( Tr2PlatformInfo::MSAA_SAMPLE ),
	  "Supports sampling MSAA textures" },

	{ "TAA",
	  BeCast( Tr2PlatformInfo::TAA ),
	  "Space scenes support TAA" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX(
	"PlatformStaticCap",
	Tr2PlatformInfo::StaticCap,
	Tr2PlatformInfoStaticCapChooser,
	ENUM_REG_ENUM_OBJECT_ON_MODULE );



const Be::ClassInfo* Tr2PlatformInfo::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PlatformInfo, "" )
		MAP_INTERFACE( Tr2PlatformInfo )

		MAP_PROPERTY_READONLY( "platformName", GetPlatformName, "AL platform name abbreviation" )
		MAP_PROPERTY_READONLY( "platformID", GetPlatformID, "AL platform id" )
		MAP_PROPERTY_READONLY( "isLowPerformance", IsLowPerformance, "Is the AL platform low-end" )

		MAP_METHOD_AND_WRAP(
			"GetStaticCap",
			GetStaticCap,
			"Returns AL platform capability flag value.\n"
			":param cap: capability name (see trinity.PlatformStaticCap)\n" )
	EXPOSURE_END()
}
