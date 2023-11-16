#include "StdAfx.h"
#include "TriGrannyRes.h"
#include "TriGeometryRes.h"



BLUE_DEFINE( Tr2GrannyIntersectionResult );

const Be::ClassInfo* Tr2GrannyIntersectionResult::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GrannyIntersectionResult, "" )
		MAP_ATTRIBUTE( "position", m_result.position, "", Be::READWRITE )
		MAP_ATTRIBUTE( "hasPosition", m_result.hasPosition, "", Be::READWRITE )
		MAP_ATTRIBUTE( "normal", m_result.normal, "", Be::READWRITE )
		MAP_ATTRIBUTE( "hasNormal", m_result.hasNormal, "", Be::READWRITE )
		MAP_ATTRIBUTE( "uv", m_result.uv, "", Be::READWRITE )
		MAP_ATTRIBUTE( "hasUv", m_result.hasUv, "", Be::READWRITE )
		MAP_ATTRIBUTE( "boneIndex", m_result.boneIndex, "", Be::READWRITE )
		MAP_ATTRIBUTE( "hasBoneIndex", m_result.hasBoneIndex, "", Be::READWRITE )
		MAP_ATTRIBUTE( "meshIndex", m_result.meshIndex, "", Be::READWRITE )
		MAP_ATTRIBUTE( "areaIndex", m_result.areaIndex, "", Be::READWRITE )
		EXPOSURE_END()
}


namespace
{
	std::pair<const granny_data_type_definition*, int32_t> FindGrannyComponent( const char* componentName, const granny_data_type_definition* vertexFormat )
	{
		int32_t offset = 0;
		while( vertexFormat->Type != GrannyEndMember )
		{
			if( strcmp( vertexFormat->Name, componentName ) == 0 )
			{
				return std::make_pair( vertexFormat, offset );
			}

			offset += GrannyGetMemberTypeSize( vertexFormat++ );
		}
		return std::make_pair( nullptr, 0 );
	}

	Vector3 ExtractVector3( const uint8_t* vertex, const std::pair<const granny_data_type_definition*, int32_t>& element )
	{
		vertex += element.second;
		if( element.first->Type == GrannyReal16Member )
		{
			return *reinterpret_cast<const Vector3_16*>( vertex );
		}
		else
		{
			return *reinterpret_cast<const Vector3*>( vertex );
		}
	}

	Vector2 ExtractVector2( const uint8_t* vertex, const std::pair<const granny_data_type_definition*, int32_t>& element )
	{
		vertex += element.second;
		if( element.first->Type == GrannyReal16Member )
		{
			return *reinterpret_cast<const Vector2_16*>( vertex );
		}
		else
		{
			return *reinterpret_cast<const Vector2*>( vertex );
		}
	}

	bool RayTriangleIntersection( float& dist, float& u, float& v, const Vector3& pos, const Vector3& dir, const uint8_t** vertices, const std::pair<const granny_data_type_definition*, int32_t>& position )
	{
		Vector3 p0 = ExtractVector3( vertices[0], position );
		Vector3 p1 = ExtractVector3( vertices[1], position );
		Vector3 p2 = ExtractVector3( vertices[2], position );

		Matrix m;
		Vector4 vec;

		m.m[0][0] = p1.x - p0.x;
		m.m[1][0] = p2.x - p0.x;
		m.m[2][0] = -dir.x;
		m.m[3][0] = 0.0f;
		m.m[0][1] = p1.y - p0.y;
		m.m[1][1] = p2.y - p0.y;
		m.m[2][1] = -dir.y;
		m.m[3][1] = 0.0f;
		m.m[0][2] = p1.z - p0.z;
		m.m[1][2] = p2.z - p0.z;
		m.m[2][2] = -dir.z;
		m.m[3][2] = 0.0f;
		m.m[0][3] = 0.0f;
		m.m[1][3] = 0.0f;
		m.m[2][3] = 0.0f;
		m.m[3][3] = 1.0f;

		vec.x = pos.x - p0.x;
		vec.y = pos.y - p0.y;
		vec.z = pos.z - p0.z;
		vec.w = 0.0f;

		if( Inverse( m, m ) )
		{
			vec = Transform( vec, m );
			if( ( vec.x >= 0.0f ) && ( vec.y >= 0.0f ) && ( vec.x + vec.y <= 1.0f ) && ( vec.z >= 0.0f ) )
			{
				u = vec.x;
				v = vec.y;
				dist = fabs( vec.z );
				return true;
			}
		}

		return false;
	}

	void FillResult( Tr2GrannyIntersectionResult::Result& result, const uint8_t** triangle, const granny_data_type_definition* vertexFormat, float u, float v )
	{
		auto position = FindGrannyComponent( GrannyVertexPositionName, vertexFormat );
		if( position.first )
		{
			Vector3 p0 = ExtractVector3( triangle[0], position );
			Vector3 p1 = ExtractVector3( triangle[1], position );
			Vector3 p2 = ExtractVector3( triangle[2], position );

			result.position = p0 + ( p1 - p0 ) * u + ( p2 - p0 ) * v;
			result.hasPosition = true;
		}
		else
		{
			result.position = Vector3( 0, 0, 0 );
			result.hasPosition = false;
		}
		auto normal = FindGrannyComponent( GrannyVertexNormalName, vertexFormat );
		if( normal.first )
		{
			Vector3 p0 = ExtractVector3( triangle[0], normal );
			Vector3 p1 = ExtractVector3( triangle[1], normal );
			Vector3 p2 = ExtractVector3( triangle[2], normal );

			result.normal = Normalize( p0 + ( p1 - p0 ) * u + ( p2 - p0 ) * v );
			result.hasNormal = true;
		}
		else
		{
			result.normal = Vector3( 0, 0, 0 );
			result.hasNormal = false;
		}
		auto uv = FindGrannyComponent( GrannyVertexTextureCoordinatesName "0", vertexFormat );
		if( uv.first )
		{
			Vector2 p0 = ExtractVector2( triangle[0], uv );
			Vector2 p1 = ExtractVector2( triangle[1], uv );
			Vector2 p2 = ExtractVector2( triangle[2], uv );

			result.uv = p0 + ( p1 - p0 ) * u + ( p2 - p0 ) * v;
			result.hasUv = true;
		}
		else
		{
			result.uv = Vector2( 0, 0 );
			result.hasUv = false;
		}
		auto boneIndex = FindGrannyComponent( GrannyVertexBoneIndicesName, vertexFormat );
		if( boneIndex.first )
		{
			result.boneIndex = triangle[0][boneIndex.second];
			result.hasBoneIndex = true;
		}
		else
		{
			result.boneIndex = -1;
			result.hasBoneIndex = false;
		}
	}

	int32_t FindAreaIndex( int32_t index, const granny_tri_topology& topology )
	{
		index /= 3;
		for( int32_t i = 0; i < topology.GroupCount; ++i )
		{
			if( index >= topology.Groups[i].TriFirst && index < topology.Groups[i].TriFirst + topology.Groups[i].TriCount )
			{
				return i;
			}
		}
		return -1;
	}

	void GetTriangleVertices( const uint8_t** triangle, const granny_mesh& mesh, int32_t index, int32_t vertexSize )
	{
		if( mesh.PrimaryTopology->Indices16 )
		{
			triangle[0] = mesh.PrimaryVertexData->Vertices + mesh.PrimaryTopology->Indices16[index++] * vertexSize;
			triangle[1] = mesh.PrimaryVertexData->Vertices + mesh.PrimaryTopology->Indices16[index++] * vertexSize;
			triangle[2] = mesh.PrimaryVertexData->Vertices + mesh.PrimaryTopology->Indices16[index++] * vertexSize;
		}
		else
		{
			triangle[0] = mesh.PrimaryVertexData->Vertices + mesh.PrimaryTopology->Indices[index++] * vertexSize;
			triangle[1] = mesh.PrimaryVertexData->Vertices + mesh.PrimaryTopology->Indices[index++] * vertexSize;
			triangle[2] = mesh.PrimaryVertexData->Vertices + mesh.PrimaryTopology->Indices[index++] * vertexSize;
		}
	}

	Tr2GrannyIntersectionResultPtr GrannyRayIntersection( uintptr_t fileinfo, const Vector3& pos, const Vector3& dir, int32_t meshIndex, int32_t areaIndex )
	{
		if( !fileinfo )
		{
			return nullptr;
		}

		auto fi = reinterpret_cast<const granny_file_info*>( fileinfo );

		if( meshIndex >= 0 && meshIndex >= fi->MeshCount )
		{
			return nullptr;
		}
		if( areaIndex >= 0 && meshIndex < 0 )
		{
			return nullptr;
		}
		if( areaIndex >= 0 )
		{
			if( !fi->Meshes[meshIndex]->PrimaryTopology )
			{
				return nullptr;
			}
			if( areaIndex >= fi->Meshes[meshIndex]->PrimaryTopology->GroupCount )
			{
				return nullptr;
			}
		}

		int32_t meshCount, meshOffset;
		if( meshIndex >= 0 )
		{
			meshCount = 1;
			meshOffset = meshIndex;
		}
		else
		{
			meshCount = fi->MeshCount;
			meshOffset = 0;
		}

		Tr2GrannyIntersectionResult::Result result;
		float closestDist = std::numeric_limits<float>::max();
		bool foundIntersection = false;

		for( int32_t i = 0; i < meshCount; ++i )
		{
			auto mesh = fi->Meshes[i + meshOffset];
			if( !mesh->PrimaryTopology )
			{
				continue;
			}

			int32_t index, triCount;
			if( areaIndex >= 0 )
			{
				index = mesh->PrimaryTopology->Groups[areaIndex].TriFirst * 3;
				triCount = mesh->PrimaryTopology->Groups[areaIndex].TriCount;
			}
			else
			{
				index = 0;
				if( mesh->PrimaryTopology->Index16Count )
				{
					triCount = mesh->PrimaryTopology->Index16Count / 3;
				}
				else
				{
					triCount = mesh->PrimaryTopology->IndexCount / 3;
				}
			}

			auto vertexFormat = mesh->PrimaryVertexData->VertexType;
			auto vertexSize = GrannyGetTotalObjectSize( vertexFormat );
			auto position = FindGrannyComponent( GrannyVertexPositionName, vertexFormat );
			if( !position.first )
			{
				continue;
			}

			for( int32_t j = 0; j < triCount; ++j )
			{
				const uint8_t* triangle[3];
				GetTriangleVertices( triangle, *mesh, index, vertexSize );
				index += 3;

				float u, v, dist;
				if( RayTriangleIntersection( dist, u, v, pos, dir, triangle, position ) )
				{
					if( dist < closestDist )
					{
						closestDist = dist;
						result.meshIndex = i + meshOffset;
						if( areaIndex >= 0 )
						{
							result.areaIndex = areaIndex;
						}
						else
						{
							result.areaIndex = FindAreaIndex( index - 3, *mesh->PrimaryTopology );
						}
						FillResult( result, triangle, vertexFormat, u, v );
						foundIntersection = true;
					}
				}
			}
		}
		if( foundIntersection )
		{
			Tr2GrannyIntersectionResultPtr ret;
			ret.CreateInstance();

			ret->m_result = result;
			return ret;
		}
		return nullptr;
	}}


BLUE_DEFINE( TriGrannyRes );

#if BLUE_WITH_PYTHON

PyObject * TriGrannyRes:: GetMaterialDictionaryForArea( int mesh, int area )
{
	unsigned int key;

	key = (mesh << 16) | area;

	GrannyMaterialWrapper * mat = m_meshAreaMaterials[key];

	if (mat==0)
	{
		PyObject*emptyDict = PyDict_New();
		return(emptyDict); // return empty list.
	}

	return mat->m_dictionary ;
}

PyObject * TriGrannyRes:: GetMaterialDictionaryStringsForAllAreas()
{
	if (m_allMaterialStringsDictionary == 0)
	{
		PyObject*emptyDict = PyDict_New();
		return(emptyDict); // return empty list.
	}

	return  m_allMaterialStringsDictionary ;
}
#endif



const Be::ClassInfo* TriGrannyRes::ExposeToBlue()
{
    EXPOSURE_BEGIN( TriGrannyRes, "" )
        MAP_INTERFACE( TriGrannyRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_ICACHEABLE_METHODS()

		MAP_METHOD_AND_WRAP
		(
			"CreateGeometryRes", 
			CreateGeometryRes,
			"y = x.CreateGeometryRes()\n"
			"Create a TriGeometryRes from this resource. Useful for baking blendshapes, for example."
		)

		MAP_METHOD_AND_WRAP
		( 
			"BakeBlendshape", 
			BakeBlendshapeFromScript,
			"x.BakeBlendshape( mesh, weights, geom )\n"
			"Bake a blendshape with the given weights, using the resulting vertex buffer to replace the vertex buffer of the given TriGeometryRes.\n"
			":param mesh: index of the mesh.\n"
			":param weights: a list with weights as floating point numbers.\n"
			":param geom: a TriGeometryRes object that receives the resulting vertex buffer. Note that it should have been created with a call to x.CreateGeometryRes().\n"
		)
		MAP_PROPERTY_READONLY
		(
			"modelCount", GetModelCount,
			"Gets the count of models in the Granny file"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetModelName", GetModelName, 
			"s = x.GetModelName( ix )\n"
			"Gets the name of the model with index 'ix'\n"
			":param ix: index of the model\n"
		)
		MAP_PROPERTY_READONLY
		(
			"meshCount", GetMeshCount,
			"Gets the count of meshes in this Granny file."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshCount", GetMeshCount, 
			"Gets the count of meshes in this Granny file."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshAreaCount", GetMeshAreaCount, 
			"Gets the count of areas within a specified meshID in this Granny file.\n"
			":param meshIdx: mesh index"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshName", GetMeshName, 
			"Gets the name of the mesh with the given index.\n"
			":param meshIdx: mesh index"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshMorphCount", GetMeshMorphCount, 
			"Gets the count of morph targets available for the given mesh.\n"
			":param meshIdx: mesh index"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshMorphName", GetMeshMorphName, 
			"Gets the name of given morph target index for the given mesh.\n"
			":param meshIdx: mesh index\n"
			":param morphIdx: morph index"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetAllMeshMorphNamesNoDigits",
			GetAllMeshMorphNamesNoDigits,
			"Returns a list containing the names of all the morph targets,\n"
			"all converted to lowercase and stripped of any digits.  Specialized\n"
			"method to get answers in a format that python wants in a single call.\n"
			":param meshIdx: mesh index\n"
		)
		MAP_PROPERTY_READONLY
		(
			"animationCount", GetAnimationCount,
			"Gets the count of animation in the Granny file"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetAnimationName", GetAnimationName, 
			"Gets the name of the animation with index 'ix'\n"
			":param ix: index of the animation\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetAnimationDuration", GetAnimationDuration, 
			"Gets the duration of the animation with index 'ix'\n"
			":param ix: index of the animation\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"CollectGrannyMaterials", 
			CollectGrannyMaterials,
			"Creates helper material data for the export process"
		)
#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP
		(
			"GetMaterialDictionaryForArea", 
			GetMaterialDictionaryForArea,
			"Gets material data for the given area\n"
			":param meshIdx: mesh index\n"
			":param areaIdx: area index\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"GetMaterialDictionaryStringsForAllAreas", 
			GetMaterialDictionaryStringsForAllAreas,
			"returns all strings for the model"
		)
#endif
		MAP_METHOD_AND_WRAP( "GetTrackGroupCount", GetTrackGroupCount, "Get the number of track groups" )
		MAP_METHOD_AND_WRAP( 
			"GetTrackGroupName", 
			GetTrackGroupName, 
			"Get the name of the track group at index\n" 
			":param groupIdx: track group index"
		)
		MAP_METHOD_AND_WRAP( 
			"GetTransformTrackCount", 
			GetTransformTrackCount, 
			"Get the number of transform tracks under a track group\n"
			":param groupIdx: track group index"
		)
		MAP_METHOD_AND_WRAP( 
			"GetTransformTrackName", 
			GetTransformTrackName, 
			"Get the name of the transform track\n"
			":param groupIdx: track group index\n"
			":param trackIdx: track index\n"
		)
		MAP_METHOD_AND_WRAP( 
			"GetVectorTrackCount", 
			GetVectorTrackCount, 
			"Get the number of transform tracks under a track group\n" 
			":param groupIdx: track group index\n"
		)
		MAP_METHOD_AND_WRAP( 
			"GetVectorTrackName", 
			GetVectorTrackName, 
			"Get the name of the transform track\n" 
			":param groupIdx: track group index\n"
			":param trackIdx: track index\n"
		)
		MAP_METHOD_AND_WRAP( 
			"GetEventTrackCount", 
			GetEventTrackCount, 
			"Get the number of event (text) tracks under a track group\n"
			":param groupIdx: track group index\n"
		)
		MAP_METHOD_AND_WRAP( 
			"GetEventTrackName", 
			GetEventTrackName, 
			"Get the name of the event (text) track\n"
			":param groupIdx: track group index\n"
			":param trackIdx: track index\n"
		)
	EXPOSURE_CHAINTO( BlueAsyncRes )
}

MAP_FUNCTION_AND_WRAP(
	"GrannyRayIntersection",
	GrannyRayIntersection,
	"Returns ray intersection"
	":param granny: granny info pointer\n"
	":param position: ray position\n"
	":param direction: ray direction\n"
	":param meshIndex: mesh index, intersect with all meshes if <0\n"
	":param areaIndex: area index, intersect with all areas if <0\n"
);
