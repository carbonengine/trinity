#include "StdAfx.h"
#include "KDdroneManagementTree.h"

BLUE_DEFINE( KDdroneManagementTree );

const Be::ClassInfo* KDdroneManagementTree::ExposeToBlue()
{
	EXPOSURE_BEGIN( KDdroneManagementTree, "" )
		MAP_INTERFACE( KDdroneManagementTree )
	EXPOSURE_END()
}