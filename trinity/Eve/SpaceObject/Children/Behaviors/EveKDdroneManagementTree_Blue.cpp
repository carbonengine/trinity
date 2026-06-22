// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveKDdroneManagementTree.h"

BLUE_DEFINE( EveKDdroneManagementTree );

const Be::ClassInfo* EveKDdroneManagementTree::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveKDdroneManagementTree, "" )
		MAP_INTERFACE( EveKDdroneManagementTree )
	EXPOSURE_END()
}