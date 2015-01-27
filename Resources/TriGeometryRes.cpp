#include "StdAfx.h"
#include "TriGeometryRes.h"
#include "TriGrannyRes.h"
#include "Tr2PerObjectData.h"
#include "Tr2VertexDefinitionUtilities.h"

#include "TriSettingsRegistrar.h"

#include "Miniball3.h"
#include "Utilities/GeometryUtils.h"

using namespace Tr2RenderContextEnum;

bool g_geometryResNormalizeOnLoad = false;
TRI_REGISTER_SETTING( "geometryResNormalizeOnLoad", g_geometryResNormalizeOnLoad );

bool g_geometryResForce32bitIndex = false;
TRI_REGISTER_SETTING( "geometryResForce32bitIndex", g_geometryResForce32bitIndex );

float g_grannyWarnLoadTime = 1.00f;
TRI_REGISTER_SETTING( "grannyWarnLoadTime", g_grannyWarnLoadTime );

CCP_STATS_DECLARE( geometryResBytes, "Trinity/geometryResBytes", false, CST_MEMORY, "Size of memory occupied by geometry resources." );

bool g_useManagedDX9Buffers = true;

TRI_REGISTER_SETTING( "useManagedDX9Buffers",	g_useManagedDX9Buffers );

//////////////////////////////////////////////////////////////////////////
//
// Structures used for pulling bounds information out of extended data
// in the granny file. There are data type definitions here as well so
// we can use the future proof method of calling GrannyConvertTree to
// pull the data out, even if we change the processing done off-line.

struct BoundingBox
{
	granny_real32 min[3];
	granny_real32 max[3];
};

struct AreaBoundsInfo
{
	BoundingBox bounds;
	granny_int32 vertexCount;
};

struct MeshBoundsInfo
{
	const char* typeName;
	BoundingBox bounds;
	granny_int32 areaCount;
	AreaBoundsInfo* areaInfos;
};

granny_data_type_definition BoundingBoxType[] =
{
	{ GrannyReal32Member, "min", 0, 3 },
	{ GrannyReal32Member, "max", 0, 3 },
	{ GrannyEndMember }
};

granny_data_type_definition AreaBoundsInfoType[] =
{
	{ GrannyInlineMember, "bounds", BoundingBoxType },
	{ GrannyInt32Member, "vertexCount" },
	{ GrannyEndMember }
};

granny_data_type_definition MeshBoundsInfoType[] =
{
	{ GrannyStringMember, "typeName" },
	{ GrannyInlineMember, "bounds", BoundingBoxType },
	{ GrannyReferenceToArrayMember, "areaInfo", AreaBoundsInfoType },
	{ GrannyEndMember }
};

//
//////////////////////////////////////////////////////////////////////////

static void InitializeBounds( Vector3& min, Vector3& max )
{
	min.x = FLT_MAX;
	min.y = FLT_MAX;
	min.z = FLT_MAX;

	max.x = -FLT_MAX;
	max.y = -FLT_MAX;
	max.z = -FLT_MAX;
}

static void UpdateBounds( Vector3& min, Vector3& max, const Vector3& pos )
{
	if( pos.x < min.x )
	{
		min.x = pos.x;
	}
	if( pos.x > max.x )
	{
		max.x = pos.x;
	}

	if( pos.y < min.y )
	{
		min.y = pos.y;
	}
	if( pos.y > max.y )
	{
		max.y = pos.y;
	}

	if( pos.z < min.z )
	{
		min.z = pos.z;
	}
	if( pos.z > max.z )
	{
		max.z = pos.z;
	}
}

static void CopyGrannyName( std::string& dest, const char* src )
{
	// check if valid pointer, THEN assign to std::string
	if( src )
	{
		dest = src;
	}
	else
	{
		dest = "";
	}
}


static void ConvertDataToVector3( Tr2VertexDefinition::DataType elementType, void * src, Vector3 * dest)
{

	switch(elementType)
	{
	case Tr2VertexDefinition::FLOAT16_4:
		{
			D3DXFloat16To32Array( (float*)dest, (const D3DXFLOAT16 *)src, 3 );
			break;
		}
	case Tr2VertexDefinition::FLOAT32_3:
		{
			memcpy( dest, src, 3 * sizeof(float));
			break;
		}
	case Tr2VertexDefinition::FLOAT32_4:
		{
			memcpy( dest, src, 3 * sizeof(float));
			break;
		}
	case Tr2VertexDefinition::SHORT_4:
		{
			ConvertShort4ToVector3( src, dest );
			break;
		}

	case Tr2VertexDefinition::UBYTE_4:
		{
			ConvertUByte4ToVector3(src, dest );
		}

	default:
		{
			dest->x = 0.0f;
			dest->y = 0.0f;
			dest->z = 0.0f;
		}

	}

}



TriGeometryRes::TriGeometryRes(IRoot* lockobj) :
	m_pGrannyFile( NULL ),
	m_memoryUse( 0 ),
	m_meshes( "TriGeometryRes/m_meshes" ),
	m_models( "TriGeometryRes/m_models" ),
	m_skeletons( "TriGeometryRes/m_skeletons" ),
	m_isDynamicGeometry( false ),
	m_inMemoryInfo( NULL ),
	m_immutable( false ),
	m_computeAccess( false )
{
}

TriGeometryRes::~TriGeometryRes()
{
	ReleaseResources( TRISTORAGE_ALL );
	ClearGrannyData();
}

void TriGeometryRes::ClearGrannyData()
{
	for( unsigned int i = 0; i < m_meshes.size(); ++i )
	{
		delete m_meshes[i];
	}
	m_meshes.clear();

	for( unsigned int i = 0; i < m_models.size(); ++i )
	{
		delete m_models[i];
	}
	m_models.clear();

	for( unsigned int i = 0; i < m_skeletons.size(); ++i )
	{
		delete m_skeletons[i];
	}
	m_skeletons.clear();

	if( m_pGrannyFile )
	{
		GrannyFreeFile( m_pGrannyFile );
		m_pGrannyFile = 0;
	}
}

void TriGeometryRes::SetIsDynamic( bool isDynamic )
{
	m_isDynamicGeometry = isDynamic;
}

granny_file_info* TriGeometryRes::GetGrannyInfo()
{
	if( m_pGrannyFile )
	{
		return GrannyGetFileInfo( m_pGrannyFile );
	}
	else
	if( m_inMemoryInfo )
	{
		return m_inMemoryInfo;
	}
	else
	{
		return 0;
	}
}

unsigned int TriGeometryRes::GetAnimationCount()
{
	if( m_pGrannyFile )
	{
		granny_file_info* info = GrannyGetFileInfo( m_pGrannyFile );
		return info->AnimationCount;
	}
	else
	if( m_inMemoryInfo )
	{
		return m_inMemoryInfo->AnimationCount;
	}
	else
	{
		return 0;
	}
}

unsigned int TriGeometryRes::GetModelCount()
{
	return (unsigned int)m_models.size();
}

TriGeometryResModelData* TriGeometryRes::GetModelData( unsigned int modelIx )
{
	if( modelIx < m_models.size() )
	{
		return m_models[modelIx];
	}
	else
	{
		return 0;
	}
}

unsigned int TriGeometryRes::GetMeshCount()
{
	return (unsigned int)m_meshes.size();
}

TriGeometryResMeshData* TriGeometryRes::GetMeshData( unsigned int meshIx )
{
	if( meshIx < m_meshes.size() )
	{
		return m_meshes[meshIx];
	}
	else
	{
		return 0;
	}
}

int TriGeometryRes::GetVertexComponentOffset( const granny_mesh* myMesh, const char* componentName ) const
{
	// now scan granny's vertex-declaration for the component's part and count the offsets
	granny_data_type_definition* vertexFormat = myMesh->PrimaryVertexData->VertexType;
	int componentIx = 0, offset = 0;
	while( vertexFormat[componentIx].Type != GrannyEndMember )
	{
		granny_data_type_definition& src = vertexFormat[componentIx];
		if( strcmp( src.Name, componentName ) == 0 )
		{
			// found it!
			return offset;
		}
		// next
		offset += GrannyGetMemberTypeSize( &src );
		++componentIx;
	}
	// error: component not found in this vertex
	return -1;
}

bool TriGeometryRes::GetBoundingBox( unsigned int meshIx, Vector3& min, Vector3& max ) const
{
	if( meshIx >= m_meshes.size() )
	{
		return false;
	}
	TriGeometryResMeshData* pMesh = m_meshes[meshIx];
	if( !pMesh )
	{
		return false;
	}

	min = pMesh->m_minBounds;
	max = pMesh->m_maxBounds;

	return true;
}

Be::Result<std::string> TriGeometryRes::GetBoundingBoxFromScript( unsigned int meshIx, std::pair<Vector3, Vector3>& bounds ) const
{
	if( meshIx >= m_meshes.size() )
	{
		return Be::Result<std::string>( "Mesh index out of bounds" );
	}
	TriGeometryResMeshData* pMesh = m_meshes[meshIx];
	if( !pMesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	bounds = std::make_pair( pMesh->m_minBounds, pMesh->m_maxBounds );

	return Be::Result<std::string>();
}

bool TriGeometryRes::GetAreaBoundingBox( unsigned int meshIx, unsigned int areaIx, Vector3& min, Vector3& max ) const
{
	// Bail out if the mesh index is out of range
	if( meshIx >= m_meshes.size() )
	{
		return false;
	}

	// Get a handle to the mesh data
	TriGeometryResMeshData* pMesh = m_meshes[meshIx];

	// Bail out if the mesh data is NULL
	if( !pMesh )
	{
		return false;
	}

	// Bail out if the area index is out of range
	if( areaIx >= pMesh->m_areas.size() )
	{
		return false;
	}

	// Finally, get the min and max bounds for the mesh area
	min = pMesh->m_areas[areaIx].m_minBounds;
	max = pMesh->m_areas[areaIx].m_maxBounds;

	return true;
}



bool TriGeometryRes::GetAreaBasis( unsigned int meshIx, unsigned int areaIx, Vector3& pointOnTriangle, Vector3& edge1, Vector3& edge2 ) const
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	// Bail out if the mesh index is out of range
	if( meshIx >= m_meshes.size() )
	{
		return false;
	}

	// Get a handle to the mesh data
	TriGeometryResMeshData* pMesh = m_meshes[meshIx];

	// Bail out if the mesh data is NULL
	if( !pMesh )
	{
		return false;
	}

	// Bail out if the area index is out of range
	if( areaIx >= pMesh->m_areas.size() )
	{
		return false;
	}

	if( !pMesh->m_vertexBuffer.IsValid() || !pMesh->m_indexBuffer.IsValid() )
	{
		return false;
	}

	const TriGeometryResAreaData& area = pMesh->m_areas[areaIx];

	uint8_t* pVertices;
	uint8_t* pIndices;

	int vertSize = pMesh->m_bytesPerVertex;
	if( FAILED(pMesh->m_vertexBuffer.Lock(0, 0, (void**)&pVertices, LOCK_READONLY, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&]{ pMesh->m_vertexBuffer.Unlock( renderContext ); } );
	if( FAILED(pMesh->m_indexBuffer.Lock(0, 0, (void**)&pIndices, LOCK_READONLY, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&]{ pMesh->m_indexBuffer.Unlock( renderContext ); } );

	uint16_t* pShortIndices = (uint16_t*)pIndices;
	uint32_t* pLongIndices = (uint32_t*)pIndices;
	
	Tr2VertexDefinition decl;
	if ( !Tr2EffectStateManager::GetVertexDeclarationElements( pMesh->m_vertexDeclaration, decl ) )
	{
		return false;
	}

	const Tr2VertexDefinition::Item* const position = decl.Find( decl.POSITION );
	if( !position )
	{
		return false;
	}
	

	unsigned int index1 = 0;
	unsigned int index2 = 0;
	unsigned int index3 = 0;
	Vector3 p1;
	Vector3 p2;
	Vector3 p3;
	unsigned int startIndex = area.m_firstIndex;
	if( pMesh->m_indexBuffer.Is16Bit() )
	{
		index1 = pShortIndices[startIndex];
		index2 = pShortIndices[startIndex+1];
		index3 = pShortIndices[startIndex+2];
	}
	else
	{
		index1 = pLongIndices[startIndex];
		index2 = pLongIndices[startIndex+1];
		index3 = pLongIndices[startIndex+2];
	}

	ConvertDataToVector3( position->m_dataType, pVertices + index1 * vertSize, &p1);
	ConvertDataToVector3( position->m_dataType, pVertices + index2 * vertSize, &p2);
	ConvertDataToVector3( position->m_dataType, pVertices + index3 * vertSize, &p3);

	// Compute edges & get a point on the triangle
	edge1 = p1 - p3;
	edge2 = p2 - p3;
	pointOnTriangle = p1;

	return true;
}

bool TriGeometryRes::GetBoundingSphere( unsigned int meshIx, Vector4& sphere )
{
	if( meshIx >= m_meshes.size() )
	{
		return false;
	}
	TriGeometryResMeshData* pMesh = m_meshes[meshIx];
	if( !pMesh )
	{
		return false;
	}

	sphere = pMesh->m_boundingSphere;

	return true;
}

unsigned int TriGeometryRes::GetAreaCount( unsigned int meshIx )
{
	if( meshIx >= m_meshes.size() || m_meshes[meshIx] == NULL )
	{
		return 0;
	}
	return (unsigned int)m_meshes[meshIx]->m_areas.size();
}

TriGeometryResAreaData* TriGeometryRes::GetAreaData( unsigned int meshIx, unsigned int areaIx )
{
	TriGeometryResMeshData* meshData = GetMeshData( meshIx );
	if( !meshData )
	{
		return NULL;
	}
	if( areaIx < GetAreaCount( meshIx ) )
	{
		return &meshData->m_areas[areaIx];
	}
	else
	{
		return NULL;
	}
}

bool TriGeometryRes::OnPrepareResources()
{
	if( IsPrepared() || IsLoading() )
	{
		return true;
	}

	Initialize( m_path.c_str(), m_ext.c_str() );
	return true;
}

void TriGeometryRes::ReleaseResources( TriStorage s )
{
	if ( s & TRISTORAGE_MANAGEDMEMORY )
	{
		CancelPendingLoad();

		ReleaseResourcesHelper();

		SetPrepared( false );
		SetGood( false );
	}
}

#if TRINITYDEV
void TriGeometryRes::GetDescription( std::string& desc )
{
	desc = "TriGeometryRes: '";
	desc += CW2A( GetPath() );
	desc += "'";
}
#endif

void TriGeometryRes::OnCloseStream()
{
	m_data = NULL;
	m_dataSize = 0;
}

// This gets called on the background loading thread
BlueAsyncRes::LoadingResult TriGeometryRes::DoLoad()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_inMemoryInfo = NULL;

	if( m_pGrannyFile || m_inMemoryInfo )
	{
		// The granny file already exists so we only have to recreate
		// the meshes.
		SetupMeshes( m_inMemoryInfo ? m_inMemoryInfo : GrannyGetFileInfo( m_pGrannyFile ) );
		return LR_SUCCESS;
	}

	if( !m_dataStream )
	{
		return LR_FAILED;
	}

	if( ReadGrannyFile() )
	{
		return LR_SUCCESS;
	}

	return LR_FAILED;
}

// This gets called on the main thread
bool TriGeometryRes::DoPrepare()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	if( Tr2Renderer::IsGeometryLoadDisabled() )
	{
		return false;
	}

	if( m_pGrannyFile || m_inMemoryInfo )
	{
		granny_file_info* gi = m_inMemoryInfo ? m_inMemoryInfo : GrannyGetFileInfo( m_pGrannyFile );

		CreateMeshesFromGrannyFile( gi, renderContext );

		if( gi->AnimationCount == 0 && m_pGrannyFile )
		{
			// Only meshes in the file - we've converted those to D3D structures.
			GrannyFreeFile( m_pGrannyFile );
			m_pGrannyFile = 0;
		}
	}

	if( m_inMemoryInfo )	// we're not called through resman, so mark ourselves as good to go:
	{
		m_isLoading = 0;
		m_isGood = 1;
		m_isPrepared = 1;
	}

	return true;
}

void TriGeometryRes::DetermineAreaBoundsAndVertCount( TriGeometryResAreaData& area, granny_mesh* myMesh, int bytesPerVertex )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Determine bounds for area
	// Assume that position is the first component

	// set up some data for the vertex fetcher.
	unsigned int  positionOffset;
	Tr2VertexDefinition::DataType positionType;
	GetVertexPositionOffsetAndType( myMesh, positionOffset, positionType );

	InitializeBounds( area.m_minBounds, area.m_maxBounds );

	std::set<int> vertexIndicesSeen;

	for( int vIx = 0; vIx < area.m_primitiveCount * 3; ++vIx )
	{
		int index;

		if( myMesh->PrimaryTopology->Indices16 )
		{
			index = myMesh->PrimaryTopology->Indices16[vIx + area.m_firstIndex];
		}
		else
		{
			index = myMesh->PrimaryTopology->Indices[vIx + area.m_firstIndex];
		}

		vertexIndicesSeen.insert( index );

		// vertices are NOT GUARENTEED to be float3 or fp16_4, so this helps with that.
		Vector3 vertexPosition;
		GetMeshVertexPosition(myMesh, index, vertexPosition, (unsigned int) bytesPerVertex, positionOffset, positionType);

		UpdateBounds( area.m_minBounds, area.m_maxBounds, vertexPosition );
	}

	area.m_vertexCount = (int)vertexIndicesSeen.size();
}

void TriGeometryRes::DetermineAreaBones( TriGeometryResAreaData& area, granny_mesh* myMesh, int bytesPerVertex )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// offset to boneindex
	int boneIndexOffset = GetVertexComponentOffset( myMesh, GrannyVertexBoneIndicesName );
	// if there are no bone-indices , we are done
	if( boneIndexOffset == -1 )
		return;

	// collect all bones used by this mesh
	TrackableStdVector<uint8_t> usedBones( "DetermineAreaBones/usedBones", myMesh->BoneBindingCount, 0xff );
	
	// keep track of verts we already re-mapped
	TrackableStdVector<bool> changedVerts( "DetermineAreaBones/changedVerts", myMesh->PrimaryVertexData->VertexCount );
	
	// prepare collection of new per-area-bones
	area.m_jointBindings.clear();

	// pointers to bone indices
	uint8_t* pBoneIndex0 = (uint8_t*)myMesh->PrimaryVertexData->Vertices + boneIndexOffset + 0;
	uint8_t* pBoneIndex1 = (uint8_t*)myMesh->PrimaryVertexData->Vertices + boneIndexOffset + 1;
	uint8_t* pBoneIndex2 = (uint8_t*)myMesh->PrimaryVertexData->Vertices + boneIndexOffset + 2;
	uint8_t* pBoneIndex3 = (uint8_t*)myMesh->PrimaryVertexData->Vertices + boneIndexOffset + 3;

	// cycle thorugh all vertices of this area and collect bone indices
	for( int vIx = 0; vIx < area.m_primitiveCount * 3; ++vIx )
	{
		int index;

		if( myMesh->PrimaryTopology->Indices16 )
		{
			index = myMesh->PrimaryTopology->Indices16[vIx + area.m_firstIndex];
		}
		else
		{
			index = myMesh->PrimaryTopology->Indices[vIx + area.m_firstIndex];
		}

		if( changedVerts[index] )
		{
			continue;
		}

		// bones
		uint8_t boneIndex0 = *(pBoneIndex0 + index * bytesPerVertex);
		uint8_t boneIndex1 = *(pBoneIndex1 + index * bytesPerVertex);
		uint8_t boneIndex2 = *(pBoneIndex2 + index * bytesPerVertex);
		uint8_t boneIndex3 = *(pBoneIndex3 + index * bytesPerVertex);

		// bone0 already used?
		if( usedBones[boneIndex0] == 0xff)
		{
			// new bone!
			usedBones[boneIndex0] = (uint8_t)area.m_jointBindings.size();
			area.m_jointBindings.push_back( boneIndex0 );
		}
		// re-map in vertex-data
		*(pBoneIndex0 + index * bytesPerVertex) = usedBones[boneIndex0];

		// bone1 already used?
		if( usedBones[boneIndex1] == 0xff)
		{
			// new bone!
			usedBones[boneIndex1] = (uint8_t)area.m_jointBindings.size();
			area.m_jointBindings.push_back( boneIndex1 );
		}
		// re-map in vertex-data
		*(pBoneIndex1 + index * bytesPerVertex) = usedBones[boneIndex1];

		// bone2 already used?
		if( usedBones[boneIndex2] == 0xff)
		{
			// new bone!
			usedBones[boneIndex2] = (uint8_t)area.m_jointBindings.size();
			area.m_jointBindings.push_back( boneIndex2 );
		}
		// re-map in vertex-data
		*(pBoneIndex2 + index * bytesPerVertex) = usedBones[boneIndex2];

		// bone3 already used?
		if( usedBones[boneIndex3] == 0xff)
		{
			// new bone!
			usedBones[boneIndex3] = (uint8_t)area.m_jointBindings.size();
			area.m_jointBindings.push_back( boneIndex3 );
		}
		// re-map in vertex-data
		*(pBoneIndex3 + index * bytesPerVertex) = usedBones[boneIndex3];

		// this one is now re-mapped
		changedVerts[index] = true;
	}
}

bool TriGeometryRes::SetupMeshes( granny_file_info* gi )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_meshes.resize( gi->MeshCount );

	for( int meshIx = 0; meshIx < gi->MeshCount; ++meshIx )
	{
		granny_mesh* myMesh = gi->Meshes[meshIx];

        if( myMesh == NULL || !myMesh->PrimaryVertexData )
        {
            CCP_LOGWARN( "Trying to load geometry with no primary vertex data" );
            return false;
        }

		granny_data_type_definition* grannyVertexDecl = myMesh->PrimaryVertexData->VertexType;

		int bytesPerVertex = GrannyGetTotalObjectSize( grannyVertexDecl );

		int vertexCount = myMesh->PrimaryVertexData->VertexCount;

		TriGeometryResMeshData* pMesh = CCP_NEW( "pMesh" ) TriGeometryResMeshData;
		if( pMesh == nullptr )
		{
			CCP_LOGWARN( "Out of memory in TriGeometryRes::SetupMeshes" );
			return false;
		}

		CopyGrannyName( pMesh->m_name, myMesh->Name );
		pMesh->m_bytesPerVertex = bytesPerVertex;
		pMesh->m_vertexCount = vertexCount;
		pMesh->m_primitiveCount = 0;

		for( int jointIx = 0; jointIx < myMesh->BoneBindingCount; ++jointIx )
		{
			pMesh->m_jointBindings.push_back( myMesh->BoneBindings[jointIx].BoneName );
		}


		MeshBoundsInfo* mbi = NULL;

		// Look for mesh bounds info in the Granny file
		if( myMesh->ExtendedData.Object )
		{
			// The mesh has extended data - ask Granny to convert it to our current version of the bounds
			// info data structure.
#ifdef __ANDROID__
			mbi = static_cast<MeshBoundsInfo*>( GrannyConvertTree( myMesh->ExtendedData.Type, myMesh->ExtendedData.Object, MeshBoundsInfoType, nullptr ) );
#else
			mbi = static_cast<MeshBoundsInfo*>( GrannyConvertTree( myMesh->ExtendedData.Type, myMesh->ExtendedData.Object, MeshBoundsInfoType, nullptr, nullptr ) );
#endif
			if( !mbi->typeName || (strcmp( mbi->typeName, "MeshBoundsInfo" ) != 0) )
			{
				// This extended data doesn't match our expectations
				GrannyFreeBuilderResult( (void*)mbi );
				mbi = NULL;
			}
		}

		InitializeBounds( pMesh->m_minBounds, pMesh->m_maxBounds );
		pMesh->m_boundingSphere = Vector4( 0.f, 0.f, 0.f, 0.f );
		 
		if( myMesh->PrimaryTopology )
		{
			pMesh->m_areas.resize( myMesh->PrimaryTopology->GroupCount );

			for( int groupIx = 0; groupIx < myMesh->PrimaryTopology->GroupCount; ++groupIx )
			{
				const granny_tri_material_group& grp = myMesh->PrimaryTopology->Groups[groupIx];

				TriGeometryResAreaData& area = pMesh->m_areas[groupIx];
				area.m_name = "";

				if( myMesh->MaterialBindingCount > grp.MaterialIndex )
				{
					if( myMesh->MaterialBindings[grp.MaterialIndex].Material != NULL )
					{
						const granny_material_binding& mb = myMesh->MaterialBindings[grp.MaterialIndex];
						CopyGrannyName( area.m_name, mb.Material->Name );
					}
				}

				area.m_firstIndex = myMesh->PrimaryTopology->Groups[groupIx].TriFirst * 3;
				area.m_primitiveCount = myMesh->PrimaryTopology->Groups[groupIx].TriCount;

				pMesh->m_primitiveCount += area.m_primitiveCount;

				// only re-map the bone indices if there is a skeleton...
				if( gi->SkeletonCount )
				{
					// no re-map if this a dynamic mesh -> unnecessary! no bone limit...
					if( !m_isDynamicGeometry )
					{
						if( myMesh->BoneBindingCount > TR2_MAX_BONES_PER_MESHAREA )
						{
							DetermineAreaBones( area, myMesh, bytesPerVertex );
							// flag this re-bone-mapping
							pMesh->m_hasPerMeshAreaBoneBindings = true;
						}
					}
				}

				if( mbi )
				{
					CCP_ASSERT( groupIx < mbi->areaCount );
					area.m_minBounds = Vector3( mbi->areaInfos[groupIx].bounds.min );
					area.m_maxBounds = Vector3( mbi->areaInfos[groupIx].bounds.max );
					area.m_vertexCount = mbi->areaInfos[groupIx].vertexCount;
				}
				else
				{
					DetermineAreaBoundsAndVertCount( area, myMesh, bytesPerVertex );
				}

				// if the area doesn't have any verts, the bounding box is invalid, so DON't use it!
				if( area.m_primitiveCount )
				{
					UpdateBounds( pMesh->m_minBounds, pMesh->m_maxBounds, area.m_minBounds );
					UpdateBounds( pMesh->m_minBounds, pMesh->m_maxBounds, area.m_maxBounds );
				}
			}
		}

		if( mbi )
		{
			GrannyFreeBuilderResult( (void*)mbi );
		}

		m_meshes[meshIx] = pMesh;
	}

    return true;
}

void TriGeometryRes::SetupModels( granny_file_info* gi )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_models.resize( gi->ModelCount );

	for(int modelIndex = 0;modelIndex<gi->ModelCount;modelIndex++)
	{
		granny_model* model = gi->Models[modelIndex];

		TriGeometryResModelData* pModel = CCP_NEW( "pModel" ) TriGeometryResModelData;
		if( pModel == nullptr )
		{
			CCP_LOGWARN( "Out of memory in TriGeometryRes::SetupModels" );
			return;
		}
		CopyGrannyName( pModel->m_name, model->Name );

		pModel->m_translation[0] = model->InitialPlacement.Position[0];
		pModel->m_translation[1] = model->InitialPlacement.Position[1];
		pModel->m_translation[2] = model->InitialPlacement.Position[2];

		pModel->m_orientation[0] = model->InitialPlacement.Orientation[0];
		pModel->m_orientation[1] = model->InitialPlacement.Orientation[1];
		pModel->m_orientation[2] = model->InitialPlacement.Orientation[2];
		pModel->m_orientation[3] = model->InitialPlacement.Orientation[3];

		m_models[modelIndex] = pModel ;
	}
}

void TriGeometryRes::SetupSkeletons( granny_file_info* gi )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_skeletons.resize( gi->SkeletonCount );
	for( int skelIx = 0; skelIx < gi->SkeletonCount; ++skelIx )
	{
		granny_skeleton* skel = gi->Skeletons[skelIx];
		TriGeometryResSkeletonData* pSkelData = CCP_NEW( "pSkelData" ) TriGeometryResSkeletonData;
		if( pSkelData == nullptr )
		{
			CCP_LOGWARN( "Out of memory in TriGeometryRes::SetupSkeletons" );
			return;
		}
		CopyGrannyName( pSkelData->m_name, skel->Name );
		pSkelData->m_joints.resize( skel->BoneCount );
		for( int jointIx = 0; jointIx < skel->BoneCount; ++jointIx )
		{
			TriGeometryResJointData& joint = pSkelData->m_joints[jointIx];
			granny_bone& bone = skel->Bones[jointIx];
			CopyGrannyName( joint.m_name, bone.Name );
			joint.m_parentJoint = bone.ParentIndex;
			joint.m_inverseWorldTransform = *(Matrix*)&bone.InverseWorld4x4;
		}

		m_skeletons[skelIx] = pSkelData;
	}
}

void TriGeometryRes::PrepareGrannyFileGeometry( granny_file_info* gi )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( g_geometryResNormalizeOnLoad && gi->ArtToolInfo )
	{
		NormalizeGrannyFile( gi );
	}
}

bool TriGeometryRes::InitializeFromMemory( granny_file_info* gfi )
{
	return ReadGrannyFile( gfi );
}

void TriGeometryRes::DoPrepareFromMemory()
{
	if( m_inMemoryInfo )
	{
		DoPrepare();
		m_inMemoryInfo = NULL;
	}
}

bool TriGeometryRes::ReadGrannyFile( granny_file_info* gi )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !gi )
	{
		void* data;
		if( !m_dataStream->LockData( &data, 0 ) )
		{
			return false;
		}

		m_dataSize = (unsigned int)m_dataStream->GetSize();

		CCP_STATS_ZONE( __FUNCTION__ " reading Granny file" );

		BeTimer t;
	
		m_pGrannyFile = ProtectedGrannyReadEntireFileFromMemory( m_path.c_str(), m_dataSize, data );

		if( !m_pGrannyFile )
		{
			return false;
		}

		const float secs = (float)t.GetSeconds();
		if( secs > g_grannyWarnLoadTime )
		{
			CCP_LOGWARN( "TriGeometryRes - GrannyRead '%S' took %f seconds", GetPath(), secs );
		}

		gi = GrannyGetFileInfo( m_pGrannyFile );
	}
	else
	{
		m_dataSize = 0;
		m_pGrannyFile = NULL;
		m_inMemoryInfo = gi;
	}

    PrepareGrannyFileGeometry( gi );

	// BeResMan->AddGeometryDataRead( m_dataSize );

	// m_pGrannyFile is freed in CreateD3DMeshFromGranny once the D3D
	// resources have been created on the main thread.

	if( !SetupMeshes( gi ) )
    {
        return false;
    }

	SetupModels( gi );
	SetupSkeletons( gi );

	return true;
}

void TriGeometryRes::PrepareFromGrannyRes( TriGrannyRes* g )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	granny_file* f = g->GetGrannyFile();
	if( !f )
	{
		return;
	}
	granny_file_info* gi = GrannyGetFileInfo( f );

	SetupMeshes( gi );
	SetupModels( gi );
	SetupSkeletons( gi );
	CreateMeshesFromGrannyFile( gi, renderContext );

	SetPrepared( true );
	SetGood( true );
}

struct CalcMiniballContext
{
	double **samplePoints;
	int pointCount;
};

void CalcMiniball( void* context, const Vector3& p1, const Vector3& p2, const Vector3& p3 )
{	
	CalcMiniballContext* ctx = static_cast<CalcMiniballContext*>( context );

	// Add points to the bounding object. We'll get duplicates, but this is ok.
	double* point = new double[3];
	point[0] = p1.x;
	point[1] = p1.y;
	point[2] = p1.z;
	ctx->samplePoints[ctx->pointCount++] = point;
	
	point = new double[3];
	point[0] = p2.x;
	point[1] = p2.y;
	point[2] = p2.z;
	ctx->samplePoints[ctx->pointCount++] = point;

	point = new double[3];
	point[0] = p3.x;
	point[1] = p3.y;
	point[2] = p3.z;
	ctx->samplePoints[ctx->pointCount++] = point;
}

void TriGeometryRes::RecalculateBoundingSphere()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	size_t nMeshes = m_meshes.size();
	for ( size_t i = 0; i < nMeshes; i++ )
	{
		if( m_meshes[i] == NULL )
		{
			continue;
		}

		typedef double* const* PointIterator; 
		typedef const double* CoordIterator;
		typedef Miniball::Miniball <Miniball::CoordAccessor<PointIterator, CoordIterator> > MB;

		CalcMiniballContext context;
		// Figure out how many points
		context.samplePoints = new double*[m_meshes[i]->m_primitiveCount*3];		
		context.pointCount = 0;
		ProcessMeshTriangles( (int)i, &CalcMiniball, &context );
		MB mb( 3, context.samplePoints, context.samplePoints+m_meshes[i]->m_primitiveCount*3 );
		
		const double* boundingSphereCenter = mb.center();

		m_meshes[i]->m_boundingSphere.x = float( boundingSphereCenter[0] );
		m_meshes[i]->m_boundingSphere.y = float( boundingSphereCenter[1] );
		m_meshes[i]->m_boundingSphere.z = float( boundingSphereCenter[2] );
		m_meshes[i]->m_boundingSphere.w = float( sqrt( mb.squared_radius() ) );

		for( int j=0; j< context.pointCount; ++j )
		{
			delete[] context.samplePoints[j];
		}
		delete[] context.samplePoints;
	}
}

struct CalcBoundingBoxContext
{
	Vector3 min;
	Vector3 max;
	Matrix transform;
};

void CalcBoundingBox( void* context, const Vector3& p1, const Vector3& p2, const Vector3& p3 )
{
	CalcBoundingBoxContext* ctx = static_cast<CalcBoundingBoxContext*>( context );

	Vector4 v[3];
	v[0] = Vector4( p1.x, p1.y, p1.z, 1.0f );
	v[1] = Vector4( p2.x, p2.y, p2.z, 1.0f );
	v[2] = Vector4( p3.x, p3.y, p3.z, 1.0f );

	for( int i=0; i<3; ++i )
	{
		D3DXVec4Transform( &v[i], &v[i], &ctx->transform );
		v[i] /= v[i].w;

		ctx->max.x = std::max( ctx->max.x, v[i].x );
		ctx->max.y = std::max( ctx->max.y, v[i].y );
		ctx->max.z = std::max( ctx->max.z, v[i].z );

		ctx->min.x = std::min( ctx->min.x, v[i].x );
		ctx->min.y = std::min( ctx->min.y, v[i].y );
		ctx->min.z = std::min( ctx->min.z, v[i].z );
	}
}

static void ConvertTriangleData( Tr2VertexDefinition::DataType elementType, unsigned int vertexByteSize, uint8_t * vertexBase,
								 unsigned int index1, unsigned int index2,unsigned int index3,
								 Vector3 * v1, Vector3 * v2, Vector3 * v3)
{

	ConvertDataToVector3(elementType, vertexBase + index1 * vertexByteSize, v1);
	ConvertDataToVector3(elementType, vertexBase + index2 * vertexByteSize, v2);
	ConvertDataToVector3(elementType, vertexBase + index3 * vertexByteSize, v3);

}

void TriGeometryRes::ProcessMeshTriangles( int meshIx, PerTriangleCallback cb, void* cbContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	int i = meshIx;

	if( m_meshes[i] == NULL || !m_meshes[i]->m_vertexBuffer.IsValid() || !m_meshes[i]->m_indexBuffer.IsValid() )
	{
		return;
	}

	uint8_t* pVertices;
	uint8_t* pIndices;

	int vertSize = m_meshes[i]->m_bytesPerVertex;
	if (FAILED(m_meshes[i]->m_vertexBuffer.Lock(0, 0, (void**)&pVertices, LOCK_READONLY, renderContext )))
	{
		return;
	}
	ON_BLOCK_EXIT( [&]{ m_meshes[i]->m_vertexBuffer.Unlock( renderContext ); } );
	if (FAILED(m_meshes[i]->m_indexBuffer.Lock(0, 0, (void**)&pIndices, LOCK_READONLY, renderContext )))
	{
		return;
	}
	ON_BLOCK_EXIT( [&]{ m_meshes[i]->m_indexBuffer.Unlock( renderContext ); } );

	uint16_t* pShortIndices = (uint16_t*)pIndices;
	uint32_t* pLongIndices = (uint32_t*)pIndices;
	

	Tr2VertexDefinition decl;
	if ( !Tr2EffectStateManager::GetVertexDeclarationElements( m_meshes[i]->m_vertexDeclaration, decl ) )
	{
		return;
	}

	unsigned int positionByteOffset = 0;

	Tr2VertexDefinition::DataType declType;
	for ( size_t j = 0; j < decl.m_items.size(); j++ )
	{
		if (( decl.m_items[j].m_usage == Tr2VertexDefinition::POSITION ) && ( decl.m_items[j].m_usageIndex == 0 ) )
		{
			declType = decl.m_items[j].m_dataType;
			positionByteOffset = decl.m_items[j].m_offset;
			break;
		}
	}

	int numPrim = m_meshes[i]->m_primitiveCount;
	for ( int j = 0; j < numPrim; j++ )
	{
		unsigned int index1 = 0;
		unsigned int index2 = 0;
		unsigned int index3 = 0;
		Vector3 p1;
		Vector3 p2;
		Vector3 p3;
		if (m_meshes[i]->m_indexBuffer.Is16Bit() )
		{
			index1 = pShortIndices[j*3];
			index2 = pShortIndices[(j*3)+1];
			index3 = pShortIndices[(j*3)+2];
		}
		else
		{
			index1 = pLongIndices[j*3];
			index2 = pLongIndices[(j*3)+1];
			index3 = pLongIndices[(j*3)+2];
		}

		ConvertTriangleData(declType, vertSize, pVertices + positionByteOffset , index1, index2, index3, &p1, &p2, &p3);

		(*cb)( cbContext, p1, p2, p3 );
	}
}

void TriGeometryRes::ProcessMeshVertices( int meshIx, PerVertexCallback cb, void* cbContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	int i = meshIx;

	if( m_meshes[i] == NULL || !m_meshes[i]->m_vertexBuffer.IsValid() )
	{
		return;
	}

	uint8_t* pVertices;

	int numVerts = m_meshes[i]->m_vertexCount;
	int vertSize = m_meshes[i]->m_bytesPerVertex;
	if (FAILED(m_meshes[i]->m_vertexBuffer.Lock(0, 0, (void**)&pVertices, LOCK_READONLY, renderContext )))
	{
		return;
	}
	ON_BLOCK_EXIT( [&]{ m_meshes[i]->m_vertexBuffer.Unlock( renderContext ); } );

	Tr2VertexDefinition decl;
	if ( !Tr2EffectStateManager::GetVertexDeclarationElements( m_meshes[i]->m_vertexDeclaration, decl ) )
	{
		return;
	}

	const Tr2VertexDefinition::Item* const position = decl.Find( decl.POSITION );
	const Tr2VertexDefinition::Item* const normal   = decl.Find( decl.NORMAL );
	
	if( !position || !normal )
	{
		return;
	}
	
	for( int j = 0; j < numVerts; j++ )
	{

		Vector3 p1;
		ConvertDataToVector3( position->m_dataType, pVertices +vertSize *j + position->m_offset, &p1 );

		Vector3 n1;
		ConvertDataToVector3( normal->m_dataType, pVertices +vertSize *j + normal->m_offset, &n1 );

		( *cb )( cbContext, p1, n1 );
	}
}

void TriGeometryRes::ProcessMeshVerticesWithUV( int meshIx, PerVertexUVCallback cb, void* cbContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	int i = meshIx;

	if( m_meshes[i] == NULL || !m_meshes[i]->m_vertexBuffer.IsValid() )
	{
		return;
	}

	uint8_t* pVertices;

	USE_MAIN_THREAD_RENDER_CONTEXT();

	int numVerts = m_meshes[i]->m_vertexCount;
	int vertSize = m_meshes[i]->m_bytesPerVertex;
	if (FAILED(m_meshes[i]->m_vertexBuffer.Lock(0, 0, (void**)&pVertices, LOCK_READONLY, renderContext )))
	{
		return;
	}
	ON_BLOCK_EXIT( [&]{ m_meshes[i]->m_vertexBuffer.Unlock( renderContext ); } );

	Tr2VertexDefinition decl;
	if ( !Tr2EffectStateManager::GetVertexDeclarationElements( m_meshes[i]->m_vertexDeclaration, decl ) )
	{
		return;
	}

	const Tr2VertexDefinition::Item* const position = decl.Find( decl.POSITION );
	const Tr2VertexDefinition::Item* const normal   = decl.Find( decl.NORMAL );
	const Tr2VertexDefinition::Item* const texcoord = decl.Find( decl.TEXCOORD );

	if( !position || !normal )
	{
		return;
	}
	
	for ( int j = 0; j < numVerts; j++ )
	{

		Vector3 p1;
		ConvertDataToVector3( position->m_dataType, pVertices +vertSize *j + position->m_offset, &p1 );

		Vector3 n1;
		ConvertDataToVector3( normal->m_dataType, pVertices +vertSize *j + normal->m_offset, &n1 );

		Vector2 uv1( 0.0f, 0.0f );
		if( texcoord )
		{
			if( texcoord->m_dataType == decl.FLOAT16_2 )
			{
				D3DXFloat16To32Array( (float*)&uv1, (const D3DXFLOAT16*)(pVertices + texcoord->m_offset + j * vertSize ), 2 );
			}
			else
			{
				uv1 = *(Vector2*)(pVertices + texcoord->m_offset + j * vertSize);
			}
		}

		(*cb)( cbContext, p1, n1, uv1 );
	}
}

bool TriGeometryRes::GetIntersectionPointAndNormal( const Vector3* pos, const Vector3* dir, Vector3* hitpoint, Vector3* normal )
{
	Vector3 farPoint;
	Vector3 farPointNormal;
	return GetIntersectionPoints( pos, dir, hitpoint, normal, &farPoint, &farPointNormal );
}

std::pair<bool, std::pair<Vector3, Vector3>> TriGeometryRes::GetIntersectionPointAndNormalFromScript( const Vector3& pos, const Vector3& dir )
{
	Vector3 hitpoint( 0.0f, 0.0f, 0.0f );
	Vector3 normal( 0.0f, 0.0f, 0.0f );
	bool result = GetIntersectionPointAndNormal( &pos, &dir, &hitpoint, &normal );
	return std::make_pair( result, std::make_pair( hitpoint, normal ) );
}

bool TriGeometryRes::GetIntersectionPoints( const Vector3* pos, const Vector3*dir, Vector3* hitpointNear, Vector3* hitpointNearNormal, Vector3* hitpointFar, Vector3* hitpointFarNormal )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	float minDist = FLT_MAX;
	float maxDist = FLT_MIN;
	bool result = false;
	size_t nMeshes = m_meshes.size();
	for ( size_t i = 0; i < nMeshes; i++ )
	{
		if( m_meshes[i] == NULL )
		{
			continue;
		}

		uint8_t* pVertices;
		uint8_t* pIndices;

		int vertSize = m_meshes[i]->m_bytesPerVertex;
		if (FAILED(m_meshes[i]->m_vertexBuffer.Lock(0, 0, (void**)&pVertices, LOCK_READONLY, renderContext )))
		{
			return 0;
		}
		ON_BLOCK_EXIT( [&]{ m_meshes[i]->m_vertexBuffer.Unlock( renderContext ); } );
		if (FAILED(m_meshes[i]->m_indexBuffer.Lock(0, 0, (void**)&pIndices, LOCK_READONLY, renderContext )))
		{			
			return false;
		}
		ON_BLOCK_EXIT( [&]{ m_meshes[i]->m_indexBuffer.Unlock( renderContext ); } );

		
		uint16_t* pShortIndices = (uint16_t*)pIndices;
		uint32_t* pLongIndices = (uint32_t*)pIndices;
		
		Tr2VertexDefinition decl;
		if ( !Tr2EffectStateManager::GetVertexDeclarationElements( m_meshes[i]->m_vertexDeclaration, decl ) )
		{
			return false;
		}
		const Tr2VertexDefinition::Item* const position = decl.Find( decl.POSITION );
		if( !position )
		{
			return false;
		}

		int numPrim = m_meshes[i]->m_primitiveCount;
		for ( int j = 0; j < numPrim; j++ )
		{
			unsigned int index1 = 0;
			unsigned int index2 = 0;
			unsigned int index3 = 0;
			Vector3 p1;
			Vector3 p2;
			Vector3 p3;
			float pu, pv, dist;
			if( m_meshes[i]->m_indexBuffer.Is16Bit() )
			{
				index1 = pShortIndices[j*3];
				index2 = pShortIndices[(j*3)+1];
				index3 = pShortIndices[(j*3)+2];
			}
			else
			{
				index1 = pLongIndices[j*3];
				index2 = pLongIndices[(j*3)+1];
				index3 = pLongIndices[(j*3)+2];
			}

			ConvertTriangleData( position->m_dataType, vertSize, pVertices, index1, index2, index3, &p1, &p2, &p3 );

			if ( D3DXIntersectTri(&p1, &p2, &p3, pos, dir, &pu, &pv, &dist ) )
			{
				float v1 = 1.0f - (pu + pv);
				Vector3 avec = p2 - p1;
				Vector3 bvec = p3 - p1;
				if ( minDist > dist )
				{
					*hitpointNear = p1*v1 + p2*pu + p3*pv;
					D3DXVec3Cross(hitpointNearNormal, &avec, &bvec );
					D3DXVec3Normalize(hitpointNearNormal, hitpointNearNormal);
					minDist = dist;
				}
				if ( maxDist < dist )
				{
					*hitpointFar = p1*v1 + p2*pu + p3*pv;
					D3DXVec3Cross(hitpointFarNormal, &avec, &bvec );
					D3DXVec3Normalize(hitpointFarNormal, hitpointFarNormal);
					maxDist = dist;
				}
				result = true;
			}

		}
	}

	if ( minDist == FLT_MAX )
	{
		*hitpointNear = *hitpointFar;
		*hitpointNearNormal = *hitpointFarNormal;
	}

	if ( maxDist == FLT_MIN )
	{
		*hitpointFar = *hitpointNear;
		*hitpointFarNormal = *hitpointNearNormal;
	}

	return result;
}

std::pair<float, Vector3> TriGeometryRes::GetClosestVertex( const Vector3& pos )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	float minDist = FLT_MAX;
	float maxDist = FLT_MIN;
	std::pair<float, Vector3> result = std::make_pair( -1.f, Vector3( 0.f, 0.f, 0.f ) );
	float currentDist = FLT_MAX;
	Vector3 currentPos = Vector3( 0.f, 0.f, 0.f );
	for ( size_t i = 0; i < m_meshes.size(); ++i )
	{
		if( m_meshes[i] == NULL )
		{
			continue;
		}

		uint8_t* pVertices;
		int vertSize = m_meshes[i]->m_bytesPerVertex;
		if (FAILED(m_meshes[i]->m_vertexBuffer.Lock(0, 0, (void**)&pVertices, LOCK_READONLY, renderContext )))
		{
			return result;
		}
		ON_BLOCK_EXIT( [&]{ m_meshes[i]->m_vertexBuffer.Unlock( renderContext ); } );
		
		Tr2VertexDefinition decl;
		if ( !Tr2EffectStateManager::GetVertexDeclarationElements( m_meshes[i]->m_vertexDeclaration, decl ) )
		{
			return result;
		}
		const Tr2VertexDefinition::Item* const position = decl.Find( decl.POSITION );
		if( !position )
		{
			return result;
		}

		for ( unsigned int j = 0; j < m_meshes[i]->m_vertexCount; ++j )
		{
			Vector3 vtx;
			ConvertDataToVector3( position->m_dataType, pVertices + j * vertSize, &vtx );
			Vector3 vec = vtx - pos;
			float d = D3DXVec3LengthSq( &vec );
			if( d < currentDist )
			{
				currentDist = d;
				currentPos = vtx;
			}
		}
	}

	result = std::make_pair( sqrtf( currentDist ), currentPos );
	return result;
}

unsigned int TriGeometryRes::GetSkeletonCount() const
{
	return (unsigned int)m_skeletons.size();
}

TriGeometryResSkeletonData* TriGeometryRes::GetSkeletonData( unsigned int skelIx )
{
	CCP_ASSERT( skelIx < m_skeletons.size() );

	return m_skeletons[skelIx];
}

bool TriGeometryRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t TriGeometryRes::GetMemoryUsage()
{
	return m_memoryUse;
}

void TriGeometryRes::Initialize( const wchar_t* name, const wchar_t* ext )
{
	ReleaseResources( TRISTORAGE_ALL );
	BlueAsyncRes::Initialize( name, ext );
}

void TriGeometryRes::ReleaseResourcesHelper()
{
	for( unsigned int i = 0; i < m_meshes.size(); ++i )
	{
		delete m_meshes[i];
	}
	m_meshes.clear();

	CCP_STATS_ADD( geometryResBytes, -(int)m_memoryUse );
	m_memoryUse = 0;
}

void CalcTriangleSurfaceArea( void* context, const Vector3& p1, const Vector3& p2, const Vector3& p3 )
{
	float* area = (float*)context;

	Vector3 v1 = p2 - p1;
	Vector3 v2 = p3 - p1;

	Vector3 t;
	D3DXVec3Cross( &t, &v1, &v2 );

	*area += 0.5f * D3DXVec3Length( &t );
}

float TriGeometryRes::GetMeshSurfaceArea( int meshIx )
{
	float result = 0.0f;
	ProcessMeshTriangles( meshIx, CalcTriangleSurfaceArea, &result );
	return result;
}

unsigned int TriGeometryResSkeletonData::FindJoint( const char* name )
{
	unsigned int jointCount = (unsigned int)m_joints.size();
	for( unsigned int ix = 0; ix < jointCount; ++ix )
	{
		if( strcmp( name, m_joints[ix].m_name.c_str() ) == 0 )
		{
			return ix;
		}
	}

	return 0xffffffff;
}

void TriGeometryRes::Reload()
{
	CancelPendingLoad();
	NotifyReleaseCachedData();
	ClearGrannyData();
	BlueAsyncRes::Reload();
}

int TriGeometryRes::PushMesh( TriGeometryResMeshData* mesh )
{
	m_meshes.push_back( mesh );
	SetPrepared( true );
	SetGood( true );
	return (int)m_meshes.size() - 1;
}

// -------------------------------------------------------------
// Description:
//   Checks of intersection between a ray and an axis-aligned
//	 bounding box.
// Arguments:
//   rayOrigin - Ray origin.
//   rayDirection - Ray direction.
//   boundsMin - Min bounds of the bounding box.
//   boundsMax - Max bounds of the bounding box.
// Return Value:
//   true If the ray intersects the bounding box
//   false If the ray does not intersect the bounding box
// -------------------------------------------------------------
static bool IntersectRayAxisAlignedBox( const Vector3& rayOrigin, 
										const Vector3& rayDirection, 
										const Vector3& boundsMin, 
										const Vector3& boundsMax ) 
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;

	float denx = 1.0f / rayDirection.x;
	float deny = 1.0f / rayDirection.y;
	float denz = 1.0f / rayDirection.z;

	if( denx  >= 0) 
	{
		tmin = ( boundsMin.x - rayOrigin.x ) * denx;
		tmax = ( boundsMax.x - rayOrigin.x ) * denx;
	}
	else 
	{
		tmin = ( boundsMax.x - rayOrigin.x ) * denx;
		tmax = ( boundsMin.x - rayOrigin.x ) * denx;
	}
	if( deny >= 0) 
	{
 		tymin = ( boundsMin.y - rayOrigin.y ) * deny;
		tymax = ( boundsMax.y - rayOrigin.y ) * deny;
	}
	else 
	{
		tymin = ( boundsMax.y - rayOrigin.y ) * deny;
		tymax = ( boundsMin.y - rayOrigin.y ) * deny;
	}
	if( tmin > tymax || tymin > tmax )
	{
		return false;
	}
	if( tymin > tmin )
	{
		tmin = tymin;
	}
	if( tymax < tmax )
	{
		tmax = tymax;
	}
	if( denz >= 0) 
	{
		tzmin = ( boundsMin.z - rayOrigin.z ) * denz;
		tzmax = ( boundsMax.z - rayOrigin.z ) * denz;
	}
	else 
	{
		tzmin = ( boundsMax.z - rayOrigin.z ) * denz;
		tzmax = ( boundsMin.z - rayOrigin.z ) * denz;
	}
	if( tmin > tzmax || tzmin > tmax )
	{
		return false;
	}
	return true;
}

// -------------------------------------------------------------
// Description:
//   Checks of intersection between a ray and mesh area.
// Arguments:
//   rayOrigin - Ray origin.
//   rayDirection - Ray direction.
//   meshIx - Index of the mesh.
//   areaIx - Index of the area of the mesh
//   resultFlags - Result flags (either return any intersection
//		or the closest to ray origin).
//   culling - Culling mode for trinagles (CCW, CW or NONE).
//   position - (out) point of intersection
//   uv - (out) texture UV coordinate at the point of intersection.
// Return Value:
//   true If the ray intersects geometry area
//   false Otherwise
// -------------------------------------------------------------
bool TriGeometryRes::GetRayAreaIntersection( const Vector3& rayOrigin, 
											 const Vector3& rayDirection, 
											 unsigned int meshIx, 
											 unsigned int areaIx, 
											 TriGeometryCollisionResultFlags resultFlags, 
											 TriGeometryCollisionCullingFlags culling, 
											 Vector3& position, 
											 Vector2& uv )
{
	if( !m_isGood )
	{
		return false;
	}

	if( meshIx >= m_meshes.size() )
	{
		return false;
	}

	TriGeometryResMeshData* pMesh = m_meshes[meshIx];
	if( !pMesh )
	{
		return false;
	}

	if( areaIx >= pMesh->m_areas.size() )
	{
		return false;
	}

	if( !IntersectRayAxisAlignedBox( rayOrigin, rayDirection, pMesh->m_minBounds, pMesh->m_maxBounds ) )
	{
		return false;
	}

	if( !pMesh->BuildCollisionData() )
	{
		return false;
	}

	const TriGeometryResAreaData& area = pMesh->m_areas[areaIx];

	int bestIndex;
	XMVECTOR bestT, bestU, bestV;
	XMVECTOR bestPosition;
	bool found = false;

	unsigned index = area.m_firstIndex;

	static const XMVECTOR epsilon =
	{
		1e-20f, 1e-20f, 1e-20f, 1e-20f
	};
	XMVECTOR zero = XMVectorZero();
	XMVECTOR origin = rayOrigin;
	XMVECTOR direction = XMVector3Normalize( rayDirection );

	for( int i = 0; i < area.m_primitiveCount; ++i )
	{
		XMVECTOR v0 = pMesh->m_collisionData.m_positions[pMesh->m_collisionData.m_indexes[index++]];
		XMVECTOR v1 = pMesh->m_collisionData.m_positions[pMesh->m_collisionData.m_indexes[index++]];
		XMVECTOR v2 = pMesh->m_collisionData.m_positions[pMesh->m_collisionData.m_indexes[index++]];

		XMVECTOR e1 = v1 - v0;
		XMVECTOR e2 = v2 - v0;

		// p = Direction ^ e2;
		XMVECTOR p = XMVector3Cross( direction, e2 );

		// det = e1 * p;
		XMVECTOR det = XMVector3Dot( e1, p );

		XMVECTOR u, v, t;

		if( XMVector3GreaterOrEqual( det, epsilon ) && culling != COLLISION_CULL_CW )
		{
			// Determinate is positive (front side of the triangle).
			XMVECTOR s = XMVectorSubtract( origin, v0 );

			// u = s * p;
			u = XMVector3Dot( s, p );

			XMVECTOR noIntersection = XMVectorLess( u, zero );
			noIntersection = XMVectorOrInt( noIntersection, XMVectorGreater( u, det ) );

			// q = s ^ e1;
			XMVECTOR q = XMVector3Cross( s, e1 );

			// v = Direction * q;
			v = XMVector3Dot( direction, q );

			noIntersection = XMVectorOrInt( noIntersection, XMVectorLess( v, zero ) );
			noIntersection = XMVectorOrInt( noIntersection, XMVectorGreater( u + v, det ) );

			// t = e2 * q;
			t = XMVector3Dot( e2, q );

			noIntersection = XMVectorOrInt( noIntersection, XMVectorLess( t, zero ) );

			if( XMVector4EqualInt( noIntersection, XMVectorTrueInt() ) )
			{
				continue;
			}
		}
		else if( XMVector3LessOrEqual( det, -epsilon ) && culling != COLLISION_CULL_CCW )
		{
			// Determinate is negative (back side of the triangle).
			XMVECTOR s = XMVectorSubtract( origin, v0 );

			// u = s * p;
			u = XMVector3Dot( s, p );

			XMVECTOR noIntersection = XMVectorGreater( u, zero );
			noIntersection = XMVectorOrInt( noIntersection, XMVectorLess( u, det ) );

			// q = s ^ e1;
			XMVECTOR q = XMVector3Cross( s, e1 );

			// v = Direction * q;
			v = XMVector3Dot( direction, q );

			noIntersection = XMVectorOrInt( noIntersection, XMVectorGreater( v, zero ) );
			noIntersection = XMVectorOrInt( noIntersection, XMVectorLess( u + v, det ) );

			// t = e2 * q;
			t = XMVector3Dot( e2, q );

			noIntersection = XMVectorOrInt( noIntersection, XMVectorGreater( t, zero ) );

			if ( XMVector4EqualInt( noIntersection, XMVectorTrueInt() ) )
			{
				continue;
			}
		}
		else
		{
			continue;
		}

		if( !found || XMVector3Less( bestT, t ) )
		{
			XMVECTOR inv_det = XMVectorReciprocal( det );
			bestIndex = i;
			bestT = XMVectorMultiply( t, inv_det );
			bestU = XMVectorMultiply( u, inv_det );
			bestV = XMVectorMultiply( v, inv_det );
			bestPosition = XMVectorMultiplyAdd( direction, bestT, origin );
			found = true;

			if( resultFlags == COLLISION_RESULT_CLOSEST )
			{
				break;
			}
		}
	}
	if( found )
	{
		const Vector2& v0 = pMesh->m_collisionData.m_texCoords[pMesh->m_collisionData.m_indexes[area.m_firstIndex + bestIndex * 3]];
		const Vector2& v1 = pMesh->m_collisionData.m_texCoords[pMesh->m_collisionData.m_indexes[area.m_firstIndex + bestIndex * 3 + 1]];
		const Vector2& v2 = pMesh->m_collisionData.m_texCoords[pMesh->m_collisionData.m_indexes[area.m_firstIndex + bestIndex * 3 + 2]];

		Vector2 u = v1 - v0;
		Vector2 v = v2 - v0;
		uv = v0 + u * XMVectorGetX( bestU ) + v * XMVectorGetX( bestV );

		position = bestPosition;
	}
	return found;
}


std::pair<bool, std::pair<Vector3, Vector2>> TriGeometryRes::GetRayAreaIntersectionFromScript( const Vector3& rayOrigin, const Vector3& rayDirection, unsigned int meshIx, unsigned int areaIx, TriGeometryCollisionResultFlags resultFlags, TriGeometryCollisionCullingFlags culling )
{
	Vector3 position;
	Vector2 uv;
	bool found = GetRayAreaIntersection( rayOrigin, rayDirection, meshIx, areaIx, resultFlags, culling, position, uv );
	
	return std::make_pair( found, std::make_pair( position, uv ) );
}

TriGeometryResSkeletonData::TriGeometryResSkeletonData() :
	m_joints( "TriGeometryResSkeletonData/m_joints" )
{
}

TriGeometryResAreaData::TriGeometryResAreaData() :
	m_firstIndex( 0 ),
	m_primitiveCount( 0 ),
	m_vertexCount( 0 ),
	m_jointBindings( "TriGeometryResAreaData/m_jointBindings" )
{
}

TriGeometryResMeshData::TriGeometryResMeshData() :
	m_bytesPerVertex( 0 ),
	m_vertexCount( 0 ),
	m_primitiveCount( 0 ),
	m_areas( "TriGeometryResMeshData/m_areas" ),
	m_jointBindings( "TriGeometryResMeshData/m_jointBindings" ),
	m_hasPerMeshAreaBoneBindings( false ),
	m_pVertexData( NULL ),
	m_vertexDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
{
	m_collisionData.m_indexes = NULL;
	m_collisionData.m_texCoords = NULL;
	m_collisionData.m_positions = NULL;
}

TriGeometryResMeshData::~TriGeometryResMeshData()
{
	delete[] m_collisionData.m_indexes;
	delete[] m_collisionData.m_texCoords;
	if( m_collisionData.m_positions )
	{
		CCP_ALIGNED_FREE( m_collisionData.m_positions );
	}
}

// -------------------------------------------------------------
// Description:
//   Builds collision-specific arrays with vertex positions, 
//	 UV coordinates and vertex indexes.
// Return Value:
//   true If collision arrays were constructed and are ready
//		to be used
//   false Otherwise
// -------------------------------------------------------------
bool TriGeometryResMeshData::BuildCollisionData()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( m_collisionData.m_positions == NULL || m_collisionData.m_texCoords == NULL )
	{
		if( !m_vertexBuffer.IsValid() )
		{
			return false;
		}

		Tr2VertexDefinition decl;
		if( !Tr2EffectStateManager::GetVertexDeclarationElements( m_vertexDeclaration, decl ) )
		{
			return false;
		}

		auto position = decl.Find( decl.POSITION, 0 );
		auto texture  = decl.Find( decl.TEXCOORD, 0 );

		if( !position || !texture )
		{
			return false;
		}
		
		char* data;
		CR_RETURN_VAL( m_vertexBuffer.Lock( 0, 0, reinterpret_cast<void**>( &data ), LOCK_READONLY, renderContext ), false );
		ON_BLOCK_EXIT( [&]{ m_vertexBuffer.Unlock( renderContext ); } );

		if( m_collisionData.m_positions == NULL )
		{
			m_collisionData.m_positions = reinterpret_cast<XMVECTOR*>( CCP_ALIGNED_MALLOC( "m_collisionData.m_positions", sizeof( XMVECTOR ) * m_vertexCount, 16 ) );
			for( unsigned int i = 0; i < m_vertexCount; ++i )
			{
				Vector3* pos3 = reinterpret_cast<Vector3*>( data + position->m_offset + i * m_bytesPerVertex );
				m_collisionData.m_positions[i] = *pos3;
			}
		}
		if( m_collisionData.m_texCoords == NULL )
		{
			m_collisionData.m_texCoords = CCP_NEW( "TriGeometryCollisionData::m_texCoords" ) Vector2[m_vertexCount];
			if( m_collisionData.m_texCoords == nullptr )
			{
				CCP_LOGWARN( "Out of memory in TriGeometryRes::BuildCollisionData" );
				return false;
			}
			for( unsigned int i = 0; i < m_vertexCount; ++i )
			{
				Vector2* uv = reinterpret_cast<Vector2*>( data + texture->m_offset + i * m_bytesPerVertex );
				m_collisionData.m_texCoords[i] = *uv;
			}
		}
	}
	if( m_collisionData.m_indexes == NULL )
	{
		if( !m_indexBuffer.IsValid() )
		{
			return false;
		}

		char* data;
		CR_RETURN_VAL( m_indexBuffer.Lock( 0, 0, reinterpret_cast<void**>( &data ), LOCK_READONLY, renderContext ), false );
		ON_BLOCK_EXIT( [&]{ m_indexBuffer.Unlock( renderContext ); } );

		m_collisionData.m_indexes = CCP_NEW( "TriGeometryCollisionData::m_indexes" ) unsigned[m_primitiveCount * 3];
		if( m_collisionData.m_indexes == nullptr )
		{
			CCP_LOGWARN( "Out of memory in TriGeometryRes::BuildCollisionData" );
			return false;
		}

		if( m_indexBuffer.Is16Bit() )
		{
			unsigned short* indexes = reinterpret_cast<unsigned short*>( data );
			for( unsigned int i = 0; i < m_primitiveCount * 3; ++i )
			{
				m_collisionData.m_indexes[i] = *indexes++;
			}
		}
		else
		{
			memcpy( m_collisionData.m_indexes, data, sizeof( unsigned ) * m_primitiveCount * 3 );
		}
	}
	return true;
}

// -------------------------------------------------------------
// Description:
//   Reverses the original mesh index buffer. This is needed for
//   hair rendering to render hair meshes back-side in "backwards"
//   order.
// Arguments:
//   meshIx - Index of the mesh that needs a reversed index buffer
// -------------------------------------------------------------
void TriGeometryRes::ReverseIndexBuffer( unsigned int meshIx, Tr2RenderContext& renderContext )
{
	if( meshIx >= GetMeshCount() )
	{
		return;
	}
	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return;
	}

	TriGeometryResMeshData &meshData = *GetMeshData( meshIx );
	if( !meshData.m_indexBuffer.IsValid() )
	{
		return;
	}
	if( meshData.m_reversedIndexBuffer.IsValid() )	// already done?
	{
		return;
	}

	Tr2IndexBufferAL&	source = meshData.m_indexBuffer;
	Tr2IndexBufferAL	reverseIB;

	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN( reverseIB.Create( source.GetNumIndices(), source.m_usage, source.GetIBBitcount(), nullptr, renderContext ) );
	}
	if( meshData.m_indexBuffer.Is16Bit() )
	{
		unsigned short *originalData = nullptr;
		CR_RETURN( meshData.m_indexBuffer.Lock( 0, 0, originalData, LOCK_READONLY, renderContext ) );
		ON_BLOCK_EXIT( [&]{ meshData.m_indexBuffer.Unlock( renderContext ); } );

		unsigned short *invertedData = nullptr;
		CR_RETURN( reverseIB.Lock( 0, 0, invertedData, LOCK_WRITEONLY, renderContext ) );
		ON_BLOCK_EXIT( [&]{ reverseIB.Unlock( renderContext ); } );

		unsigned length = meshData.m_primitiveCount * 3;
		for( unsigned int i = 0; i < length; ++i )
		{
			invertedData[length - i - 1] = originalData[i];
		}
	}
	else
	{
		unsigned *originalData = NULL;
		CR_RETURN( meshData.m_indexBuffer.Lock( 0, 0, originalData, LOCK_READONLY, renderContext ) );
		ON_BLOCK_EXIT( [&]{ meshData.m_indexBuffer.Unlock( renderContext ); } );

		unsigned *invertedData = NULL;
		CR_RETURN( reverseIB.Lock( 0, 0, invertedData, LOCK_WRITEONLY, renderContext ) );
		ON_BLOCK_EXIT( [&]{ reverseIB.Unlock( renderContext ); } );

		unsigned length = meshData.m_primitiveCount * 3;
		for( unsigned int i = 0; i < length; ++i )
		{
			invertedData[length - i - 1] = originalData[i];
		}
	}

	meshData.m_reversedIndexBuffer = std::move( reverseIB );
}

bool TriGeometryRes::RenderArea( unsigned int meshIx, unsigned int areaIx, Tr2RenderContext& renderContext, bool reversed )
{
	if( !m_isGood )
	{
		return false;
	}

	if( meshIx >= m_meshes.size() )
	{
		return false;
	}

	TriGeometryResMeshData* pMesh = m_meshes[meshIx];
	if( !pMesh )
	{
		return false;
	}

	if( areaIx >= pMesh->m_areas.size() )
	{
		return false;
	}

	renderContext.m_esm.ApplyVertexDeclaration( pMesh->m_vertexDeclaration );
	renderContext.m_esm.ApplyStreamSource( 0, pMesh->m_vertexBuffer, 0, pMesh->m_bytesPerVertex );
	if( reversed )
	{
		ReverseIndexBuffer( meshIx, renderContext );
		renderContext.m_esm.ApplyIndexBuffer( pMesh->m_reversedIndexBuffer );
	}
	else
	{
		renderContext.m_esm.ApplyIndexBuffer( pMesh->m_indexBuffer );
	}

	const TriGeometryResAreaData& area = pMesh->m_areas[areaIx];

	renderContext.SetTopology( TOP_TRIANGLES );
	if( reversed )
	{
		renderContext.DrawIndexedPrimitive( pMesh->m_vertexCount, pMesh->m_primitiveCount * 3 - area.m_firstIndex - area.m_primitiveCount * 3, area.m_primitiveCount );
	}
	else
	{
		renderContext.DrawIndexedPrimitive( pMesh->m_vertexCount, area.m_firstIndex, area.m_primitiveCount );
	}


	return true;
}



bool TriGeometryRes::RenderAreas( unsigned int meshIx, unsigned int areaIx, unsigned int areaCount, Tr2RenderContext & renderContext, bool reversed )
{
    if( !m_isGood )
    {
        return false;
    }

    if( meshIx >= m_meshes.size() )
    {
        return false;
    }

    TriGeometryResMeshData* pMesh = m_meshes[meshIx];
    if( !pMesh )
    {
        return false;
    }

    if( areaIx >= pMesh->m_areas.size() )
    {
        return false;
    }

    if( areaIx + areaCount > pMesh->m_areas.size() )
    {
		areaCount = (unsigned int)pMesh->m_areas.size() - areaIx;
    }

    const TriGeometryResAreaData& area = pMesh->m_areas[areaIx];

    unsigned int primCount = area.m_primitiveCount;
    for( unsigned int i = 1; i < areaCount; ++i )
    {
        const TriGeometryResAreaData& curArea = pMesh->m_areas[areaIx + i];
        primCount += curArea.m_primitiveCount;
    }

	if( primCount )
	{
		renderContext.m_esm.ApplyVertexDeclaration( pMesh->m_vertexDeclaration );
		renderContext.m_esm.ApplyStreamSource( 0, pMesh->m_vertexBuffer, 0, pMesh->m_bytesPerVertex );
		if( reversed )
		{
			ReverseIndexBuffer( meshIx, renderContext );
			renderContext.m_esm.ApplyIndexBuffer( pMesh->m_reversedIndexBuffer );
		}
		else
		{
			renderContext.m_esm.ApplyIndexBuffer( pMesh->m_indexBuffer );
		}

		renderContext.SetTopology( TOP_TRIANGLES );
		if( reversed )
		{
			renderContext.DrawIndexedPrimitive( pMesh->m_vertexCount, pMesh->m_primitiveCount * 3 - area.m_firstIndex - primCount * 3, primCount );
		}
		else
		{
			renderContext.DrawIndexedPrimitive( pMesh->m_vertexCount, area.m_firstIndex, primCount );
		}
	}

    return true;
}

//--------------------------------------------------------------------------------------------------
bool TriGeometryRes::RenderAreasFromDynamicVertexBuffer( Tr2VertexBufferAL& vertexBuffer, unsigned int meshIx, unsigned int areaIx, unsigned int areaCount, bool reversed )
{
	// Note that we're not actually using the passed in vertexBuffer.. not sure what's up with that, but it's been that way since
	// at least 2010 :|

	USE_MAIN_THREAD_RENDER_CONTEXT();


	if( !m_isGood )
	{
		return false;
	}

	if( meshIx >= m_meshes.size() )
	{
		return false;
	}

	TriGeometryResMeshData* pMesh = m_meshes[meshIx];
	if( !pMesh )
	{
		return false;
	}

	if( areaIx >= pMesh->m_areas.size() )
	{
		return false;
	}

	if( areaIx + areaCount > pMesh->m_areas.size() )
	{
		areaCount = (unsigned int)pMesh->m_areas.size() - areaIx;
	}

	const TriGeometryResAreaData& area = pMesh->m_areas[areaIx];

	unsigned int primCount = area.m_primitiveCount;
	for( unsigned int i = 1; i < areaCount; ++i )
	{
		const TriGeometryResAreaData& curArea = pMesh->m_areas[areaIx + i];
		primCount += curArea.m_primitiveCount;
	}

	if( primCount )
	{
		renderContext.m_esm.ApplyVertexDeclaration( pMesh->m_vertexDeclaration );
		renderContext.m_esm.ApplyStreamSource( 0, pMesh->m_vertexBuffer, 0, pMesh->m_bytesPerVertex );
		if( reversed )
		{
			ReverseIndexBuffer( meshIx, renderContext );
			renderContext.m_esm.ApplyIndexBuffer( pMesh->m_reversedIndexBuffer );
		}
		else
		{
			renderContext.m_esm.ApplyIndexBuffer( pMesh->m_indexBuffer );
		}

		
		renderContext.SetTopology( TOP_TRIANGLES );
		if( reversed )
		{
			renderContext.DrawIndexedPrimitive( pMesh->m_vertexCount, pMesh->m_primitiveCount * 3 - area.m_firstIndex - primCount * 3, primCount );
		}
		else
		{
			renderContext.DrawIndexedPrimitive( pMesh->m_vertexCount, area.m_firstIndex, primCount );
		}
	}

	return true;
}

bool TriGeometryRes::RenderAsOneArea( unsigned int meshIx )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_isGood )
	{
		return false;
	}

	if( meshIx >= m_meshes.size() )
	{
		return false;
	}

	TriGeometryResMeshData* pMesh = m_meshes[meshIx];
	if( !pMesh )
	{
		return false;
	}

	renderContext.m_esm.ApplyVertexDeclaration( pMesh->m_vertexDeclaration );
	renderContext.m_esm.ApplyStreamSource( 0, pMesh->m_vertexBuffer, 0, pMesh->m_bytesPerVertex );
	renderContext.m_esm.ApplyIndexBuffer( pMesh->m_indexBuffer );

	
	renderContext.SetTopology( TOP_TRIANGLES );
	renderContext.DrawIndexedPrimitive( pMesh->m_vertexCount, 0, pMesh->m_primitiveCount );


	return true;
}



static bool ForceFullFloat( granny_data_type_definition* grannyVertexDecl, granny_data_type_definition* fullFloatVertexDecl, int vertexDeclSize )
{
	int componentIx = 0;
	do {
		granny_data_type_definition& src = grannyVertexDecl[componentIx];
		granny_data_type_definition& dst = fullFloatVertexDecl[componentIx];

		dst = src;
		if( dst.Type == GrannyReal16Member )
		{
			dst.Type = GrannyReal32Member;
		}
		if( componentIx + 1 == vertexDeclSize )
		{
			return false;
		}
	}	
	while( grannyVertexDecl[componentIx++].Type != GrannyEndMember );

	return true;
}

bool TriGeometryRes::CreateMeshFromGrannyMesh( granny_mesh* myMesh, TriGeometryResMeshData* pMesh, Tr2PrimaryRenderContext& renderContext, void* pVBOverride )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !Tr2Renderer::IsResourceCreationAllowed() || myMesh == NULL )
	{
		return false;
	}

	const int kVertexComponentMaxCount = 13;
	
	granny_data_type_definition* grannyVertexDecl = myMesh->PrimaryVertexData->VertexType;
	granny_data_type_definition fullFloatVertexDecl[kVertexComponentMaxCount];

	bool forceFullFloat = false;
	if( !renderContext.GetCaps().SupportsFloat16() )
	{
		// Device does not support half-precision floats
		forceFullFloat = true;
	}

	if( forceFullFloat )
	{
		// If needed, change any references to half precision float to full precision floats.
		// Doing this here, on the granny vertex declaration before anything else happens,
		// ensures the D3D vertex buffer is created in the proper format and size.
		// Granny also has a function to copy vertex data with conversions as needed.
		// We pass in this (converted) vertex format - it gives us back the vertex data
		// in the format it prescribes.
		if( !ForceFullFloat( grannyVertexDecl, fullFloatVertexDecl, kVertexComponentMaxCount ) )
		{
			return false;
		}
		grannyVertexDecl = fullFloatVertexDecl;
	}

	Tr2VertexDefinition vertexDefinition = BuildFromGrannyVertexDecl( grannyVertexDecl );
	const unsigned bytesPerVertex = vertexDefinition.m_nextOffset[0];

	unsigned bytesPerIndex = 2;

	int vertexCount = myMesh->PrimaryVertexData->VertexCount;
	int vbSize = vertexCount * bytesPerVertex;

	void* pSrc = pVBOverride;
	if( !pSrc )
	{
		pSrc = GrannyGetMeshVertices( myMesh );
	}

	if( myMesh->Name && strncmp( myMesh->Name, "gpuraw_", 7 ) == 0 )
	{
		BufferUsage usage;
		if( m_immutable )
		{
			// We can't create an immutable structured buffer that is a shader resource, so
			// we use read-only flag instead
			usage = USAGE_CPU_READ;
		}
		else
		{
			usage = USAGE_CPU_READ | USAGE_CPU_WRITE;
		}
		if( forceFullFloat )
		{
			TrackableStdVector<char> tempBuffer( "TriGeometryRes/tempBuffer", vertexCount * bytesPerVertex );
			granny_data_type_definition* pSrcFmt = GrannyGetMeshVertexType( myMesh );
			GrannyConvertVertexLayouts( vertexCount, pSrcFmt, pSrc, grannyVertexDecl, &tempBuffer[0] );
			USE_MAIN_THREAD_RENDER_CONTEXT();
			CR_RETURN_VAL( pMesh->m_shaderResourceBuffer.CreateStructured( vertexCount, bytesPerVertex, usage, 0, &tempBuffer[0], renderContext ), false );
		}
		else
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			CR_RETURN_VAL( pMesh->m_shaderResourceBuffer.CreateStructured( vertexCount, bytesPerVertex, usage, 0, pSrc, renderContext ), false );
		}

		pMesh->m_bytesPerVertex = bytesPerVertex;
		pMesh->m_vertexCount = vertexCount;

		pMesh->m_vertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( vertexDefinition );

		m_memoryUse += vbSize; // Memory use is only approximate as a  hint for the resource cache
	
		return true;
	}

	int indexCount = myMesh->PrimaryTopology->Index16Count;
	if( indexCount == 0 )
	{
		indexCount = myMesh->PrimaryTopology->IndexCount;

		if( indexCount == 0 )
		{
			return false;
		}

		// Granny file has 32 bit indices - make sure we really need that,
		// otherwise they're converted by Granny in GrannyCopyMeshIndices.

		if( g_geometryResForce32bitIndex || vertexCount > 65535 )
		{
			bytesPerIndex = 4;
		}
	}

	

	// here we distinct the mesh creation between dynamic and static:
	// for static geometry we can create a d3d buffer right here, it ca be shared
	// for dynamic geometry we need a shared buffer in system-memory, which we read from during cpu skinning
	if( m_isDynamicGeometry )
	{
		if( !CreateSystemVertexBuffer( pMesh, vertexCount, bytesPerVertex, myMesh, pSrc, grannyVertexDecl, forceFullFloat ) )
		{
			return false;
		}
	}
	else
	{
		if( !CreateD3DVertexBuffer( pMesh, vertexCount, bytesPerVertex, myMesh, pSrc, grannyVertexDecl, forceFullFloat, renderContext ) )
		{
			return false;
		}
	}


	// create d3d index buffer, this one is shared, either for dynamic or static geometry
	Tr2IndexBufferAL d3dIB;
	int ibSize = indexCount * bytesPerIndex;

	if( m_immutable )
	{
		CCP_ASSERT( indexCount > 0 );

		std::vector<char> tempBuffer( indexCount * bytesPerIndex );

		GrannyCopyMeshIndices ( myMesh, bytesPerIndex, &tempBuffer[0] );
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN_VAL( d3dIB.Create( indexCount, USAGE_CPU_READ | USAGE_IMMUTABLE, bytesPerIndex == 2 ? IB_16BIT : IB_32BIT, &tempBuffer[0], renderContext ), false );
	}
	else
	{
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			HRESULT hr = d3dIB.Create( indexCount, USAGE_CPU_READ | USAGE_CPU_WRITE | USAGE_HINT_MANAGED, bytesPerIndex == 2 ? IB_16BIT : IB_32BIT, nullptr, renderContext );
			if( FAILED( hr ) )
			{
				pMesh->m_vertexBuffer.Destroy();
				return false;	
			}
		}

		void* dst;
		HRESULT hr = d3dIB.Lock( 0, ibSize, &dst, LOCK_WRITEONLY, renderContext );
		if( FAILED( hr ) )
		{
			pMesh->m_vertexBuffer.Destroy();
			return false;	
		}

		// Note that this function converts 32bit indices to 16bit if needed
		GrannyCopyMeshIndices ( myMesh, bytesPerIndex, dst );

		d3dIB.Unlock( renderContext );
	}


	pMesh->m_bytesPerVertex = bytesPerVertex;
	pMesh->m_vertexCount = vertexCount;
	pMesh->m_indexBuffer = std::move( d3dIB );

	pMesh->m_vertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( vertexDefinition );

	m_memoryUse += vbSize + ibSize; // Memory use is only approximate as a  hint for the resource cache
	
	return true;
}

bool TriGeometryRes::CreateMeshesFromGrannyFile( granny_file_info* gi, Tr2PrimaryRenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( int meshIx = 0; meshIx < gi->MeshCount; ++meshIx )
	{
		granny_mesh* myMesh = gi->Meshes[meshIx];
		TriGeometryResMeshData* pMesh = m_meshes[meshIx];

		CreateMeshFromGrannyMesh( myMesh, pMesh, renderContext );
	}
	CCP_STATS_ADD( geometryResBytes, m_memoryUse );

	return true;
}

// ----------------------------------------------------------------------------------------------------------------------------
bool TriGeometryRes::CreateD3DVertexBuffer(
	TriGeometryResMeshData* pMesh,
	int vtxCount,
	int bytesPerVtx,
	const granny_mesh* mesh,
	const void* pSrc,
	const granny_data_type_definition* grnVtxDecl,
	bool fullFloat, 
	Tr2PrimaryRenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( pMesh == NULL )
	{
		return false;
	}

	if( m_immutable || m_computeAccess )
	{
		std::vector<char> tempBuffer;
		const void* vertices = pSrc;
		if( fullFloat )
		{
			tempBuffer.resize( vtxCount * bytesPerVtx );
			vertices = &tempBuffer[0];

			granny_data_type_definition* pSrcFmt = GrannyGetMeshVertexType( mesh );
			GrannyConvertVertexLayouts( vtxCount, pSrcFmt, pSrc, grnVtxDecl, &tempBuffer[0] );
		}
		
		CR_RETURN_VAL( 
			pMesh->m_vertexBuffer.Create( 
							vtxCount * bytesPerVtx, 
							( m_immutable ? USAGE_CPU_READ | USAGE_IMMUTABLE : USAGE_CPU_READ ) | ( m_computeAccess ? USAGE_UNORDERED_ACCESS : 0 ),
							vertices, 
							renderContext )
			, false );

		return true;
	}


	Tr2VertexBufferAL &vb = pMesh->m_vertexBuffer;
	CR_RETURN_VAL( vb.Create( vtxCount * bytesPerVtx, USAGE_CPU_READ | USAGE_CPU_WRITE | USAGE_HINT_MANAGED, nullptr, renderContext ), false );

	void* dst;
	CR_RETURN_VAL( vb.Lock( 0, vtxCount * bytesPerVtx, &dst, LOCK_WRITEONLY, renderContext ), false );
	
	// The version of Granny we're using is very slow in copying vertices
	// Only use it if we need any conversion to take place - otherwise
	// do a straight memcpy
	if( fullFloat )
	{
		granny_data_type_definition* pSrcFmt = GrannyGetMeshVertexType( mesh );
		GrannyConvertVertexLayouts( vtxCount, pSrcFmt, pSrc, grnVtxDecl, dst );
	}
	else
	{
		memcpy( dst, pSrc, vtxCount * bytesPerVtx );
	}

	vb.Unlock( renderContext );

	return true;
}

// ----------------------------------------------------------------------------------------------------------------------------
bool TriGeometryRes::CreateSystemVertexBuffer(
	TriGeometryResMeshData* pMesh,
	int vtxCount,
	int bytesPerVtx,
	const granny_mesh* mesh,
	const void* pSrc,
	const granny_data_type_definition* grnVtxDecl,
	bool fullFloat )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( pMesh == NULL )
	{
		return false;
	}

	// alloc special data for shared system-memory vertexlayoutinfo and the vertexbuffer itself
	pMesh->m_pVertexData = static_cast<TriGeometryResVertexData*>( CCP_MALLOC( "TriGeometryRes/pMesh/m_pVertexData", sizeof( TriGeometryResVertexData ) ) );
	if( !pMesh->m_pVertexData )
	{
		return false;
	}
	TriGeometryResVertexData* pVtxData = pMesh->m_pVertexData;
	memset( pVtxData, 0, sizeof( TriGeometryResVertexData ) );
	// alloc buffer for the vertices itself
	pVtxData->m_pBuffer = (granny_uint8*)CCP_MALLOC( "TriGeometryRes/pMesh/m_pVertexData/m_pBuffer", vtxCount * bytesPerVtx );
	if( !pVtxData->m_pBuffer )
	{
		CCP_FREE( pMesh->m_pVertexData );
		return false;
	}

	// The version of Granny we're using is very slow in copying vertices
	// Only use it if we need any conversion to take place - otherwise
	// do a straight memcpy
	if( fullFloat )
	{
		granny_data_type_definition* pSrcFmt = GrannyGetMeshVertexType( mesh );
		GrannyConvertVertexLayouts( vtxCount, pSrcFmt, pSrc, grnVtxDecl, pVtxData->m_pBuffer );
	}
	else
	{
		memcpy( pVtxData->m_pBuffer, pSrc, vtxCount * bytesPerVtx );
	}

	// pre-calc some pointers into the vertex for fast access later during cpu skinning
	unsigned int boneIndexOffset = GetVertexComponentOffset( mesh, GrannyVertexBoneIndicesName );
	if( boneIndexOffset != -1 )
	{
		pVtxData->m_pBoneIndex0 = pVtxData->m_pBuffer + boneIndexOffset + 0;
		pVtxData->m_pBoneIndex1 = pVtxData->m_pBuffer + boneIndexOffset + 1;
		pVtxData->m_pBoneIndex2 = pVtxData->m_pBuffer + boneIndexOffset + 2;
		pVtxData->m_pBoneIndex3 = pVtxData->m_pBuffer + boneIndexOffset + 3;
	}
	unsigned int boneWeightOffset = GetVertexComponentOffset( mesh, GrannyVertexBoneWeightsName );
	if( boneWeightOffset != -1 )
	{
		pVtxData->m_pBoneWeight0 = pVtxData->m_pBuffer + boneWeightOffset + 0;
		pVtxData->m_pBoneWeight1 = pVtxData->m_pBuffer + boneWeightOffset + 1;
		pVtxData->m_pBoneWeight2 = pVtxData->m_pBuffer + boneWeightOffset + 2;
		pVtxData->m_pBoneWeight3 = pVtxData->m_pBuffer + boneWeightOffset + 3;
	}
	unsigned int positionOffset = GetVertexComponentOffset( mesh, GrannyVertexPositionName );
	if( positionOffset != -1 )
	{
		pVtxData->m_pSrcPosition = pVtxData->m_pBuffer + positionOffset;
		pVtxData->m_dstPositionOffset = positionOffset;
	}
	unsigned int normalOffset = GetVertexComponentOffset( mesh, GrannyVertexNormalName );
	if( normalOffset != -1 )
	{
		pVtxData->m_pSrcNormal = pVtxData->m_pBuffer + normalOffset;
		pVtxData->m_dstNormalOffset = normalOffset;
	}
	unsigned int tangentOffset = GetVertexComponentOffset( mesh, GrannyVertexTangentName );
	if( tangentOffset != -1 )
	{
		pVtxData->m_pSrcTangent = pVtxData->m_pBuffer + tangentOffset;
		pVtxData->m_dstTangentOffset = tangentOffset;
	}
	unsigned int binormalOffset = GetVertexComponentOffset( mesh, GrannyVertexBinormalName );
	if( binormalOffset != -1 )
	{
		pVtxData->m_pSrcBinormal = pVtxData->m_pBuffer + binormalOffset;
		pVtxData->m_dstBinormalOffset = binormalOffset;
	}
	return true;
}

bool TriGeometryRes::SaveMeshToGrannyFile( TriGeometryResMeshData* pMesh, const char* filename )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !pMesh || !pMesh->m_indexBuffer.IsValid() || !pMesh->m_vertexBuffer.IsValid() || pMesh->m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	granny_data_type_definition grannyVertexDecl[16];
	
	Tr2VertexDefinition decl;
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( pMesh->m_vertexDeclaration, decl ) )
	{
		return false;
	}
	
	if( !ConvertVertexDeclToGranny( decl, grannyVertexDecl, sizeof(grannyVertexDecl)/sizeof(grannyVertexDecl[0]) ) )
	{
		return false;
	}

	void* pVertices;
	if( FAILED( pMesh->m_vertexBuffer.Lock( 0, 0, &pVertices, LOCK_READONLY, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&]{ pMesh->m_vertexBuffer.Unlock( renderContext ); } );

	void* pIndices;
	if( FAILED( pMesh->m_indexBuffer.Lock( 0, 0, &pIndices, LOCK_READONLY, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&]{ pMesh->m_indexBuffer.Unlock( renderContext ); } );


	granny_vertex_data myVertexData;
	memset( &myVertexData, 0, sizeof( myVertexData ) );
	myVertexData.Vertices = (granny_uint8*)pVertices;
	myVertexData.VertexCount = pMesh->m_vertexCount;
	myVertexData.VertexType = grannyVertexDecl;

	granny_tri_topology myTopology;
	memset( &myTopology, 0, sizeof( myTopology ) );

	if( pMesh->m_vertexCount <= 65535 )
	{
		myTopology.Indices16 = (granny_uint16*)pIndices;
		myTopology.Index16Count = pMesh->m_primitiveCount * 3;
	}
	else
	{
		myTopology.Indices = (granny_int32*)pIndices;
		myTopology.IndexCount = pMesh->m_primitiveCount * 3;
	}

	myTopology.GroupCount = (granny_int32)pMesh->m_areas.size();
	myTopology.Groups = CCP_NEW("myTopology.Groups") granny_tri_material_group[myTopology.GroupCount];
	if( myTopology.Groups )
	{
		for( int i = 0; i < myTopology.GroupCount; ++i )
		{
			myTopology.Groups[i].MaterialIndex = i;
			myTopology.Groups[i].TriFirst = pMesh->m_areas[i].m_firstIndex / 3;
			myTopology.Groups[i].TriCount = pMesh->m_areas[i].m_primitiveCount;
		}
	}
	else
	{
		myTopology.GroupCount = 0;
	}

	granny_mesh myMesh;
	memset( &myMesh, 0, sizeof( myMesh ) );

	myMesh.PrimaryVertexData = &myVertexData;
	myMesh.PrimaryTopology = &myTopology;

    granny_file_info fileInfo;
	memset( &fileInfo, 0, sizeof( fileInfo ) );

	granny_mesh* meshes[] = { &myMesh };
	granny_tri_topology* topologies[] = { myMesh.PrimaryTopology };
	granny_vertex_data* vertexDatas[] = { myMesh.PrimaryVertexData };

	fileInfo.ModelCount = 0;
	fileInfo.Models = 0;
	fileInfo.MeshCount = 1;
	fileInfo.Meshes = meshes;
	fileInfo.VertexDataCount = 1;
	fileInfo.VertexDatas = vertexDatas;
	fileInfo.TriTopologyCount = 1;
	fileInfo.TriTopologies = topologies;

	fileInfo.MaterialCount = (granny_int32)pMesh->m_areas.size();
	fileInfo.Materials = CCP_NEW("fileInfo.Materials") granny_material*[fileInfo.MaterialCount];
	if( fileInfo.Materials == nullptr )
	{
		fileInfo.MaterialCount = 0;
	}

	myMesh.MaterialBindingCount = fileInfo.MaterialCount;
	myMesh.MaterialBindings = CCP_NEW("myMesh.MaterialBindings") granny_material_binding[fileInfo.MaterialCount];
	if( myMesh.MaterialBindings == nullptr )
	{
		myMesh.MaterialBindingCount = 0;
	}

	granny_material* pMaterials = nullptr;
	if( fileInfo.MaterialCount )
	{
		pMaterials = CCP_NEW("pMaterials") granny_material[fileInfo.MaterialCount];
		if( pMaterials )
		{
			memset( pMaterials, 0, sizeof( granny_material ) * fileInfo.MaterialCount );

			for( int i = 0; i < fileInfo.MaterialCount; ++i )
			{
				fileInfo.Materials[i] = &pMaterials[i];
				fileInfo.Materials[i]->Name = pMesh->m_areas[i].m_name.c_str();
				myMesh.MaterialBindings[i].Material = fileInfo.Materials[i];
			}
		}
	}
	
	// Bones
	myMesh.BoneBindingCount = (granny_int32)pMesh->m_jointBindings.size();
	myMesh.BoneBindings = CCP_NEW("myMesh.BoneBindings") granny_bone_binding[myMesh.BoneBindingCount];
	if( myMesh.BoneBindings == nullptr )
	{
		myMesh.BoneBindingCount = 0;
	}
	if( myMesh.BoneBindingCount > 0 )
	{
		memset( myMesh.BoneBindings, 0, sizeof( granny_bone_binding ) * myMesh.BoneBindingCount );
		for( int i = 0; i < myMesh.BoneBindingCount; ++i )
		{
			myMesh.BoneBindings[i].BoneName = pMesh->m_jointBindings[i].c_str();
		}
	}


    granny_file_builder *Builder = GrannyBeginFile(1, GrannyCurrentGRNStandardTag,
                                                   GrannyGRNFileMV_32Bit_LittleEndian,
                                                   GrannyGetTemporaryDirectory(),
                                                   "GRNFileTemp");
    granny_file_data_tree_writer *Writer =
        GrannyBeginFileDataTreeWriting(GrannyFileInfoType, &fileInfo, 0, 0);
    GrannyWriteDataTreeToFileBuilder(Writer, Builder);
    GrannyEndFileDataTreeWriting(Writer);

	std::wstring filenameW = (const wchar_t*)CA2W( filename );
	std::wstring fullPath = BePaths->ResolvePathForWritingW( filenameW );
	GrannyEndFile( Builder, CW2A( fullPath.c_str() ) );

	CCP_DELETE [] pMaterials;
	CCP_DELETE [] myMesh.MaterialBindings;
	CCP_DELETE [] myMesh.BoneBindings;
	CCP_DELETE [] fileInfo.Materials;

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Query if the instance data is ready to be 
//   used, i.e. if the resource is good. 
// Return Value:
//   true if data is ready to be used
//   false otherwise
// --------------------------------------------------------------------------------------
bool TriGeometryRes::IsInstanceDataReady() const
{
	return IsGood();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns number of instance buffers available 
//   with this data, i.e. number of meshes.
// Return Value:
//   number of meshes
// --------------------------------------------------------------------------------------
unsigned int TriGeometryRes::GetInstanceBufferCount() const
{
	return (unsigned int)m_meshes.size();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns returns vertex declaration handle 
//   for the given instance buffer index.
// Arguments:
//   bufferIndex - instance buffer index
// Return Value:
//   vertex declaration handle
// --------------------------------------------------------------------------------------
unsigned int TriGeometryRes::GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const
{
	if( bufferIndex < m_meshes.size() )
	{
		return m_meshes[bufferIndex]->m_vertexDeclaration;
	}
	return Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns number of vertexes in the given 
//   instance buffer.
// Arguments:
//   bufferIndex - instance buffer index
// Return Value:
//   number of vertexes in the mesh
// --------------------------------------------------------------------------------------
unsigned int TriGeometryRes::GetInstanceBufferVertexCount( unsigned int bufferIndex ) const
{
	if( bufferIndex < m_meshes.size() )
	{
		return m_meshes[bufferIndex]->m_vertexCount;
	}
	return 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns vertex buffer with instance data.
// Arguments:
//   bufferIndex - instance buffer index
//   buffer - (out) vertex buffer containing instance data (can be null)
//   stride - (out) vertex stride for the vertex buffer
// --------------------------------------------------------------------------------------
void TriGeometryRes::GetVertexBuffer( unsigned int bufferIndex, Tr2VertexBufferAL*& buffer, unsigned& stride )
{
	if( bufferIndex < m_meshes.size() )
	{
		buffer = &m_meshes[bufferIndex]->m_vertexBuffer;
		stride = m_meshes[bufferIndex]->m_bytesPerVertex;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GpuBuffer interface. Returns GPU buffer with instance data.
// Arguments:
//   bufferIndex - instance buffer index
// Return Value:
//   GPU buffer containing instance data
// --------------------------------------------------------------------------------------
Tr2GpuBufferAL* TriGeometryRes::GetGpuBuffer( unsigned bufferIndex )
{
	if( !IsGood() )
	{
		return nullptr;
	}
	if( bufferIndex < m_meshes.size() )
	{
		return &m_meshes[bufferIndex]->m_shaderResourceBuffer;
	}
	return nullptr;
}

Be::Result<std::string> TriGeometryRes::GetModelName( unsigned int ix, std::string& name )
{
	if( ix >= GetModelCount() )
	{
		return Be::Result<std::string>( "Model index out of range" );
	}

	TriGeometryResModelData* pModel = GetModelData( ix );
	if( !pModel )
	{
		return Be::Result<std::string>( "Invalid model" );
	}

	name = pModel->m_name;

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGeometryRes::GetMeshName( unsigned int ix, std::string& name )
{
	if( ix >= GetMeshCount() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}

	TriGeometryResMeshData* pMesh = GetMeshData( ix );
	if( !pMesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	name = pMesh->m_name;

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGeometryRes::GetMeshAreaCount( unsigned int ix, int& count )
{
	if( ix >= GetMeshCount() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}

	TriGeometryResMeshData* pMesh = GetMeshData( ix );
	if( !pMesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	count = (int)pMesh->m_areas.size();

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGeometryRes::GetMeshAreaName( unsigned int meshIx, unsigned int areaIx, std::string& name )
{
	if( meshIx >= GetMeshCount() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}

	TriGeometryResMeshData* pMesh = GetMeshData( meshIx );
	if( !pMesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	if( areaIx >= pMesh->m_areas.size() )
	{
		return Be::Result<std::string>( "Area index out of range" );
	}

	name = pMesh->m_areas[areaIx].m_name;

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGeometryRes::GetAreaBoundingBoxFromScript( unsigned int meshIx, unsigned int areaIx, std::pair<Vector3, Vector3>& bounds )
{
	if( meshIx >= GetMeshCount() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}

	TriGeometryResMeshData* pMesh = GetMeshData( meshIx );
	if( !pMesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	if( areaIx >= pMesh->m_areas.size() )
	{
		return Be::Result<std::string>( "Area index out of range" );
	}

	bounds = std::make_pair( pMesh->m_areas[areaIx].m_minBounds, pMesh->m_areas[areaIx].m_maxBounds );

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGeometryRes::GetBoundingSphereFromScript( unsigned int meshIx, std::pair<Vector3, float>& bounds )
{
	if( meshIx >= m_meshes.size() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}
	TriGeometryResMeshData* pMesh = m_meshes[meshIx];
	if( !pMesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	bounds = std::make_pair( Vector3( pMesh->m_boundingSphere.x, pMesh->m_boundingSphere.y, pMesh->m_boundingSphere.z ), pMesh->m_boundingSphere.w );
	return Be::Result<std::string>();
}

Be::Result<std::string> TriGeometryRes::CalculateBoundingBoxFromTransform( unsigned int meshIx, const Matrix& transform, std::pair<Vector3, Vector3>& bounds )
{
	if( meshIx >= m_meshes.size() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}
	TriGeometryResMeshData* pMesh = m_meshes[meshIx];
	if( !pMesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	CalcBoundingBoxContext ctx;
	ctx.min = Vector3(  FLT_MAX,  FLT_MAX,  FLT_MAX );
	ctx.max = Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	ctx.transform = transform;

	if( meshIx < m_meshes.size() )
	{
		ProcessMeshTriangles( meshIx, &CalcBoundingBox, &ctx );
	}

	bounds = std::make_pair( ctx.min, ctx.max );
	return Be::Result<std::string>();
}
