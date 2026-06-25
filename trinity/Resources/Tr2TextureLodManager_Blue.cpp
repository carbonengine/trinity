// Copyright © 2022 CCP ehf.

#include "StdAfx.h"
#include "Tr2TextureLodManager.h"
#include "TriTextureRes.h"


BLUE_DEFINE_ABSTRACT( Tr2TextureLodManager );


const Be::ClassInfo* Tr2TextureLodManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TextureLodManager, "" )
		MAP_INTERFACE( Tr2TextureLodManager )



		MAP_METHOD_AND_WRAP( "GetManagedTextures", GetManagedTextures, "Returns all textures managed by this manager" )
		MAP_PROPERTY( "useLowResVtaFiles", GetUseLowResVtaFilesSetting, SetUseLowResVtaFilesSetting, "Use _lowdetail file versions for VTA textures" )
	EXPOSURE_END();
}


namespace
{
Tr2TextureLodManagerPtr GetTextureLodManager()
{
	return &Tr2TextureLodManager::Instance();
}
}

MAP_FUNCTION_AND_WRAP(
	"GetTextureLodManager",
	GetTextureLodManager,
	"Returns a global instance of the texture LOD manager" );
