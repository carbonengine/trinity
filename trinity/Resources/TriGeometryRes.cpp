#include "StdAfx.h"
#include "Include/TriMath.h"
#include "TriGeometryRes.h"
#include "TriGrannyRes.h"
#include "Tr2PerObjectData.h"
#include "Tr2VertexDefinitionUtilities.h"

#include "TriSettingsRegistrar.h"

#include "Utilities/GeometryUtils.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/BoundingBox.h"

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARE( geometryResBytes, "Trinity/geometryResBytes", false, CST_MEMORY, "Size of memory occupied by geometry resources." );

#if TRINITY_PLATFORM != TRINITY_DIRECTX11
const auto gpuUsage = Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE;
#else
const auto gpuUsage = Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::INDEX_BUFFER;
#endif

Tr2SuballocatedBuffer g_sharedBuffer( "TriGeometryRes shared vertex/index buffer", gpuUsage, SHARED_BUFFER_BLOCK_SIZE, SHARED_BUFFER_MAX_SIZE );

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


#ifdef _WIN32
#pragma pack(push, 4)
#endif

struct MeshBoundsInfo
{
	const char* typeName;
	BoundingBox bounds;
	granny_int32 areaCount;
	AreaBoundsInfo* areaInfos;
	granny_int32 sourceMeshIndex;
	granny_int32 maxScreenSize;
	granny_int32 uvDensityCount;
	granny_real32* uvDensities;
}
#ifndef _WIN32
// On non-windows x64 platforms areaInfos maybe 64bit aligned
__attribute__((packed))
#endif
;

#ifdef _WIN32
#pragma pack(pop)
#endif


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

granny_data_type_definition UvDensityInfoType[] = {
	{ GrannyReal32Member, "density" },
	{ GrannyEndMember }
};

granny_data_type_definition MeshBoundsInfoType[] = {
	{ GrannyStringMember, "typeName" },
	{ GrannyInlineMember, "bounds", BoundingBoxType },
	{ GrannyReferenceToArrayMember, "areaInfo", AreaBoundsInfoType },
	{ GrannyInt32Member, "sourceMeshIndex" },
	{ GrannyInt32Member, "maxScreenSize" },
	{ GrannyReferenceToArrayMember, "uvDensities", UvDensityInfoType },
	{ GrannyEndMember }
};

//
//////////////////////////////////////////////////////////////////////////

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


static void ConvertDataToVector3( Tr2VertexDefinition::DataType elementType, const void * src, Vector3 * dest)
{

	switch(elementType)
	{
	case Tr2VertexDefinition::FLOAT16_4:
		{
			*reinterpret_cast<Vector3*>( dest ) = *static_cast<const Vector3_16*>( src );
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
			break;
		}

	default:
		{
			dest->x = 0.0f;
			dest->y = 0.0f;
			dest->z = 0.0f;
		}

	}

}


uint32_t GetPrimitiveCount( const TriGeometryResMeshData& mesh, uint32_t index, uint32_t count )
{
	if( index >= mesh.m_areas.size() )
	{
			return 0;
	}

	if( index + count > mesh.m_areas.size() )
	{
			count = uint32_t( mesh.m_areas.size() - index );
	}

	auto& meshArea = mesh.m_areas[index];

	uint32_t primCount = uint32_t( meshArea.m_primitiveCount );
	for( uint32_t i = 1; i < count; ++i )
	{
			primCount += mesh.m_areas[index + i].m_primitiveCount;
	}
	return primCount;
}

namespace
{
ALResult ReverseIndexBuffer( TriGeometryResMeshData& mesh, granny_mesh& grannyMesh, Tr2RenderContext& renderContext ) 
{
	uint32_t bytesPerIndex = 2;
	auto indexCount = grannyMesh.PrimaryTopology->Index16Count;
	if( indexCount == 0 )
	{
		indexCount = grannyMesh.PrimaryTopology->IndexCount;
		if( indexCount == 0 )
		{
			return S_OK;
		}
		if( grannyMesh.PrimaryVertexData->VertexCount > 65535 )
		{
			bytesPerIndex = 4;
		}
	}

	std::vector<uint8_t> tempBuffer( indexCount * bytesPerIndex );
	GrannyCopyMeshIndices( &grannyMesh, bytesPerIndex, tempBuffer.data() );

	if( bytesPerIndex == 2 )
	{
		std::reverse( reinterpret_cast<uint16_t*>( tempBuffer.data() ), reinterpret_cast<uint16_t*>( tempBuffer.data() + tempBuffer.size() ) );
	}
	else
	{
		std::reverse( reinterpret_cast<uint32_t*>( tempBuffer.data() ), reinterpret_cast<uint32_t*>( tempBuffer.data() + tempBuffer.size() ) );
	}

	CR_RETURN_HR( g_sharedBuffer.Allocate( bytesPerIndex, indexCount, tempBuffer.data(), renderContext, mesh.m_reversedIndexAllocation ) );
	mesh.m_reversedIndicesValid = true;
	return S_OK;
}

}


TriGeometryRes::TriGeometryRes(IRoot* lockobj) :
	m_pGrannyFile( NULL ),
	m_memoryUse( 0 ),
	m_meshes( "TriGeometryRes/m_meshes" ),
	m_meshLods( "TriGeometryRes/m_meshLods" ),
	m_models( "TriGeometryRes/m_models" ),
	m_skeletons( "TriGeometryRes/m_skeletons" ),
	m_inMemoryInfo( NULL )
{
}

TriGeometryRes::~TriGeometryRes()
{
	ReleaseResources( TRISTORAGE_ALL );
	ClearGrannyData();
}

void TriGeometryRes::ClearGrannyData()
{
	m_meshes.clear();
	m_meshLods.clear();
	m_models.clear();
	m_skeletons.clear();

	if( m_pGrannyFile )
	{
		GrannyFreeFile( m_pGrannyFile );
		m_pGrannyFile = 0;
	}
}

granny_file_info* TriGeometryRes::GetGrannyInfo() const
{
	if( m_pGrannyFile )
	{
		return GrannyGetFileInfo( m_pGrannyFile );
	}
	else if( m_inMemoryInfo )
	{
		return m_inMemoryInfo;
	}
	else
	{
		return nullptr;
	}
}

unsigned int TriGeometryRes::GetAnimationCount() const
{
	if( auto info = GetGrannyInfo() )
	{
		return info->AnimationCount;
	}
	return 0;
}

unsigned int TriGeometryRes::GetModelCount() const
{
	return unsigned( m_models.size() );
}

TriGeometryResModelData* TriGeometryRes::GetModelData( unsigned int modelIx ) const
{
	if( modelIx < m_models.size() )
	{
		return m_models[modelIx].get();
	}
	else
	{
		return nullptr;
	}
}

unsigned int TriGeometryRes::GetMeshCount() const
{
	return unsigned( m_meshes.size() );
}

TriGeometryResMeshData* TriGeometryRes::GetMeshData( unsigned int meshIx ) const
{
	if( !m_isGood )
	{
		return nullptr;
	}
	if( meshIx < m_meshes.size() )
	{
		return m_meshes[meshIx].get();
	}
	return nullptr;
}

TriGeometryResMeshData* TriGeometryRes::GetMeshData( unsigned int meshIx, float screenSize ) const
{
	auto mesh = GetMeshData( meshIx );
	if( mesh )
	{
		int lodIndex = GetLodIndexForScreenSize( meshIx, screenSize );
		if( lodIndex >= 0 )
		{
			mesh = m_meshLods[mesh->m_lods[lodIndex].meshIndex].get();
		}
	}
	return mesh;
}

TriGeometryResMeshData* TriGeometryRes::GetMeshDataLod( unsigned int meshIx, int lodIndex ) const
{
	auto mesh = GetMeshData( meshIx );
	if( mesh && lodIndex >= 0 && lodIndex < mesh->m_lods.size() )
	{
		return m_meshLods[mesh->m_lods[lodIndex].meshIndex].get();
	}

	return nullptr;
}

int TriGeometryRes::GetLodIndexForScreenSize( unsigned int meshIx, float screenSize ) const
{
	auto mesh = GetMeshData( meshIx );
	if( mesh && !isinf( screenSize ) )
	{
		if( m_forceLod && m_forcedLodIndex > 0 )
		{
			return int( std::min( m_forcedLodIndex, uint32_t( mesh->m_lods.size() ) ) - 1 );
		}

		for( int i = static_cast<int>( mesh->m_lods.size() - 1 ); i >= 0; i-- )
		{
			if( mesh->m_lods[i].maxScreenSize >= screenSize )
			{
				return i;
			}
		}
	}
	return -1;
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
	TriGeometryResMeshData* pMesh = GetMeshData( meshIx );
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
	TriGeometryResMeshData* pMesh = m_meshes[meshIx].get();
	if( !pMesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	bounds = std::make_pair( pMesh->m_minBounds, pMesh->m_maxBounds );

	return Be::Result<std::string>();
}

bool TriGeometryRes::GetAreaBoundingBox( unsigned int meshIx, unsigned int areaIx, Vector3& min, Vector3& max ) const
{
	TriGeometryResMeshData* pMesh = GetMeshData( meshIx );
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


bool TriGeometryRes::GetBoundingSphere( unsigned int meshIx, Vector4& sphere ) const
{
	TriGeometryResMeshData* pMesh = GetMeshData( meshIx );
	if( !pMesh )
	{
		return false;
	}

	sphere = pMesh->m_boundingSphere;

	return true;
}

unsigned int TriGeometryRes::GetAreaCount( unsigned int meshIx ) const
{
	if( meshIx >= m_meshes.size() || m_meshes[meshIx] == NULL )
	{
		return 0;
	}
	return (unsigned int)m_meshes[meshIx]->m_areas.size();
}

TriGeometryResAreaData* TriGeometryRes::GetAreaData( unsigned int meshIx, unsigned int areaIx ) const
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

		CreateMeshesFromGrannyFile( gi, Tr2CpuUsage::READ, renderContext );

		if( gi->AnimationCount == 0 && m_pGrannyFile )
		{
			bool hasMeshBindings = false;
			for( int i = 0; i < gi->MeshCount; ++i )
			{
				if( gi->Meshes[i]->BoneBindingCount > 0 )
				{
					hasMeshBindings = true;
					break;
				}
			}

			if( !hasMeshBindings )
			{
				// Only meshes in the file - we've converted those to D3D structures.
				GrannyFreeFile( m_pGrannyFile );
				m_pGrannyFile = 0;
			}
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

	BoundingBoxInitialize( area.m_minBounds, area.m_maxBounds );

	std::set<int32_t> vertexIndicesSeen;

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

		BoundingBoxUpdate( area.m_minBounds, area.m_maxBounds, vertexPosition );
	}

	area.m_vertexCount = (int)vertexIndicesSeen.size();
}

bool TriGeometryRes::IsAreaSkinned( TriGeometryResAreaData& area, granny_mesh* myMesh, granny_file_info* gi, int bytesPerVertex )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	// offset to boneindex
	int boneIndexOffset = GetVertexComponentOffset( myMesh, GrannyVertexBoneIndicesName );
	// if there are no bone-indices , we are done
	if( boneIndexOffset == -1 )
	{
		return false;
	}

	auto FindRootBoneName = [&]() -> const char* {
		for( int32_t i = 0; i < gi->ModelCount; ++i )
		{
			granny_model* model = gi->Models[i];
			if( model->Skeleton && model->Skeleton->BoneCount > 1 )
			{
				for( int32_t j = 0; j < model->MeshBindingCount; ++j )
				{
					return model->Skeleton->Bones[0].Name;
				}
			}
		}
		return nullptr;
	};

	auto FindRootBoneIndex = [&]() -> std::optional<uint8_t> {
		const char* rootBone = FindRootBoneName();
		for( int32_t i = 0; i < myMesh->BoneBindingCount; ++i )
		{
			if( myMesh->BoneBindings[i].BoneName && strcmp( myMesh->BoneBindings[i].BoneName, rootBone ) == 0 )
			{
				return uint8_t( i );
			}
		}
		return {};
	};

	auto rootBoneIndex = FindRootBoneIndex();
	if( !rootBoneIndex.has_value() )
	{
		return true;
	}

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

		// bones
		uint8_t boneIndex0 = *(pBoneIndex0 + index * bytesPerVertex);
		uint8_t boneIndex1 = *(pBoneIndex1 + index * bytesPerVertex);
		uint8_t boneIndex2 = *(pBoneIndex2 + index * bytesPerVertex);
		uint8_t boneIndex3 = *(pBoneIndex3 + index * bytesPerVertex);

		if( boneIndex0 != *rootBoneIndex || boneIndex1 != *rootBoneIndex || boneIndex2 != *rootBoneIndex || boneIndex3 != *rootBoneIndex )
		{
			// found it attached to a bone so return early
			return true;
		}
	}
	// if we reach this point, then no vertices have any valid bone indices
	return false;
}

bool TriGeometryRes::SetupMeshes( granny_file_info* gi )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_meshes.reserve( gi->MeshCount );
	std::vector<std::pair<granny_int32, float>> lods;
	std::vector<granny_int32> meshIndices;
	

	for( int32_t meshIx = 0; meshIx < gi->MeshCount; ++meshIx )
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

		std::unique_ptr<TriGeometryResMeshData> pMesh( new TriGeometryResMeshData() );
		if( pMesh == nullptr )
		{
			CCP_LOGWARN( "Out of memory in TriGeometryRes::SetupMeshes" );
			return false;
		}

		CopyGrannyName( pMesh->m_name, myMesh->Name );
		pMesh->m_grannyMeshIndex = meshIx;
		pMesh->m_bytesPerVertex = bytesPerVertex;
		pMesh->m_vertexCount = vertexCount;
		pMesh->m_primitiveCount = 0;

		for( int jointIx = 0; jointIx < myMesh->BoneBindingCount; ++jointIx )
		{
			TriJointBinding binding;
			binding.m_name = myMesh->BoneBindings[jointIx].BoneName;
			binding.m_obbMin = *reinterpret_cast<const Vector3*>( myMesh->BoneBindings[jointIx].OBBMin );
			binding.m_obbMax = *reinterpret_cast<const Vector3*>( myMesh->BoneBindings[jointIx].OBBMax );
			pMesh->m_jointBindings.push_back( binding );
		}


		MeshBoundsInfo* mbi = NULL;

		// Look for mesh bounds info in the Granny file
		if( myMesh->ExtendedData.Object )
		{
			// The mesh has extended data - ask Granny to convert it to our current version of the bounds
			// info data structure.
			mbi = static_cast<MeshBoundsInfo*>( GrannyConvertTree( myMesh->ExtendedData.Type, myMesh->ExtendedData.Object, MeshBoundsInfoType, nullptr, nullptr ) );
			if( !mbi->typeName || (strcmp( mbi->typeName, "MeshBoundsInfo" ) != 0) )
			{
				// This extended data doesn't match our expectations
				GrannyFreeBuilderResult( (void*)mbi );
				mbi = NULL;
			}
		}

		if( mbi && mbi->uvDensityCount > 0 && mbi->uvDensities )
		{
			pMesh->m_uvDensities.assign( mbi->uvDensities, mbi->uvDensities + mbi->uvDensityCount );
		}
		if( !mbi )
		{
			BoundingBoxInitialize( pMesh->m_minBounds, pMesh->m_maxBounds );
		}
		else
		{
			pMesh->m_minBounds = *reinterpret_cast<Vector3*>( &mbi->bounds.min[0] );
			pMesh->m_maxBounds = *reinterpret_cast<Vector3*>( &mbi->bounds.max[0] );
		}
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

				area.m_isSkinned = IsAreaSkinned( area, myMesh, gi, bytesPerVertex );

				if( mbi )
				{
					CCP_ASSERT( groupIx < mbi->areaCount );
					area.m_minBounds = *reinterpret_cast<Vector3*>( &mbi->areaInfos[groupIx].bounds.min[0] );
					area.m_maxBounds = *reinterpret_cast<Vector3*>( &mbi->areaInfos[groupIx].bounds.max[0] );
					area.m_vertexCount = mbi->areaInfos[groupIx].vertexCount;
				}
				else
				{
					DetermineAreaBoundsAndVertCount( area, myMesh, bytesPerVertex );

					// if the area doesn't have any verts, the bounding box is invalid, so DON't use it!
					if( area.m_primitiveCount )
					{
						BoundingBoxUpdate( pMesh->m_minBounds, pMesh->m_maxBounds, area.m_minBounds, area.m_maxBounds );
					}
				}

			}
		}

		if( mbi && mbi->maxScreenSize > 0)
		{
			lods.push_back( { mbi->sourceMeshIndex, float( mbi->maxScreenSize ) } );
			pMesh->m_isLodMesh = true;
			m_meshLods.push_back( std::move( pMesh ) );
			meshIndices.push_back( -1 );
		}
		else
		{
			meshIndices.push_back( granny_int32( m_meshes.size() ) );
			m_meshes.push_back( std::move( pMesh ) );
		}


		if( mbi )
		{
			GrannyFreeBuilderResult( (void*)mbi );
		}

	}

	for( size_t i = 0; i < m_meshLods.size(); ++i )
	{
		auto idx = lods[i].first;
		if( idx >= 0 && idx < meshIndices.size() && meshIndices[idx] >= 0 )
		{
			m_meshes[meshIndices[idx]]->m_lods.push_back( { i, lods[i].second } );
		}
		else
		{
			CCP_LOGWARN( "Invalid mesh index %i for LOD mesh \"%s\" in \"%ls\"", int( idx ), m_meshLods[i]->m_name.c_str(), GetPath () );
		}
	}

	for( auto& mesh : m_meshes )
	{
		sort( begin( mesh->m_lods ), end( mesh->m_lods ), []( auto& a, auto& b ) { return a.maxScreenSize > b.maxScreenSize; } );
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

		std::unique_ptr<TriGeometryResModelData> pModel( new TriGeometryResModelData() );
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

		std::swap( m_models[modelIndex], pModel );
	}
}

void TriGeometryRes::SetupSkeletons( granny_file_info* gi )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_skeletons.resize( gi->SkeletonCount );
	for( int skelIx = 0; skelIx < gi->SkeletonCount; ++skelIx )
	{
		granny_skeleton* skel = gi->Skeletons[skelIx];
		std::unique_ptr<TriGeometryResSkeletonData> pSkelData( new TriGeometryResSkeletonData() );
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

		std::swap( m_skeletons[skelIx], pSkelData );
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

		CCP_STATS_ZONE( "TriGeometryRes::ReadGrannyFile reading Granny file" );

		m_pGrannyFile = ProtectedGrannyReadEntireFileFromMemory( m_path.c_str(), uint32_t( m_dataStream->GetSize() ), data );

		if( !m_pGrannyFile )
		{
			return false;
		}

		gi = GrannyGetFileInfo( m_pGrannyFile );
	}
	else
	{
		m_pGrannyFile = NULL;
		m_inMemoryInfo = gi;
	}

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
	CreateMeshesFromGrannyFile( gi, Tr2CpuUsage::READ | Tr2CpuUsage::WRITE, renderContext );

	m_sourceGranny = g;

	SetPrepared( true );
	SetGood( true );
}

void TriGeometryRes::RecalculateBoundingSphere()
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	for ( auto& mesh : m_meshes )
	{
		if( mesh == nullptr )
		{
			continue;
		}

		// need all the verts
		if( !mesh->m_allocationsValid )
		{
			continue;
		}

		// vertex info
		uint32_t vertSize = mesh->m_bytesPerVertex;
		const uint8_t* pVertices;
		if( SUCCEEDED( mesh->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
		{
			// need vertex declaration to get offset of position element in the vertex
			Tr2VertexDefinition decl;
			if( Tr2EffectStateManager::GetVertexDeclarationElements( mesh->m_vertexDeclaration, decl ) )
			{
				auto position = decl.Find( decl.POSITION, 0 );

				// build a list of pointers to the positions
				std::vector<const Vector3*> points( mesh->m_vertexCount );
				for( uint32_t p = 0; p < mesh->m_vertexCount; ++p )
				{
					points[p] = (const Vector3*)&pVertices[p * vertSize + position->m_offset];
				}

				// all is done in this recursive function
				BoundingSphereFromPoints( mesh->m_boundingSphere, &points[0], points.size() );
			}
			mesh->m_vertexAllocation.UnmapForReading( renderContext );
		}
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
		v[i] = Transform( v[i], ctx->transform );
		v[i] /= v[i].w;

		ctx->max.x = std::max( ctx->max.x, v[i].x );
		ctx->max.y = std::max( ctx->max.y, v[i].y );
		ctx->max.z = std::max( ctx->max.z, v[i].z );

		ctx->min.x = std::min( ctx->min.x, v[i].x );
		ctx->min.y = std::min( ctx->min.y, v[i].y );
		ctx->min.z = std::min( ctx->min.z, v[i].z );
	}
}

static void ConvertTriangleData( Tr2VertexDefinition::DataType elementType, unsigned int vertexByteSize, const uint8_t * vertexBase,
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

	if( m_meshes[i] == NULL || !m_meshes[i]->m_allocationsValid )
	{
		return;
	}

	const uint8_t* pVertices;
	const uint8_t* pIndices;

	int vertSize = m_meshes[i]->m_bytesPerVertex;
	if( FAILED( m_meshes[i]->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
	{
		return;
	}
	ON_BLOCK_EXIT( [&] { m_meshes[i]->m_vertexAllocation.UnmapForReading( renderContext ); } );
	if( FAILED( m_meshes[i]->m_indexAllocation.MapForReading( pIndices, renderContext ) ) )
	{
		return;
	}
	ON_BLOCK_EXIT( [&] { m_meshes[i]->m_indexAllocation.UnmapForReading( renderContext ); } );

	const uint16_t* pShortIndices = (uint16_t*)pIndices;
	const uint32_t* pLongIndices = (uint32_t*)pIndices;
	

	Tr2VertexDefinition decl;
	if ( !Tr2EffectStateManager::GetVertexDeclarationElements( m_meshes[i]->m_vertexDeclaration, decl ) )
	{
		return;
	}

	auto foundPosition = decl.Find( Tr2VertexDefinition::POSITION, 0 );
	if( !foundPosition )
	{
		return;
	}
	unsigned int positionByteOffset = foundPosition->m_offset;
	Tr2VertexDefinition::DataType declType = foundPosition->m_dataType;

	int numPrim = m_meshes[i]->m_primitiveCount;
	for ( int j = 0; j < numPrim; j++ )
	{
		unsigned int index1 = 0;
		unsigned int index2 = 0;
		unsigned int index3 = 0;
		Vector3 p1;
		Vector3 p2;
		Vector3 p3;
		if (m_meshes[i]->m_indexAllocation.GetStride() == 2 )
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

bool TriGeometryRes::GetIntersectionPointNormalBone( const Vector3* pos, const Vector3* dir, Vector3* hitpoint, Vector3* normal, int* boneIndex, unsigned int areaIx )
{
	Vector3 farPoint;
	Vector3 farPointNormal;
	int farBoneIndex;
	return GetIntersectionPoints( pos, dir, hitpoint, normal, &farPoint, &farPointNormal, boneIndex, &farBoneIndex, areaIx );
}

std::pair<bool, std::pair<int, std::pair<Vector3, Vector3>>> TriGeometryRes::GetIntersectionPointNormalBoneFromScript( const Vector3& pos, const Vector3& dir )
{
	Vector3 hitpoint( 0.0f, 0.0f, 0.0f );
	Vector3 normal( 0.0f, 0.0f, 0.0f );
	int boneIndex;
	bool result = GetIntersectionPointNormalBone( &pos, &dir, &hitpoint, &normal, &boneIndex );
	return std::make_pair( result, std::make_pair( boneIndex, std::make_pair( hitpoint, normal ) ) );
}

Be::Result<std::string> TriGeometryRes::GetAreaIntersectionPointNormalBoneFromScript( const Vector3& pos, const Vector3& dir, int areaIx, std::pair<bool, std::pair<int, std::pair<Vector3, Vector3>>>& result )
{
	Vector3 hitpoint( 0.0f, 0.0f, 0.0f );
	Vector3 normal( 0.0f, 0.0f, 0.0f );
	int boneIndex;
	
	if( areaIx < -1 )
	{
		// -1 is a special case handled to maintain legacy behavior.
		return Be::Result<std::string>("Invalid area index");
	}

	// In Python 3, passing a signed value to an unsigned C exposed attribute is an error.
	// In order to maintain backwards compatible behavior with Python 2
	// this method got changed to accept signed integers and cast them to unsigned.
	auto unsignedAreaIndex = static_cast<unsigned int>(areaIx);

	bool success = GetIntersectionPointNormalBone( &pos, &dir, &hitpoint, &normal, &boneIndex, unsignedAreaIndex );
	result = std::make_pair( success, std::make_pair( boneIndex, std::make_pair( hitpoint, normal ) ) );
	return Be::Result<std::string>();
}

static bool GetBoneIndex( Tr2VertexDefinition::DataType elementType, const void* src, int& dest )
{
	if( elementType != Tr2VertexDefinition::UBYTE_4 )
	{
		CCP_LOGERR( "TriGeometryRes: BELNDINDICE using unsupported format." );
		return false;
	}

	const uint8_t* vdata = static_cast<const uint8_t*>( src );
	dest = vdata[0];
	return true;
}

static bool IntersectTri(
	const Vector3* p0,
	const Vector3* p1,
	const Vector3* p2,
	const Vector3* rayPos,
	const Vector3* rayDir,
	float *u,
	float *v,
	float *dist )
{
	Matrix m;
	Vector4 vec;

	m.m[0][0] = p1->x - p0->x;
	m.m[1][0] = p2->x - p0->x;
	m.m[2][0] = -rayDir->x;
	m.m[3][0] = 0.0f;
	m.m[0][1] = p1->y - p0->y;
	m.m[1][1] = p2->y - p0->y;
	m.m[2][1] = -rayDir->y;
	m.m[3][1] = 0.0f;
	m.m[0][2] = p1->z - p0->z;
	m.m[1][2] = p2->z - p0->z;
	m.m[2][2] = -rayDir->z;
	m.m[3][2] = 0.0f;
	m.m[0][3] = 0.0f;
	m.m[1][3] = 0.0f;
	m.m[2][3] = 0.0f;
	m.m[3][3] = 1.0f;

	vec.x = rayPos->x - p0->x;
	vec.y = rayPos->y - p0->y;
	vec.z = rayPos->z - p0->z;
	vec.w = 0.0f;

	if( Inverse( m, m ) )
	{
		vec = Transform( vec, m );
		if( ( vec.x >= 0.0f ) && ( vec.y >= 0.0f ) && ( vec.x + vec.y <= 1.0f ) && ( vec.z >= 0.0f ) )
		{
			*u = vec.x;
			*v = vec.y;
			*dist = fabs( vec.z );
			return true;
		}
	}

	return false;
}

bool TriGeometryRes::GetIntersectionPoints( const Vector3* pos, const Vector3*dir, Vector3* hitpointNear, Vector3* hitpointNearNormal, Vector3* hitpointFar, Vector3* hitpointFarNormal, int* boneIndexNear, int* boneIndexFar, unsigned int areaIx )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();
	
	*boneIndexFar = *boneIndexNear = -1;
	int boneIndex = 0;
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

		const uint8_t* pVertices;
		const uint8_t* pIndices;

		int vertSize = m_meshes[i]->m_bytesPerVertex;
		if( FAILED( m_meshes[i]->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
		{
			return 0;
		}
		ON_BLOCK_EXIT( [&] { m_meshes[i]->m_vertexAllocation.UnmapForReading( renderContext ); } );
		if( FAILED( m_meshes[i]->m_indexAllocation.MapForReading( pIndices, renderContext ) ) )
		{			
			return false;
		}
		ON_BLOCK_EXIT( [&] { m_meshes[i]->m_indexAllocation.UnmapForReading( renderContext ); } );
		
		const uint16_t* pShortIndices = (uint16_t*)pIndices;
		const uint32_t* pLongIndices = (uint32_t*)pIndices;
		
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
		
		const Tr2VertexDefinition::Item* const blendIndices = decl.Find( decl.BLENDINDICES );
		int numPrim = m_meshes[i]->m_primitiveCount;
		auto currentIndex = 0;
		if ( areaIx != -1 )
		{
			if ( areaIx >= m_meshes[i]->m_areas.size() )
			{
				continue;
			}
			currentIndex = m_meshes[i]->m_areas[areaIx].m_firstIndex;
			numPrim = m_meshes[i]->m_areas[areaIx].m_primitiveCount;
		}

		for ( int j = 0; j < numPrim; j++ )
		{
			unsigned int index1 = 0;
			unsigned int index2 = 0;
			unsigned int index3 = 0;
			Vector3 p1;
			Vector3 p2;
			Vector3 p3;
			float pu, pv, dist;
			if( m_meshes[i]->m_indexAllocation.GetStride() == 2 )
			{
				index1 = pShortIndices[currentIndex++];
				index2 = pShortIndices[currentIndex++];
				index3 = pShortIndices[currentIndex++];
			}
			else
			{
				index1 = pLongIndices[currentIndex++];
				index2 = pLongIndices[currentIndex++];
				index3 = pLongIndices[currentIndex++];
			}

			ConvertTriangleData( position->m_dataType, vertSize, pVertices, index1, index2, index3, &p1, &p2, &p3 );

			if ( IntersectTri(&p1, &p2, &p3, pos, dir, &pu, &pv, &dist ) )
			{
				float v1 = 1.0f - (pu + pv);
				Vector3 avec = p2 - p1;
				Vector3 bvec = p3 - p1;
				if ( minDist > dist )
				{
					*hitpointNear = p1*v1 + p2*pu + p3*pv;
					*hitpointNearNormal = Normalize( Cross( avec, bvec ) );
					minDist = dist;
					if( blendIndices && GetBoneIndex( blendIndices->m_dataType, pVertices + index1 * vertSize + blendIndices->m_offset, boneIndex ) )
					{
						*boneIndexNear = boneIndex;
					}
				}
				if ( maxDist < dist )
				{
					*hitpointFar = p1*v1 + p2*pu + p3*pv;
					*hitpointFarNormal = Normalize( Cross( avec, bvec ) );
					maxDist = dist;
					if( blendIndices && GetBoneIndex( blendIndices->m_dataType, pVertices + index1 * vertSize + blendIndices->m_offset, boneIndex ) )
					{
						*boneIndexFar = boneIndex;
					}
				}
				result = true;
			}

		}
	}

	if ( minDist == FLT_MAX )
	{
		*hitpointNear = *hitpointFar;
		*hitpointNearNormal = *hitpointFarNormal;
		*boneIndexNear = *boneIndexFar;
	}

	if ( maxDist == FLT_MIN )
	{
		*hitpointFar = *hitpointNear;
		*hitpointFarNormal = *hitpointNearNormal;
		*boneIndexFar = *boneIndexNear;
	}

	return result;
}

unsigned int TriGeometryRes::GetSkeletonCount() const
{
	return (unsigned int)m_skeletons.size();
}

TriGeometryResSkeletonData* TriGeometryRes::GetSkeletonData( unsigned int skelIx ) const
{
	CCP_ASSERT( skelIx < m_skeletons.size() );

	return m_skeletons[skelIx].get();
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
	m_meshes.clear();
	m_meshLods.clear();

	CCP_STATS_ADD( geometryResBytes, -(int)m_memoryUse );
	m_memoryUse = 0;
}

unsigned int TriGeometryResSkeletonData::FindJoint( const char* name ) const
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


TriGeometryResSkeletonData::TriGeometryResSkeletonData() :
	m_joints( "TriGeometryResSkeletonData/m_joints" )
{
}

TriGeometryResAreaData::TriGeometryResAreaData() :
	m_firstIndex( 0 ),
	m_primitiveCount( 0 ),
	m_vertexCount( 0 ),
	m_jointBindings( "TriGeometryResAreaData/m_jointBindings" ),
	m_isSkinned( false )
{
}

TriGeometryResMeshData::TriGeometryResMeshData() :
	m_bytesPerVertex( 0 ),
	m_vertexCount( 0 ),
	m_primitiveCount( 0 ),
	m_areas( "TriGeometryResMeshData/m_areas" ),
	m_jointBindings( "TriGeometryResMeshData/m_jointBindings" ),
	m_isLodMesh( false ),
	m_pVertexData( NULL ),
	m_vertexDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_allocationsValid(false),
	m_reversedIndicesValid(false)
{
}

void TriGeometryRes::RequestReversedIndexBuffers()
{
	if ( m_reversedIndexBuffersRequested )
	{
		return;
	}
	m_reversedIndexBuffersRequested = true;
	if( !m_isPrepared )
	{
		return;
	}
	if( m_sourceGranny )
	{
		if( !m_isGood )
		{
			return;
		}
		granny_file* f = m_sourceGranny->GetGrannyFile();
		if( !f )
		{
			return;
		}
		granny_file_info* gi = GrannyGetFileInfo( f );

		USE_MAIN_THREAD_RENDER_CONTEXT();

		for( auto& mesh : m_meshes )
		{
			auto grannyMesh = gi->Meshes[mesh->m_grannyMeshIndex];
			ReverseIndexBuffer( *mesh, *grannyMesh, renderContext );
		}
		for( auto& mesh : m_meshLods )
		{
			auto grannyMesh = gi->Meshes[mesh->m_grannyMeshIndex];
			ReverseIndexBuffer( *mesh, *grannyMesh, renderContext );
		}
	}
	else
	{
		Reload();
	}
}

bool TriGeometryRes::RenderAreas( unsigned int meshIx, unsigned int areaIx, unsigned int areaCount, Tr2RenderContext& renderContext, bool reversed )
{
	return RenderAreas( std::numeric_limits<float>::max(), meshIx, areaIx, areaCount, renderContext, reversed );
}

bool TriGeometryRes::RenderAreas( float screenSize, unsigned int meshIx, unsigned int areaIx, unsigned int areaCount, Tr2RenderContext& renderContext, bool reversed )
{
    if( !m_isGood )
    {
        return false;
    }

    if( meshIx >= m_meshes.size() )
    {
        return false;
    }

    TriGeometryResMeshData* pMesh = m_meshes[meshIx].get();
    if( !pMesh )
    {
        return false;
    }

	if( m_forceLod )
	{
		if( !pMesh->m_lods.empty() && m_forcedLodIndex > 0 )
		{
			auto lod = pMesh->m_lods[std::min( m_forcedLodIndex, uint32_t( pMesh->m_lods.size() ) ) - 1];
			pMesh = m_meshLods[lod.meshIndex].get();
		}
	}
	else
	{
		for( auto lod : pMesh->m_lods )
		{
			if( lod.maxScreenSize < screenSize )
			{
				break;
			}
			pMesh = m_meshLods[lod.meshIndex].get();
		}
	}

    unsigned int primCount = GetPrimitiveCount( *pMesh, areaIx, areaCount );

	if( primCount )
	{
		if( reversed )
		{
			if( !pMesh->m_reversedIndicesValid )
			{
				return false;
			}
		}

		const TriGeometryResAreaData& area = pMesh->m_areas[areaIx];
		renderContext.m_esm.ApplyVertexDeclaration( pMesh->m_vertexDeclaration );
		renderContext.m_esm.ApplyStreamSource( 0, pMesh->m_vertexAllocation );

		auto& indices = reversed ? pMesh->m_reversedIndexAllocation : pMesh->m_indexAllocation;
		renderContext.m_esm.ApplyIndexBuffer( indices );

		renderContext.SetTopology( TOP_TRIANGLES );
		if( reversed )
		{
			renderContext.DrawIndexedPrimitive( pMesh->m_vertexCount, indices.GetStartIndex() + pMesh->m_primitiveCount * 3 - area.m_firstIndex - primCount * 3, primCount );
		}
		else
		{
			renderContext.DrawIndexedPrimitive( pMesh->m_vertexCount, indices.GetStartIndex() + area.m_firstIndex, primCount );
		}
	}

    return true;
}

bool TriGeometryRes::CreateMeshFromGrannyMesh( granny_mesh* myMesh, TriGeometryResMeshData* pMesh, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext, void* pVBOverride )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !Tr2Renderer::IsResourceCreationAllowed() || myMesh == NULL )
	{
		return false;
	}

	const int kVertexComponentMaxCount = 13;
	
	granny_data_type_definition* grannyVertexDecl = myMesh->PrimaryVertexData->VertexType;

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

		if( vertexCount > 65535 )
		{
			bytesPerIndex = 4;
		}
	}

	CR_RETURN_VAL( g_sharedBuffer.Allocate( bytesPerVertex, vertexCount, pSrc, renderContext, pMesh->m_vertexAllocation ), false );
	
	// create d3d index buffer, this one is shared, either for dynamic or static geometry
	int ibSize = indexCount * bytesPerIndex;

	{
		std::vector<uint8_t> tempBuffer( indexCount * bytesPerIndex );
		GrannyCopyMeshIndices( myMesh, bytesPerIndex, &tempBuffer[0] );

		

		USE_MAIN_THREAD_RENDER_CONTEXT();
		ALResult hr = g_sharedBuffer.Allocate( bytesPerIndex, indexCount, &tempBuffer[0], renderContext, pMesh->m_indexAllocation );
		if( FAILED( hr ) )
		{
			g_sharedBuffer.Free( pMesh->m_vertexAllocation );
			return false;	
		}

		if ( m_reversedIndexBuffersRequested )
		{
			if( bytesPerIndex == 2 )
			{
				std::reverse( reinterpret_cast<uint16_t*>( tempBuffer.data() ), reinterpret_cast<uint16_t*>( tempBuffer.data() + tempBuffer.size() ) );
			}
			else
			{
				std::reverse( reinterpret_cast<uint32_t*>( tempBuffer.data() ), reinterpret_cast<uint32_t*>( tempBuffer.data() + tempBuffer.size() ) );
			}

			if( FAILED( g_sharedBuffer.Allocate( bytesPerIndex, indexCount, tempBuffer.data(), renderContext, pMesh->m_reversedIndexAllocation ) ) )
			{
				g_sharedBuffer.Free( pMesh->m_vertexAllocation );
				g_sharedBuffer.Free( pMesh->m_indexAllocation );
				return false;
			}
			pMesh->m_reversedIndicesValid = true;
		}
	}


	pMesh->m_bytesPerVertex = bytesPerVertex;
	pMesh->m_vertexCount = vertexCount;

	pMesh->m_vertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( vertexDefinition );

	pMesh->m_allocationsValid = true;

	m_memoryUse += vbSize + ibSize; // Memory use is only approximate as a hint for the resource cache
	if( m_reversedIndexBuffersRequested )
	{
		m_memoryUse += ibSize;
	}
	
	return true;
}

bool TriGeometryRes::CreateMeshesFromGrannyFile( granny_file_info* gi, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto& mesh : m_meshes )
	{
		CreateMeshFromGrannyMesh( gi->Meshes[mesh->m_grannyMeshIndex], mesh.get(), cpuUsage, renderContext );
	}
	for( auto& mesh : m_meshLods )
	{
		CreateMeshFromGrannyMesh( gi->Meshes[mesh->m_grannyMeshIndex], mesh.get(), cpuUsage, renderContext );
	}
	CCP_STATS_ADD( geometryResBytes, m_memoryUse );

	return true;
}

Be::Result<std::string> TriGeometryRes::SaveMesh( const char* filename, uint32_t meshIndex ) const
{
	if( !IsGood() )
	{
		return Be::Result<std::string>( "Geometry is not prepared" );
	}
	auto meshData = GetMeshData( meshIndex );
	if( !meshData )
	{
		return Be::Result<std::string>( "Invalid mesh index" );
	}
	if( !SaveMeshToGrannyFile( meshData, filename ) )
	{
		return Be::Result<std::string>( "Failed to save the mesh" );
	}
	return Be::Result<std::string>();
}

bool TriGeometryRes::SaveMeshToGrannyFile( TriGeometryResMeshData* pMesh, const char* filename )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !pMesh || !pMesh->m_allocationsValid || pMesh->m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
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

	const void* pVertices;
	if( FAILED( pMesh->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { pMesh->m_vertexAllocation.UnmapForReading( renderContext ); } );

	const void* pIndices;
	if( FAILED( pMesh->m_indexAllocation.MapForReading( pIndices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { pMesh->m_indexAllocation.UnmapForReading( renderContext ); } );


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
			myMesh.BoneBindings[i].BoneName = pMesh->m_jointBindings[i].m_name.c_str();
			myMesh.BoneBindings[i].OBBMin[0] = pMesh->m_jointBindings[i].m_obbMin.x;
			myMesh.BoneBindings[i].OBBMin[1] = pMesh->m_jointBindings[i].m_obbMin.y;
			myMesh.BoneBindings[i].OBBMin[2] = pMesh->m_jointBindings[i].m_obbMin.z;
			myMesh.BoneBindings[i].OBBMax[0] = pMesh->m_jointBindings[i].m_obbMax.x;
			myMesh.BoneBindings[i].OBBMax[1] = pMesh->m_jointBindings[i].m_obbMax.y;
			myMesh.BoneBindings[i].OBBMax[2] = pMesh->m_jointBindings[i].m_obbMax.z;
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

ITr2InstanceData::InstanceData TriGeometryRes::GetInstanceData( unsigned int bufferIndex, float screenSize ) const
{
	auto mesh = GetMeshData( bufferIndex, screenSize );
	if( mesh )
	{
		return {
			mesh->m_vertexAllocation.GetBuffer(), mesh->m_vertexAllocation.GetOffset(), mesh->m_bytesPerVertex, mesh->m_vertexCount
		};
	}
	static Tr2BufferAL nullVB;
	return { nullVB };
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
	if( bufferIndex < m_meshes.size() && m_meshes[bufferIndex] )
	{
		return m_meshes[bufferIndex]->m_vertexDeclaration;
	}
	return Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

CcpMath::AxisAlignedBox TriGeometryRes::GetInstanceBufferBoundingBox( unsigned int bufferIndex ) const
{
	auto mesh = GetMeshData( bufferIndex );
	if( !mesh )
	{
		return CcpMath::AxisAlignedBox();
	}
	return CcpMath::AxisAlignedBox( mesh->m_minBounds, mesh->m_maxBounds );
}

Be::Result<std::string> TriGeometryRes::GetModelName( unsigned int ix, std::string& name ) const
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

Be::Result<std::string> TriGeometryRes::GetMeshName( unsigned int ix, std::string& name ) const
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

Be::Result<std::string> TriGeometryRes::GetMeshAreaCount( unsigned int ix, int& count ) const
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

Be::Result<std::string> TriGeometryRes::GetMeshAreaName( unsigned int meshIx, unsigned int areaIx, std::string& name ) const
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

Be::Result<std::string> TriGeometryRes::GetBoundingSphereFromScript( unsigned int meshIx, std::pair<Vector3, float>& bounds ) const
{
	if( meshIx >= m_meshes.size() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}
	auto& pMesh = m_meshes[meshIx];
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
	auto& pMesh = m_meshes[meshIx];
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


BlueStdResult TriGeometryRes::GetMeshVertexElements( size_t meshIndex, std::vector<std::pair<uint32_t, uint32_t>>& elements ) const
{
	elements.clear();

	if( !IsGood() )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "TriGeometryRes is not prepared" );
	}
	if( meshIndex >= m_meshes.size() || !m_meshes[meshIndex] )
	{
		return BlueStdResult( BLUE_STD_RESULT_INDEX_ERROR, "mesh index out of range" );
	}

	auto& mesh = m_meshes[meshIndex];
	Tr2VertexDefinition decl;
	if ( !Tr2EffectStateManager::GetVertexDeclarationElements( mesh->m_vertexDeclaration, decl ) )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "could not retrieve vertex declaration" );
	}

	for( auto it = std::begin( decl.m_items ); it != std::end( decl.m_items ); ++it )
	{
		elements.push_back( std::make_pair( it->m_usage, it->m_usageIndex ) );
	}
	return BLUE_STD_RESULT_OK;
}
