// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Include/TriMath.h"
#include "TriGeometryRes.h"
#include "TriGrannyRes.h"
#include "Tr2PerObjectData.h"
#include "Tr2VertexDefinitionUtilities.h"
#include "TriDevice.h"

#include "TriSettingsRegistrar.h"

#include "Utilities/GeometryUtils.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/BoundingBox.h"

#include <cctype>
#include "TbbStub.h"

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARED_ELSEWHERE( geometryResBytes );

int g_grannyDeprecationLevel = (int)GrannyDeprecationLevel::LOG_ERROR_AND_ASSERT;
TRI_REGISTER_SETTING( "grannyDeprecationLevel", g_grannyDeprecationLevel );


void HandleGrannyDeprecation( const std::wstring& path )
{
	switch( (GrannyDeprecationLevel)g_grannyDeprecationLevel )
	{
	case GrannyDeprecationLevel::DO_NOTHING:
		break;
	case GrannyDeprecationLevel::LOG_ERROR: {
		CCP_LOGERR( "Don't load granny! Use CMF! Tried to load: %ls", path.c_str() );
	}
	break;
	case GrannyDeprecationLevel::LOG_ERROR_AND_ASSERT: {
		CCP_LOGERR( "Don't load granny! Use CMF! Tried to load: %ls", path.c_str() );
		CCP_ASSERT_M( false, "Use CMF!" );
	}
	break;
	default: {
		CCP_ASSERT_M( false, "Invalid GrannyDeprecationLevel" );
	}
	break;
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

void Tr2RaycastGeometryRes::SetLodIndices( std::vector<int32_t>&& lodIndices )
{
	m_lodIndices = std::move( lodIndices );
}

BVH::BoundingVolumeHierarchy& Tr2RaycastGeometryRes::GetBVH()
{
	return m_bvh;
}

BlueAsyncRes::LoadingResult Tr2RaycastGeometryRes::DoLoad()
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	if( !m_dataStream )
	{
		return LR_FAILED;
	}

	CCP_ASSERT( m_path.size() >= 4 && m_path.compare( m_path.size() - 4, 4, L".cmf" ) == 0 );

	auto cmfContent = Tr2CmfContents( *m_dataStream, CW2A( m_path.c_str() ) );
	if( !cmfContent )
	{
		return LR_FAILED;
	}

	auto cmfData = cmfContent.GetData();
	if( !cmfData )
	{
		return LR_FAILED;
	}

	if( m_lodIndices.size() != cmfData->meshes.size() )
	{
		return LR_FAILED;
	}

	for( size_t i = 0; i < cmfData->meshes.size(); i++ )
	{
		if( m_lodIndices[i] >= cmfData->meshes[i].lods.size() )
		{
			return LR_FAILED;
		}
	}

	// BVH creation also decompresses the cmf buffers, which allows safe multithreaded raycast queries.
	m_bvh = BVH::BoundingVolumeHierarchy( std::move( cmfContent ), m_lodIndices );

	return LR_SUCCESS;
}

bool Tr2RaycastGeometryRes::DoPrepare()
{
	return true;
}


namespace
{
struct MorphToBaseData
{
	unsigned int bytesPerVertex;
	Tr2VertexDefinition::Item* foundPosition;
	unsigned int positionByteOffset;
	Tr2VertexDefinition::DataType declType;
};

MorphToBaseData InitMorphToBaseData( Tr2VertexDefinition decl )
{
	MorphToBaseData data{};
	data.bytesPerVertex = decl.m_nextOffset[0];
	data.foundPosition = decl.Find( Tr2VertexDefinition::POSITION, 0 );
	CCP_ASSERT_M( data.foundPosition, "InitMorphToBaseData: Couldn't find Tr2VertexDefinition::POSITION." );
	data.positionByteOffset = data.foundPosition->m_offset;
	data.declType = data.foundPosition->m_dataType;
	return data;
}

float SquaredDistMorphToBase( int index, bool dataIsDeltas, uint8_t* pVertices, MorphToBaseData baseData, uint8_t* pMorphSrc, MorphToBaseData morphData )
{
	Vector3 vertex;
	ConvertDataToVector3( baseData.declType, pVertices + baseData.positionByteOffset + index * baseData.bytesPerVertex, &vertex );

	Vector3 morphVertex;
	ConvertDataToVector3( morphData.declType, pMorphSrc + morphData.positionByteOffset + index * morphData.bytesPerVertex, &morphVertex );

	Vector3 diff;
	if( dataIsDeltas )
	{
		diff = morphVertex;
	}
	else
	{
		diff = morphVertex - vertex;
	}

	return Dot( diff, diff );
}
}


TriGeometryRes::TriGeometryRes( IRoot* lockobj ) :
#if WITH_GRANNY
	m_pGrannyFile( NULL ),
#endif
	m_useCMF( true )
{
}

TriGeometryRes::~TriGeometryRes()
{
	DestroyRayCaster();
	ReleaseResources( TRISTORAGE_ALL );
#if WITH_GRANNY
	ClearGrannyData();
#endif
	ClearCMFData();
}

void TriGeometryRes::ClearCMFData()
{
	m_cmfContents = {};
}

#if WITH_GRANNY
void TriGeometryRes::ClearGrannyData()
{
	m_meshes.clear();
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
	else
	{
		return nullptr;
	}
}
#endif

unsigned int TriGeometryRes::GetAnimationCount() const
{
	if( IsUsingCMF() )
	{
		if( IsGood() && m_cmfContents )
		{
			return (int)m_cmfContents.GetData()->animations.size();
		}
	}
#if WITH_GRANNY
	else
	{
		if( auto info = GetGrannyInfo() )
		{
			return info->AnimationCount;
		}
	}
#endif
	return 0;
}

#if WITH_GRANNY
int TriGeometryRes::GetVertexComponentOffset( const granny_mesh* grannyMesh, const char* componentName ) const
{
	CCP_ASSERT_M( !m_useCMF, "This should never be called on a cmf codepath!" );

	// now scan granny's vertex-declaration for the component's part and count the offsets
	granny_data_type_definition* vertexFormat = grannyMesh->PrimaryVertexData->VertexType;
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
#endif

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
	if( s & TRISTORAGE_MANAGEDMEMORY )
	{
		CancelPendingLoad();

		DestroyRayCaster();

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
	TRINITY_STATS_ZONE( __FUNCTION__ );

	if( m_cmfContents )
	{
		if( !m_cmfContents.GetData() || !SetupMeshes( *m_cmfContents.GetData() ) )
		{
			return LR_FAILED;
		};
		return LR_SUCCESS;
	}
#if WITH_GRANNY
	else if( m_pGrannyFile )
	{
		// The granny file already exists so we only have to recreate
		// the meshes.
		SetupMeshes( GrannyGetFileInfo( m_pGrannyFile ) );
		return LR_SUCCESS;
	}
#endif

	if( !m_dataStream )
	{
		return LR_FAILED;
	}

	if( m_path.size() >= 4 && m_path.compare( m_path.size() - 4, 4, L".cmf" ) == 0 )
	{
		if( ReadCMFFile() )
		{
			// Decompress sections we will use in Prepare
			for( uint32_t i = 0; i < m_meshes.size(); i++ )
			{
				auto& mesh = m_meshes[i];
				auto& cmfMesh = m_cmfContents.GetData()->meshes[i];

				for( const auto& lod : mesh->m_lods )
				{
					auto& cmfLod = cmfMesh.lods[lod->m_originalLodIndex];
					m_cmfContents.GetViewData( cmfLod.vb );
					m_cmfContents.GetViewData( cmfLod.ib );
					for( const auto& morph : cmfLod.morphTargets )
					{
						m_cmfContents.GetViewData( morph.vb );
					}
				}
			}

			return LR_SUCCESS;
		}
	}
	else
	{
		HandleGrannyDeprecation( m_path );
#if WITH_GRANNY
		if( ReadGrannyFile() )
		{
			return LR_SUCCESS;
		}
#endif
	}

	return LR_FAILED;
}

// This gets called on the main thread
bool TriGeometryRes::DoPrepare()
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	if( Tr2Renderer::IsGeometryLoadDisabled() )
	{
		return false;
	}

	if( IsUsingCMF() )
	{
		if( m_cmfContents )
		{
			CreateMeshesFromCMFFile( m_cmfContents, Tr2CpuUsage::READ, renderContext );
			const auto* cmfData = m_cmfContents.GetData();
			if( cmfData && cmfData->animations.empty() )
			{
				const auto hasMeshBindings = std::any_of( cmfData->meshes.begin(), cmfData->meshes.end(), []( const cmf::Mesh& mesh ) {
					return !mesh.boneBindings.empty();
				} );
				if( !hasMeshBindings )
				{
					m_cmfContents = {};
				}
			}
		}
	}
#if WITH_GRANNY
	else if( m_pGrannyFile )
	{
		CCP_ASSERT_M( !m_useCMF, "TriGeometryRes::DoPrepare: intern messed up." );

		granny_file_info* gi = GrannyGetFileInfo( m_pGrannyFile );

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
#endif

	return true;
}

#if WITH_GRANNY
void TriGeometryRes::DetermineAreaBoundsAndVertCount( TriGeometryResAreaData& area, granny_mesh* grannyMesh, int bytesPerVertex )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	// Determine bounds for area
	// Assume that position is the first component

	// set up some data for the vertex fetcher.
	unsigned int positionOffset;
	Tr2VertexDefinition::DataType positionType;
	GetVertexPositionOffsetAndType( grannyMesh, positionOffset, positionType );

	BoundingBoxInitialize( area.m_minBounds, area.m_maxBounds );

	std::set<int32_t> vertexIndicesSeen;

	for( int vIx = 0; vIx < area.m_primitiveCount * 3; ++vIx )
	{
		int index;

		if( grannyMesh->PrimaryTopology->Indices16 )
		{
			index = grannyMesh->PrimaryTopology->Indices16[vIx + area.m_firstIndex];
		}
		else
		{
			index = grannyMesh->PrimaryTopology->Indices[vIx + area.m_firstIndex];
		}

		vertexIndicesSeen.insert( index );

		// vertices are NOT GUARENTEED to be float3 or fp16_4, so this helps with that.
		Vector3 vertexPosition;
		GetMeshVertexPosition( grannyMesh, index, vertexPosition, (unsigned int)bytesPerVertex, positionOffset, positionType );

		BoundingBoxUpdate( area.m_minBounds, area.m_maxBounds, vertexPosition );
	}
}
#endif

#if WITH_GRANNY
bool TriGeometryRes::IsAreaSkinned( TriGeometryResAreaData& area, granny_mesh* grannyMesh, granny_file_info* gi, int bytesPerVertex )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );
	// offset to boneindex
	int boneIndexOffset = GetVertexComponentOffset( grannyMesh, GrannyVertexBoneIndicesName );
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
		if( rootBone )
		{
			for( int32_t i = 0; i < grannyMesh->BoneBindingCount; ++i )
			{
				if( grannyMesh->BoneBindings[i].BoneName && strcmp( grannyMesh->BoneBindings[i].BoneName, rootBone ) == 0 )
				{
					return uint8_t( i );
				}
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
	uint8_t* pBoneIndex0 = (uint8_t*)grannyMesh->PrimaryVertexData->Vertices + boneIndexOffset + 0;
	uint8_t* pBoneIndex1 = (uint8_t*)grannyMesh->PrimaryVertexData->Vertices + boneIndexOffset + 1;
	uint8_t* pBoneIndex2 = (uint8_t*)grannyMesh->PrimaryVertexData->Vertices + boneIndexOffset + 2;
	uint8_t* pBoneIndex3 = (uint8_t*)grannyMesh->PrimaryVertexData->Vertices + boneIndexOffset + 3;

	// cycle thorugh all vertices of this area and collect bone indices
	for( int vIx = 0; vIx < area.m_primitiveCount * 3; ++vIx )
	{
		int index;

		if( grannyMesh->PrimaryTopology->Indices16 )
		{
			index = grannyMesh->PrimaryTopology->Indices16[vIx + area.m_firstIndex];
		}
		else
		{
			index = grannyMesh->PrimaryTopology->Indices[vIx + area.m_firstIndex];
		}

		// bones
		uint8_t boneIndex0 = *( pBoneIndex0 + index * bytesPerVertex );
		uint8_t boneIndex1 = *( pBoneIndex1 + index * bytesPerVertex );
		uint8_t boneIndex2 = *( pBoneIndex2 + index * bytesPerVertex );
		uint8_t boneIndex3 = *( pBoneIndex3 + index * bytesPerVertex );

		if( boneIndex0 != *rootBoneIndex || boneIndex1 != *rootBoneIndex || boneIndex2 != *rootBoneIndex || boneIndex3 != *rootBoneIndex )
		{
			// found it attached to a bone so return early
			return true;
		}
	}
	// if we reach this point, then no vertices have any valid bone indices
	return false;
}

bool TriGeometryRes::IsAreaMorphed( TriGeometryResAreaData& area, granny_mesh* myMesh, granny_file_info* gi )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	if( myMesh->MorphTargetCount == 0 )
	{
		return false;
	}

	const float EPSILON = .001f;

	// primary mesh vertex declaration
	granny_data_type_definition* grannyVertexDecl = myMesh->PrimaryVertexData->VertexType;
	Tr2VertexDefinition vertexDefinition = BuildFromGrannyVertexDecl( grannyVertexDecl );

	// morph target vertex declaration
	granny_data_type_definition* grannyMorphVertexDecl = myMesh->MorphTargets[0].VertexData->VertexType;
	Tr2VertexDefinition vertexMorphDefinition = BuildFromGrannyVertexDecl( grannyMorphVertexDecl );

	MorphToBaseData baseData = InitMorphToBaseData( vertexDefinition );
	MorphToBaseData morphData = InitMorphToBaseData( vertexMorphDefinition );

	// let's try to find at least one vertex that is being affected by at least one morph target
	bool dataIsDeltas = myMesh->MorphTargets->DataIsDeltas;
	auto pVertices = (uint8_t*)GrannyGetMeshVertices( myMesh );

	for( int i = 0; i < myMesh->MorphTargetCount; ++i )
	{
		auto pMorphSrc = (uint8_t*)GrannyGetMeshMorphVertices( myMesh, i );

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

			float squaredDist = SquaredDistMorphToBase( index, dataIsDeltas, pVertices, baseData, pMorphSrc, morphData );
			if( squaredDist > EPSILON )
			{
				return true;
			}
		}
	}

	return false;
}
#endif

#if WITH_GRANNY
bool TriGeometryRes::SetupMeshes( granny_file_info* gi )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT_M( !m_useCMF, "TriGeometryRes::SetupMeshes: intern messed up cmf loading" );

	//Used by LOD meshes to find the model they belong to.
	std::vector<TriGeometryResMeshData*> grannyMeshIndexToMeshMap;
	grannyMeshIndexToMeshMap.resize( gi->MeshCount );

	for( int32_t meshIndex = 0; meshIndex < gi->MeshCount; ++meshIndex )
	{

		granny_mesh* grannyMesh = gi->Meshes[meshIndex];

		if( grannyMesh == NULL || !grannyMesh->PrimaryVertexData || !grannyMesh->PrimaryTopology )
		{
			CCP_LOGWARN( "Trying to load geometry with no primary vertex data and/or topology" );
			return false;
		}

		granny_data_type_definition* grannyVertexDecl = grannyMesh->PrimaryVertexData->VertexType;
		Tr2VertexDefinition vertexDefinition = BuildFromGrannyVertexDecl( grannyVertexDecl );
		const uint32_t vertexDeclarationHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( vertexDefinition );
		unsigned int bytesPerVertex = vertexDefinition.m_nextOffset[0];

		MeshBoundsInfo* mbi = NULL;

		// Look for mesh bounds info in the Granny file
		if( grannyMesh->ExtendedData.Object )
		{
			// The mesh has extended data - ask Granny to convert it to our current version of the bounds
			// info data structure.
			mbi = static_cast<MeshBoundsInfo*>( GrannyConvertTree( grannyMesh->ExtendedData.Type, grannyMesh->ExtendedData.Object, MeshBoundsInfoType, nullptr, nullptr ) );
			if( !mbi->typeName || ( strcmp( mbi->typeName, "MeshBoundsInfo" ) != 0 ) )
			{
				// This extended data doesn't match our expectations
				GrannyFreeBuilderResult( (void*)mbi );
				mbi = NULL;
			}
		}

		//First we need to find the mesh for this LOD.

		TriGeometryResMeshData* mesh = NULL;
		float maxScreenSize = std::numeric_limits<float>::infinity();

		const auto mainLod = !mbi || mbi->maxScreenSize <= 0;

		if( !mainLod )
		{
			//We are an LOD of an existing mesh. Add ourselves to it.
			int32_t sourceMeshIndex = mbi->sourceMeshIndex;

			//We assume the list is sorted so that we don't reference previous meshes.
			CCP_ASSERT( sourceMeshIndex < meshIndex );

			mesh = grannyMeshIndexToMeshMap[sourceMeshIndex];

			if( mesh->m_vertexDeclarationHandle != vertexDeclarationHandle || mesh->m_bytesPerVertex != bytesPerVertex )
			{
				if( mbi )
					GrannyFreeBuilderResult( (void*)mbi );
				continue;
			}

			maxScreenSize = float( mbi->maxScreenSize );
		}
		else
		{
			//We are the main LOD of a mesh, so we need to create a new mesh.

			mesh = new TriGeometryResMeshData();

			if( mesh == nullptr )
			{
				CCP_LOGWARN( "Out of memory in TriGeometryRes::SetupModels" );
				return false;
			}

			CopyGrannyName( mesh->m_name, grannyMesh->Name );

			//Initialize the bounding box so that we can include the various meshes and LODs into it.
			BoundingBoxInitialize( mesh->m_minBounds, mesh->m_maxBounds );

			//The bounding sphere is computed later, so just initialize it for now.
			mesh->m_boundingSphere = Vector4( 0.f, 0.f, 0.f, 0.f );

			mesh->m_vertexDeclarationHandle = vertexDeclarationHandle;
			mesh->m_bytesPerVertex = bytesPerVertex;


			for( int jointIndex = 0; jointIndex < grannyMesh->BoneBindingCount; ++jointIndex )
			{
				TriJointBinding binding;
				binding.m_name = grannyMesh->BoneBindings[jointIndex].BoneName;
				binding.m_obbMin = *reinterpret_cast<const Vector3*>( grannyMesh->BoneBindings[jointIndex].OBBMin );
				binding.m_obbMax = *reinterpret_cast<const Vector3*>( grannyMesh->BoneBindings[jointIndex].OBBMax );
				mesh->m_jointBindings.push_back( binding );
			}

			m_meshes.push_back( std::unique_ptr<TriGeometryResMeshData>( mesh ) );
		}

		//When we load a LOD mesh, it references the granny mesh index which is the "parent" mesh.
		//Therefore, we update the mapping so that future meshes can reference this mesh.
		grannyMeshIndexToMeshMap[meshIndex] = mesh;



		//Setup a new LOD, which will be added as an LOD of the mesh.

		std::unique_ptr<TriGeometryResLodData> lod( new TriGeometryResLodData() );
		if( lod == nullptr )
		{
			CCP_LOGWARN( "Out of memory in TriGeometryRes::SetupModels" );
			return false;
		}

		lod->m_mesh = mesh;

		lod->m_grannyMeshIndex = meshIndex;

		CopyGrannyName( lod->m_name, grannyMesh->Name );

		lod->m_maxScreenSize = maxScreenSize;

		lod->m_vertexCount = grannyMesh->PrimaryVertexData->VertexCount;
		lod->m_primitiveCount = 0; //Will be summed up by looping over areas.

		if( mbi )
		{
			//Get the UV densities if available, which are used for texture streaming.
			if( mbi->uvDensityCount > 0 && mbi->uvDensities )
			{
				lod->m_uvDensities.assign( mbi->uvDensities, mbi->uvDensities + mbi->uvDensityCount );
			}

			if( mainLod )
			{
				Vector3 meshMin = *reinterpret_cast<Vector3*>( &mbi->bounds.min[0] );
				Vector3 meshMax = *reinterpret_cast<Vector3*>( &mbi->bounds.max[0] );
				BoundingBoxUpdate( mesh->m_minBounds, mesh->m_maxBounds, meshMin, meshMax );
			}
		}



		lod->m_areas.resize( grannyMesh->PrimaryTopology->GroupCount );

		for( int groupIndex = 0; groupIndex < grannyMesh->PrimaryTopology->GroupCount; ++groupIndex )
		{
			const granny_tri_material_group& grp = grannyMesh->PrimaryTopology->Groups[groupIndex];

			TriGeometryResAreaData& area = lod->m_areas[groupIndex];
			area.m_name = "";

			if( grannyMesh->MaterialBindingCount > grp.MaterialIndex )
			{
				if( grannyMesh->MaterialBindings[grp.MaterialIndex].Material != NULL )
				{
					const granny_material_binding& mb = grannyMesh->MaterialBindings[grp.MaterialIndex];
					CopyGrannyName( area.m_name, mb.Material->Name );
				}
			}

			area.m_firstIndex = grannyMesh->PrimaryTopology->Groups[groupIndex].TriFirst * 3;
			area.m_primitiveCount = grannyMesh->PrimaryTopology->Groups[groupIndex].TriCount;

			lod->m_primitiveCount += area.m_primitiveCount;

			area.m_isSkinned = IsAreaSkinned( area, grannyMesh, gi, bytesPerVertex );
			area.m_isMorphed = IsAreaMorphed( area, grannyMesh, gi );

			if( mbi )
			{
				CCP_ASSERT( groupIndex < mbi->areaCount );
				area.m_minBounds = *reinterpret_cast<Vector3*>( &mbi->areaInfos[groupIndex].bounds.min[0] );
				area.m_maxBounds = *reinterpret_cast<Vector3*>( &mbi->areaInfos[groupIndex].bounds.max[0] );
			}
			else
			{
				DetermineAreaBoundsAndVertCount( area, grannyMesh, bytesPerVertex );

				// if the area doesn't have any verts, the bounding box is invalid, so DON't use it!
				if( mainLod && area.m_primitiveCount )
				{
					BoundingBoxUpdate( mesh->m_minBounds, mesh->m_maxBounds, area.m_minBounds, area.m_maxBounds );
				}
			}
		}

		mesh->m_lods.push_back( std::move( lod ) );

		if( mbi )
		{
			GrannyFreeBuilderResult( (void*)mbi );
		}
	}

	int minimumLOD = gTriDev->GetMinimumModelLOD();

	for( auto& mesh : m_meshes )
	{
		sort( begin( mesh->m_lods ), end( mesh->m_lods ), []( auto& a, auto& b ) { return a->m_maxScreenSize > b->m_maxScreenSize; } );

		for( size_t i = 0; i < mesh->m_lods.size(); i++ )
		{
			mesh->m_lods[i]->m_originalLodIndex = (int32_t)i;
		}

		if( minimumLOD > 0 )
		{
			size_t toRemove = min( (size_t)minimumLOD, mesh->m_lods.size() - 1 );
			if( toRemove > 0 )
			{
				mesh->m_lods.erase( mesh->m_lods.begin(), mesh->m_lods.begin() + toRemove );
			}
		}


		uint32_t lodMask = 0u;
		for( size_t i = 0; i < mesh->m_lods.size(); i++ )
		{
			lodMask |= 1u << mesh->m_lods[i]->m_originalLodIndex;
		}

		mesh->m_lodMask = lodMask;
	}

	return true;
}
#endif

#if WITH_GRANNY
void TriGeometryRes::SetupSkeletons( granny_file_info* gi )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT_M( !m_useCMF, "TriGeometryRes::SetupSkeletons: intern messed up cmf loading" );

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
#endif

bool TriGeometryRes::ReadCMFFile()
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	m_useCMF = true;
	m_cmfContents = Tr2CmfContents( *m_dataStream, CW2A( m_path.c_str() ) );
	if( !m_cmfContents || !m_cmfContents.GetData() )
	{
		return false;
	}
	if( !SetupMeshes( *m_cmfContents.GetData() ) )
	{
		return false;
	}

	SetupSkeletons( *m_cmfContents.GetData() );

	return true;
}

#if WITH_GRANNY
bool TriGeometryRes::ReadGrannyFile()
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	m_useCMF = false;

	void* data;
	if( !m_dataStream->LockData( &data, 0 ) )
	{
		return false;
	}

	m_pGrannyFile = ProtectedGrannyReadEntireFileFromMemory( m_path.c_str(), uint32_t( m_dataStream->GetSize() ), data );

	if( !m_pGrannyFile )
	{
		return false;
	}

	granny_file_info* gi = GrannyGetFileInfo( m_pGrannyFile );

	// BeResMan->AddGeometryDataRead( m_dataSize );

	// m_pGrannyFile is freed in CreateD3DMeshFromGranny once the D3D
	// resources have been created on the main thread.

	if( !SetupMeshes( gi ) )
	{
		return false;
	}

	SetupSkeletons( gi );

	return true;
}
#endif

void TriGeometryRes::PrepareFromGrannyRes( TriGrannyRes* g )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( g->IsUsingCMF() )
	{
		const cmf::Data* cmfData = g->GetCMFData();

		if( !cmfData )
		{
			return;
		}

		m_useCMF = true;

		SetupMeshes( *cmfData );
		SetupSkeletons( *cmfData );
		CreateMeshesFromCMFFile( g->GetCMFContents(), Tr2CpuUsage::READ | Tr2CpuUsage::WRITE, renderContext );
	}
#if WITH_GRANNY
	else
	{
		granny_file* f = g->GetGrannyFile();
		if( !f )
		{
			return;
		}
		granny_file_info* gi = GrannyGetFileInfo( f );

		m_useCMF = false;

		SetupMeshes( gi );
		SetupSkeletons( gi );
		CreateMeshesFromGrannyFile( gi, Tr2CpuUsage::READ | Tr2CpuUsage::WRITE, renderContext );
	}
#endif

	m_sourceGranny = g;

	SetPrepared( true );
	SetGood( true );
}

void TriGeometryRes::PrepareRayCaster()
{
	m_bvh.sessions++;

	if( m_bvh.geometry || !m_useCMF )
	{
		return;
	}

	std::vector<int32_t> lodIndices;
	lodIndices.resize( m_meshes.size() );
	for( size_t i = 0; i < m_meshes.size(); i++ )
	{
		lodIndices[i] = m_meshes[i] ? m_meshes[i]->m_lods[0]->m_originalLodIndex : 0;
	}

	m_bvh.geometry.CreateInstance();
	m_bvh.geometry->SetLodIndices( std::move( lodIndices ) );
	m_bvh.geometry->Initialize( GetFilePath().c_str(), GetExt() );
}

void TriGeometryRes::DestroyRayCaster()
{
	if( m_bvh.geometry )
	{
		m_bvh.geometry->CancelPendingLoad();
		m_bvh.geometry.Unlock();
	}
}

void TriGeometryRes::ResetRayCaster()
{
	m_bvh.sessions--;

	CCP_ASSERT( m_bvh.sessions >= 0 );

	if( m_bvh.sessions > 0 )
	{
		return;
	}

	m_bvh.geometry.Unlock();
}

bool TriGeometryRes::IsRayCasterReady() const
{
	return m_bvh.geometry && m_bvh.geometry->IsGood();
}

bool TriGeometryRes::HasRayCasterPreparationFailed() const
{
	if( m_bvh.sessions > 0 && !m_bvh.geometry ) // ReleaseResources destroyed our bvh :(
	{
		return true;
	}
	return m_bvh.geometry && m_bvh.geometry->IsPrepared() && !m_bvh.geometry->IsGood();
}

static std::vector<BVH::IntersectedNode>& GetRaycastStack()
{
	static Tr2EnumerableThreadSpecific<std::vector<BVH::IntersectedNode>> stacks;
	return stacks.local();
}

bool TriGeometryRes::GetIntersectionPoints( const Vector3& pos, const Vector3& dir, RayCastResult& result, uint32_t areaIx, float rayLength )
{
	if( !m_useCMF || !m_bvh.geometry || !m_bvh.geometry->IsGood() )
	{
		if( BeResMan->IsOnMainThread() )
		{
			return GetIntersectionPointsLegacy( pos, dir, result, areaIx, rayLength );
		}
		CCP_ASSERT_M( false, "Raycast fallback is not available in a multithreaded environment! You need to wait for IsRayCasterReady before raycasting!" );
		return false;
	}

	BVH::RayHit rayHit;
	bool hit = false;
	if( areaIx != -1 )
	{
		hit = m_bvh.geometry->GetBVH().IntersectAreaAcrossMeshes( GetRaycastStack(), CcpMath::Ray{ pos, dir }, rayLength, areaIx, rayHit );
	}
	else
	{
		hit = m_bvh.geometry->GetBVH().IntersectAll( GetRaycastStack(), CcpMath::Ray{ pos, dir }, rayLength, rayHit );
	}

	if( !hit )
	{
		return false;
	}

	result.distance = rayHit.distance;
	result.position = pos + dir * rayHit.distance;

	auto& bvh = m_bvh.geometry->GetBVH();
	auto indices = bvh.GetIndices( rayHit.meshIndex );
	auto positions = bvh.GetPositions( rayHit.meshIndex );
	auto bones = bvh.GetBones( rayHit.meshIndex );
	auto colors = bvh.GetColors( rayHit.meshIndex );

	uint32_t index1 = indices[rayHit.primitive * 3 + 0];
	uint32_t index2 = indices[rayHit.primitive * 3 + 1];
	uint32_t index3 = indices[rayHit.primitive * 3 + 2];

	Vector3 p1 = positions[index1];
	Vector3 p2 = positions[index2];
	Vector3 p3 = positions[index3];

	Vector3 avec = p2 - p1;
	Vector3 bvec = p3 - p1;
	result.unnormalizedNormal = Cross( avec, bvec );

	if( bones.has_value() )
	{
		result.boneIndex = bones.value()[index1][0];
	}
	if( colors.has_value() )
	{
		float v1 = 1.0f - ( rayHit.u + rayHit.v );
		Color color1 = colors.value()[index1];
		Color color2 = colors.value()[index2];
		Color color3 = colors.value()[index3];
		result.firstVertexColor = color1;
		result.interpolatedVertexColor = color1 * v1 + color2 * rayHit.u + color3 * rayHit.v;
	}

	return true;
}

void TriGeometryRes::Initialize( const wchar_t* name, const wchar_t* ext )
{
	ReleaseResources( TRISTORAGE_ALL );
	BlueAsyncRes::Initialize( name, ext );
}

void TriGeometryRes::Reload()
{
	CancelPendingLoad();
	NotifyReleaseCachedData();
#if WITH_GRANNY
	ClearGrannyData();
#endif
	ClearCMFData();
	BlueAsyncRes::Reload();
}

#if WITH_GRANNY
float CalculateMorphDeformationAmount( bool dataIsDeltas, int32_t vertexCount, uint8_t* pMorphSrc, Tr2VertexDefinition morphDecl, uint8_t* pVertices, Tr2VertexDefinition decl )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	MorphToBaseData baseData = InitMorphToBaseData( decl );
	MorphToBaseData morphData = InitMorphToBaseData( morphDecl );

	float maxAmount = 0.f;
	for( int j = 0; j < vertexCount; j++ )
	{
		float squaredDist = SquaredDistMorphToBase( j, dataIsDeltas, pVertices, baseData, pMorphSrc, morphData );
		maxAmount = max( maxAmount, squaredDist );
	}

	return sqrt( maxAmount );
}
#endif

#if WITH_GRANNY
bool TriGeometryRes::CreateLodFromGrannyMesh( granny_mesh* grannyMesh, TriGeometryResLodData* lod, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext, void* pVBOverride )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	if( !Tr2Renderer::IsResourceCreationAllowed() || grannyMesh == NULL )
	{
		return false;
	}

	const int kVertexComponentMaxCount = 13;

	granny_data_type_definition* grannyVertexDecl = grannyMesh->PrimaryVertexData->VertexType;

	Tr2VertexDefinition vertexDefinition = BuildFromGrannyVertexDecl( grannyVertexDecl );
	const unsigned bytesPerVertex = vertexDefinition.m_nextOffset[0];

	unsigned bytesPerIndex = 2;

	int vertexCount = grannyMesh->PrimaryVertexData->VertexCount;
	int vbSize = vertexCount * bytesPerVertex;

	void* pSrc = pVBOverride;
	if( !pSrc )
	{
		pSrc = GrannyGetMeshVertices( grannyMesh );
	}

	int indexCount = grannyMesh->PrimaryTopology->Index16Count;
	if( indexCount == 0 )
	{
		indexCount = grannyMesh->PrimaryTopology->IndexCount;

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

	CR_RETURN_VAL( g_sharedBuffer.Allocate( bytesPerVertex, vertexCount, pSrc, renderContext, lod->m_vertexAllocation ), false );

	// create d3d index buffer, this one is shared, either for dynamic or static geometry
	int ibSize = indexCount * bytesPerIndex;

	{ // Index Buffer
		std::vector<uint8_t> tempBuffer( indexCount * bytesPerIndex );
		GrannyCopyMeshIndices( grannyMesh, bytesPerIndex, &tempBuffer[0] );

		USE_MAIN_THREAD_RENDER_CONTEXT();
		ALResult hr = g_sharedBuffer.Allocate( bytesPerIndex, indexCount, &tempBuffer[0], renderContext, lod->m_indexAllocation );
		if( FAILED( hr ) )
		{
			g_sharedBuffer.Free( lod->m_vertexAllocation );
			return false;
		}

		if( bytesPerIndex == 2 )
		{
			std::reverse( reinterpret_cast<uint16_t*>( tempBuffer.data() ), reinterpret_cast<uint16_t*>( tempBuffer.data() + tempBuffer.size() ) );
		}
		else
		{
			std::reverse( reinterpret_cast<uint32_t*>( tempBuffer.data() ), reinterpret_cast<uint32_t*>( tempBuffer.data() + tempBuffer.size() ) );
		}

		if( FAILED( g_sharedBuffer.Allocate( bytesPerIndex, indexCount, tempBuffer.data(), renderContext, lod->m_reversedIndexAllocation ) ) )
		{
			g_sharedBuffer.Free( lod->m_vertexAllocation );
			g_sharedBuffer.Free( lod->m_indexAllocation );
			return false;
		}
		lod->m_reversedIndicesValid = true;
	}

	lod->m_morphVertexDeclaration = -1;
	{ // Morph Target
		if( grannyMesh->MorphTargetCount > 0 )
		{
			// Allocate the morph target array based on the size of the first morph target
			const granny_morph_target& firstMorphTarget = grannyMesh->MorphTargets[0];
			Tr2VertexDefinition firstMorphTargetVertexDefinition = BuildFromGrannyVertexDecl( firstMorphTarget.VertexData->VertexType );
			lod->m_bytesPerMorphTargetVertex = firstMorphTargetVertexDefinition.m_nextOffset[0];
			uint32_t morphDataSize = firstMorphTarget.VertexData->VertexCount * lod->m_bytesPerMorphTargetVertex;
			uint32_t morphTargetBufferSize = ( morphDataSize * grannyMesh->MorphTargetCount + sizeof( TriMorphTargetGeometryConstants ) );

			CR_RETURN_VAL( g_sharedBuffer.Allocate(
							   4,
							   morphTargetBufferSize >> 2,
							   nullptr,
							   renderContext,
							   lod->m_morphTargetAllocation ),
						   false );

			TriMorphTargetGeometryConstants morphTargetConstants = CreateMorphGeometryConstants(
				firstMorphTargetVertexDefinition,
				lod->m_bytesPerMorphTargetVertex,
				firstMorphTarget.VertexData->VertexCount,
				renderContext );

			lod->m_morphTargetAllocation.Update( &morphTargetConstants, 0, sizeof( TriMorphTargetGeometryConstants ), renderContext );

			for( int i = 0; i < grannyMesh->MorphTargetCount; ++i )
			{
				const granny_morph_target& morphTarget = grannyMesh->MorphTargets[i];
				Tr2VertexDefinition morphTargetVertexDefinition = BuildFromGrannyVertexDecl( morphTarget.VertexData->VertexType );

				uint32_t currentBytesPerMorphTargetVertex = morphTargetVertexDefinition.m_nextOffset[0];
				uint32_t currentMorphDataSize = morphTarget.VertexData->VertexCount * currentBytesPerMorphTargetVertex;

				CCP_ASSERT_M( morphTargetVertexDefinition == firstMorphTargetVertexDefinition, "Morph targets have different definitions, these need to match!" );
				CCP_ASSERT_M( vertexCount == morphTarget.VertexData->VertexCount, "Morph targets have different vertex counts, these need to match!" );
				CCP_ASSERT_M( morphDataSize == currentMorphDataSize, "Morph sizes need to match!" );

				size_t nameLength = strlen( morphTarget.ScalarName );

				// By convention (due to the exporter), the morph target name ends with "Shape".
				std::string morphTargetName( morphTarget.ScalarName );
				if( nameLength > 5 && strcmp( morphTarget.ScalarName + nameLength - 5, "Shape" ) == 0 )
				{
					morphTargetName = std::string( morphTarget.ScalarName, nameLength - 5 );
				}

				lod->m_morphTargetNames.push_back( morphTargetName );

				lod->m_isBakedMorphTarget.push_back( false );

				void* pMorphSrc = GrannyGetMeshMorphVertices( grannyMesh, i );
				lod->m_morphTargetAllocation.Update(
					pMorphSrc,
					morphDataSize * i + sizeof( TriMorphTargetGeometryConstants ),
					morphDataSize,
					renderContext );

				float deformationAmount = CalculateMorphDeformationAmount( morphTarget.DataIsDeltas, vertexCount, (uint8_t*)pMorphSrc, morphTargetVertexDefinition, (uint8_t*)pSrc, vertexDefinition );
				lod->m_morphTargetDeformationAmounts.push_back( deformationAmount );
			}

			lod->m_morphVertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( firstMorphTargetVertexDefinition );
		}
	}

	lod->m_allocationsValid = true;

	m_memoryUse += vbSize + ibSize; // Memory use is only approximate as a hint for the resource cache

	// Append reversed index buffer size also
	m_memoryUse += ibSize;

	return true;
}
#endif

#if WITH_GRANNY
static granny_mesh* FindOcclusionMesh( granny_file_info* gi, const std::string& meshName )
{
	std::string occlusionName = meshName + "_Occlusion";
	for( int i = 0; i < gi->MeshCount; ++i )
	{
		std::string name;
		CopyGrannyName( name, gi->Meshes[i]->Name );
		if( name == occlusionName )
			return gi->Meshes[i];
	}
	return nullptr;
}

bool TriGeometryRes::CreateMeshesFromGrannyFile( granny_file_info* gi, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	for( auto& mesh : m_meshes )
	{
		for( auto& lod : mesh->m_lods )
		{
			CreateLodFromGrannyMesh( gi->Meshes[lod->m_grannyMeshIndex], lod.get(), cpuUsage, renderContext );
		}

		if( g_eveIsAudioOcclusionGeometryEnabled )
		{
			granny_mesh* audioMesh = FindOcclusionMesh( gi, mesh->m_name );
			if( !audioMesh && !mesh->m_lods.empty() )
			{
				audioMesh = gi->Meshes[mesh->m_lods.back()->m_grannyMeshIndex];
			}

			if( audioMesh )
			{
				ExtractAudioGeometry( mesh.get(), audioMesh );
			}
		}
	}

	CCP_STATS_ADD( geometryResBytes, m_memoryUse );
	return true;
}

void TriGeometryRes::ExtractAudioGeometry( TriGeometryResMeshData* mesh, granny_mesh* grannyMesh )
{
	auto audioGeometry = std::make_unique<AudioGeometryResData>();

	unsigned int positionOffset;
	Tr2VertexDefinition::DataType positionType;
	GetVertexPositionOffsetAndType( grannyMesh, positionOffset, positionType );

	const int vertexCount = grannyMesh->PrimaryVertexData->VertexCount;
	Tr2VertexDefinition audioVertexDef = BuildFromGrannyVertexDecl( grannyMesh->PrimaryVertexData->VertexType );
	const unsigned int bytesPerVertex = audioVertexDef.m_nextOffset[0];

	// Read original indices from granny mesh
	int indexCount = grannyMesh->PrimaryTopology->Index16Count;
	if( indexCount == 0 )
	{
		indexCount = grannyMesh->PrimaryTopology->IndexCount;
	}

	if( indexCount == 0 || indexCount % 3 != 0 )
	{
		return;
	}


	audioGeometry->m_vertices.resize( vertexCount );
	for( int v = 0; v < vertexCount; ++v )
	{
		GetMeshVertexPosition( grannyMesh, static_cast<uint32_t>( v ), audioGeometry->m_vertices[v], bytesPerVertex, positionOffset, positionType );
	}

	audioGeometry->m_indices.resize( indexCount );
	GrannyCopyMeshIndices( grannyMesh, sizeof( uint32_t ), audioGeometry->m_indices.data() );

	audioGeometry->m_minBounds = mesh->m_minBounds;
	audioGeometry->m_maxBounds = mesh->m_maxBounds;

	mesh->m_audioGeometry = std::move( audioGeometry );
}
#endif

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

	if( IsUsingCMF() )
	{
		if( !SaveMeshToCMFFile( *meshData, filename ) )
		{
			return Be::Result<std::string>( "Failed to save the mesh" );
		}
	}
#if WITH_GRANNY
	else
	{
		if( !SaveMeshToGrannyFile( *meshData, filename ) )
		{
			return Be::Result<std::string>( "Failed to save the mesh" );
		}
	}
#else
	else
	{
		return Be::Result<std::string>( "Failed to save the mesh. Use CMF!" );
	}
#endif
	return Be::Result<std::string>();
}

bool TriGeometryRes::IsUsingCMF() const
{
#if WITH_GRANNY != 1
	CCP_ASSERT_M( m_useCMF, "TriGeometryRes: Some place was about to take the granny code path, even though it's disabled! Make sure to use CMF!" );
	return true;
#endif
	return m_useCMF;
}

const cmf::Data* TriGeometryRes::GetCMFData() const
{
	return m_cmfContents.GetData();
}

void TriGeometryRes::VisualizeBVH( Tr2DebugObjectReference owner, const Matrix& transform, ITr2DebugRenderer2& renderer ) const
{
	if( m_bvh.geometry && m_bvh.geometry->IsGood() )
	{
		m_bvh.geometry->GetBVH().Visualize( owner, transform, renderer );
	}
}
