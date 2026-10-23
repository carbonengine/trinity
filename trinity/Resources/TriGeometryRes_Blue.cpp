// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriGeometryRes.h"
#include "Tr2Mesh.h"
#include "Utilities/BoundingSphere.h"

BLUE_DEFINE( TriGeometryRes );

IBlueResource* CreateStaticGeometryResource( const wchar_t* name )
{
	TriGeometryResPtr p;
	p.CreateInstance();
	p->m_name = CW2A( name );
	return p.Detach();
}

#if WITH_GRANNY
BLUE_REGISTER_RESOURCE_EXTENSION( L"gr2", CreateStaticGeometryResource );
#endif
BLUE_REGISTER_RESOURCE_EXTENSION( L"cmf", CreateStaticGeometryResource );


BLUE_DEFINE( Tr2RaycastGeometryRes );

const Be::ClassInfo* Tr2RaycastGeometryRes::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2RaycastGeometryRes, "Session-scoped CPU raycast data (BVH) for a geometry resource." )

		MAP_INTERFACE( Tr2RaycastGeometryRes )
		MAP_INTERFACE( IBlueResource )

	EXPOSURE_CHAINTO( BlueAsyncRes )
}


const Be::ClassInfo* TriGeometryRes::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriGeometryRes, "" )
		MAP_INTERFACE( TriGeometryRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_INTERFACE( ITr2InstanceData )
		MAP_ICACHEABLE_METHODS()

		MAP_METHOD_AND_WRAP(
			"Reload",
			Reload,
			"Forces a reload from disk" )

		MAP_PROPERTY_READONLY(
			"animationCount",
			GetAnimationCount,
			"Gets the count of animations in this geometry resource" )

	EXPOSURE_CHAINTO( Tr2GeometryProvider )
}
