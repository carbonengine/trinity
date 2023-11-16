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

BLUE_REGISTER_RESOURCE_EXTENSION( L"gr2", CreateStaticGeometryResource );


const Be::ClassInfo* TriGeometryRes::ExposeToBlue()
{
    EXPOSURE_BEGIN( TriGeometryRes, "" )
        MAP_INTERFACE( TriGeometryRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_INTERFACE( ITr2InstanceData )
		MAP_INTERFACE( ITr2GpuBuffer )
		MAP_ICACHEABLE_METHODS()

		MAP_PROPERTY_READONLY
		(
			"modelCount",
			GetModelCount,
			"Gets the count of models in this geometry resource"
		)
		MAP_PROPERTY_READONLY
		(
			"meshCount",
			GetMeshCount,
			"Gets the count of meshes in this geometry resource"
		)
		MAP_PROPERTY_READONLY
		(
			"animationCount",
			GetAnimationCount,
			"Gets the count of animations in this geometry resource"
		)
		MAP_ATTRIBUTE( "name", m_name, "Name for debugging/logging", Be::READWRITE )

		MAP_ATTRIBUTE( "forceLod", m_forceLod, "Force geometry to use a specific LOD\n:jessica-group: Debug", Be::READWRITE )
		MAP_ATTRIBUTE( "forcedLodIndex", m_forcedLodIndex, "LOD index to use when forceLod is on\n:jessica-group: Debug", Be::READWRITE )
		
		MAP_METHOD_AND_WRAP
		(
			"GetModelCount", 
			GetModelCount, 
			"Gets the count of models"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetModelName", 
			GetModelName, 
			"Gets the name of the indexed model\n"
			":param idx: model index"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetMeshCount", 
			GetMeshCount, 
			"Gets the count of meshes"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetMeshName", 
			GetMeshName, 
			"Gets the name of the indexed mesh\n"
			":param idx: mesh index"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetMeshAreaCount",
			GetMeshAreaCount, 
			"Gets the count of areas within the indexed mesh\n"
			":param idx: mesh index"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetMeshAreaName", 
			GetMeshAreaName, 
			"Gets the name of the indexed area within the indexed mesh\n"
			":param meshIdx: mesh index\n"
			":param areaIdx: area index\n"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetAreaBoundingBox", 
			GetAreaBoundingBoxFromScript, 
			"Get the bounding box of the specified area within the mesh\n"
			":param meshIdx: mesh index\n"
			":param areaIdx: area index\n"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetBoundingBox", 
			GetBoundingBoxFromScript, 
			"Get the bounding box of the specified mesh\n"
			":param meshIdx: mesh index\n"
		)
		MAP_METHOD_AND_WRAP(
			"GetBoundingSphere", 
			GetBoundingSphereFromScript,
			"Get the bounding sphere of the specified area\n"
			":param meshIdx: mesh index\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"CalculateBoundingBoxFromTransform", 
			CalculateBoundingBoxFromTransform, 
			"Get the mesh's bounding box with all points transformed using transform.\n"
			":param meshIdx: mesh index\n"
			":param transform: transform matrix\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"RecalculateBoundingSphere",
			RecalculateBoundingSphere,
			"Rebuild bounding sphere info for these meshes"
		)

		MAP_METHOD_AND_WRAP
		( 
			"Reload", 
			Reload, 
			"Forces a reload from disk"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetIntersectionPointNormalBone", 
			GetIntersectionPointNormalBoneFromScript,
			"( pos, dir ) ->( near, far )\nGet the near intersection points and the normal between a ray and the geometry.\n"
			":param pos: ray origin\n"
			":param direction: ray direction\n"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetAreaIntersectionPointNormalBone", 
			GetAreaIntersectionPointNormalBoneFromScript,
			"( pos, dir ) ->( near, far )\nGet the near intersection points and the normal between a ray and the geometry.\n"
			":param pos: ray origin\n"
			":param direction: ray direction\n"
			":param areaIx: the mesh area index\n"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetMeshVertexElements",
			GetMeshVertexElements,
			"Returns a list of (usage, usage index) tuples for vertex layout of the specified mesh.\n"
			":param idx: mesh index"
		)

		MAP_METHOD_AND_WRAP(
			"SaveMesh",
			SaveMesh,
			"Save mesh data to a granny file\n"
			":param path: desctination file path\n"
			":param mesh: index of the mesh to save"
		)
    EXPOSURE_CHAINTO( BlueAsyncRes )
}


namespace
{
	std::pair<Vector3, float> BoundingSphereFromPointsScript( const std::vector<Vector3>& points )
	{
		Vector4 sphere( 0, 0, 0, 0 );
		if( points.empty() )
		{
			::BoundingSphereFromPoints( sphere, nullptr, 0 );
		}
		else
		{
			std::vector<const Vector3*> pointers;
			pointers.reserve( points.size() );
			auto ptr = &points[0];
			for( size_t i = 0; i < points.size(); ++i )
			{
				pointers.push_back( ptr++ );
			}
			::BoundingSphereFromPoints( sphere, &pointers[0], points.size() );
		}
		return std::make_pair( Vector3( sphere.x, sphere.y, sphere.z ), sphere.w );
	}
}

MAP_FUNCTION_AND_WRAP(
	"BoundingSphereFromPoints",
	BoundingSphereFromPointsScript,
	"Generates bounding sphere around a given points.\n"
	":param points: list of 3D points in space" );