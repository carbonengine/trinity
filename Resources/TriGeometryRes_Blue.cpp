#include "StdAfx.h"
#include "TriGeometryRes.h"
#include "Tr2Mesh.h"

BLUE_DEFINE( TriGeometryRes );

static Be::VarChooser TriGeometryCollisionResultFlagsChooser[] =
{
	{
		"ANY",     
		BeCast( COLLISION_RESULT_ANY ),     
		"Collision function will return the first intersection it finds"
	},
	{
		"CLOSEST",     
		BeCast( COLLISION_RESULT_CLOSEST ),     
		"Collision function will return the closest intersection to ray origin"
	},
	{ 0 },
};

BLUE_REGISTER_ENUM_EX( "TriGeometryCollisionResultFlags", 
					  TriGeometryCollisionResultFlags, 
					  TriGeometryCollisionResultFlagsChooser, 
					  ENUM_REG_ENUM_OBJECT_ON_MODULE );

static Be::VarChooser TriGeometryCollisionCullingFlagsChooser[] =
{
	{
		"CCW",     
		BeCast( COLLISION_CULL_CCW ),     
		"CCW culling"
	},
	{
		"CW",     
		BeCast( COLLISION_CULL_CW ),     
		"CW culling"
	},
	{
		"NONE",     
		BeCast( COLLISION_CULL_NONE ),     
		"None culling"
	},
	{ 0 },
};

BLUE_REGISTER_ENUM_EX( "TriGeometryCollisionCullingFlags", 
					  TriGeometryCollisionCullingFlags, 
					  TriGeometryCollisionCullingFlagsChooser, 
					  ENUM_REG_ENUM_OBJECT_ON_MODULE );

IBlueResource* CreateStaticGeometryResource( const wchar_t* name )
{
	TriGeometryResPtr p;
	p.CreateInstance();
	p->SetIsDynamic( false );
	p->m_name = CW2A( name );
	return p.Detach();
}

IBlueResource* CreateDynamicGeometryResource( const wchar_t* name )
{
	TriGeometryResPtr p;
	p.CreateInstance();
	p->SetIsDynamic( true );
	p->m_name = CW2A( name );
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"gr2", CreateStaticGeometryResource );
BLUE_REGISTER_RESOURCE_EXTENSION( L"gr2dyn", CreateDynamicGeometryResource );


const Be::ClassInfo* TriGeometryRes::ExposeToBlue()
{
    EXPOSURE_BEGIN( TriGeometryRes, "" )
        MAP_INTERFACE( TriGeometryRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_INTERFACE( ITr2InstanceData )
		MAP_INTERFACE( ITr2GpuBuffer )
		MAP_ICACHEABLE_METHODS()

		MAP_ATTRIBUTE( "isDynamicGeometry", m_isDynamicGeometry, "is for dynamic geometry (cpu skinning)", Be::READ )
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

		MAP_METHOD_AND_WRAP( 
			"GetMeshSurfaceArea", 
			GetMeshSurfaceArea,
			"Gets the surface area for a particular meshID"
			"\n"
			"\nArguments:"
			"\nmeshID - the mesh to calculate the surface area for")
		
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
			"Gets the name of the indexed model"
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
			"Gets the name of the indexed mesh"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetMeshAreaCount",
			GetMeshAreaCount, 
			"Gets the count of areas within the indexed mesh"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetMeshAreaName", 
			GetMeshAreaName, 
			"Gets the name of the indexed area within the indexed mesh"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetAreaBoundingBox", 
			GetAreaBoundingBoxFromScript, 
			"( nMeshIdx, nAreaIdx )->( min, max )\nGet the bounding box of the specified area within the mesh"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetBoundingBox", 
			GetBoundingBoxFromScript, 
			"( nMeshIdx )->( min, max )\nGet the bounding box of the specified mesh"
		)
		MAP_METHOD_AND_WRAP(
			"GetBoundingSphere", 
			GetBoundingSphereFromScript,
			"( nMeshIdx )->( center, radius )\nGet the bounding sphere of the specified area"
		)
		MAP_METHOD_AND_WRAP
		(
			"CalculateBoundingBoxFromTransform", 
			CalculateBoundingBoxFromTransform, 
			"( meshIdx, transform )->( min, max )\n"
			"Get the mesh's bounding box with all points transformed using transform."
		)

		MAP_METHOD_AND_WRAP
		( 
			"Reload", 
			Reload, 
			"Forces a reload from disk"
		)
		
		MAP_METHOD_AND_WRAP
		(
			"GetRayAreaIntersection", 
			GetRayAreaIntersectionFromScript, 
			"Perform ray - area geometry intersection test"
			"\nReturns (position, uv) tuple with intersection position and texture UV coordinates "
			"or None if no intersection is found"
			"\n"
			"\nArguments:"
			"\norigin - Ray origin (3-tuple)"
			"\ndirection - Ray direction (3-tuple)"
			"\nmeshIndex - Mesh index"
			"\nareaIndex - Area index"
			"\nresultFlags - (optional) Result flags (member of TriGeometryCollisionResultFlags), closest collision by default"
			"\ncullingFlags - (optional) Culling flags (member of TriGeometryCollisionCullingFlags), no culling by default"
		)

		MAP_METHOD_AND_WRAP
		(
			"GetIntersectionPointAndNormal", 
			GetIntersectionPointAndNormalFromScript,
			"( pos, dir ) ->( near, far )\nGet the near intersection points and the normal between a ray and the geometry."
		)

    EXPOSURE_CHAINTO( BlueAsyncRes )
}

void SaveMeshDataToGrannyFile(	Tr2Mesh* mesh,
								char const* resPath,
								char const* rawPath )
{
	TriGeometryRes* geometryRes = mesh->GetGeometryResource();
	TriGeometryResMeshData* meshData = geometryRes->GetMeshData( mesh->GetMeshIndex() );
	TriGeometryRes::SaveMeshToGrannyFile( meshData, rawPath );
	mesh->DeferGeometryLoad( true );
	mesh->SetMeshResPath( resPath );
	mesh->DeferGeometryLoad( false );
}

#if BLUE_WITH_PYTHON
static PyObject* PySaveMeshDataToGrannyFile( PyObject* self, PyObject* args )
{
	PyObject* pyMesh = NULL;
	char const* resPath = NULL;
	char const* rawPath = NULL;

	if( PyArg_ParseTuple( args, "Oss", &pyMesh, &resPath, &rawPath ) )
	{
		if( Tr2Mesh* mesh = BluePythonCast<Tr2Mesh*>( pyMesh ) )
		{
			SaveMeshDataToGrannyFile( mesh, resPath, rawPath );
			return PyBool_FromLong( true );
		}
	}

	return PyBool_FromLong( false );
}

MAP_FUNCTION( "SaveMeshDataToGrannyFile",
              PySaveMeshDataToGrannyFile,
              "Saves mesh data to a Granny file.");

#endif
