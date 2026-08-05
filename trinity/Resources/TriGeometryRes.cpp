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


using namespace Tr2RenderContextEnum;

std::atomic<uint64_t> AudioGeometryResData::s_nextId{ 1 };

bool g_eveIsAudioOcclusionGeometryEnabled = false;
TRI_REGISTER_SETTING( "eveIsAudioOcclusionGeometryEnabled", g_eveIsAudioOcclusionGeometryEnabled );

int g_grannyDeprecationLevel = (int)GrannyDeprecationLevel::LOG_ERROR_AND_ASSERT;
TRI_REGISTER_SETTING( "grannyDeprecationLevel", g_grannyDeprecationLevel );

CCP_STATS_DECLARE( geometryResBytes, "Trinity/geometryResBytes", false, CST_MEMORY, "Size of memory occupied by geometry resources." );

#if TRINITY_PLATFORM != TRINITY_DIRECTX11
const auto gpuUsage = Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE;
#else
const auto gpuUsage = Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::INDEX_BUFFER;
#endif

Tr2SuballocatedBuffer g_sharedBuffer( "TriGeometryRes shared vertex/index buffer", gpuUsage, SHARED_BUFFER_BLOCK_SIZE, SHARED_BUFFER_MAX_SIZE );


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


static void ConvertDataToVector3( Tr2VertexDefinition::DataType elementType, const void* src, Vector3* dest )
{

	switch( elementType )
	{
	case Tr2VertexDefinition::FLOAT16_4: {
		*reinterpret_cast<Vector3*>( dest ) = *static_cast<const Vector3_16*>( src );
		break;
	}
	case Tr2VertexDefinition::FLOAT32_3: {
		memcpy( dest, src, 3 * sizeof( float ) );
		break;
	}
	case Tr2VertexDefinition::FLOAT32_4: {
		memcpy( dest, src, 3 * sizeof( float ) );
		break;
	}
	case Tr2VertexDefinition::SHORT_4: {
		ConvertShort4ToVector3( src, dest );
		break;
	}

	case Tr2VertexDefinition::UBYTE_4: {
		ConvertUByte4ToVector3( src, dest );
		break;
	}

	default: {
		dest->x = 0.0f;
		dest->y = 0.0f;
		dest->z = 0.0f;
	}
	}
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


uint32_t GetPrimitiveCount( const TriGeometryResLodData& lod, uint32_t index, uint32_t count )
{
	if( index >= lod.m_areas.size() )
	{
		return 0;
	}

	if( index + count > lod.m_areas.size() )
	{
		count = uint32_t( lod.m_areas.size() - index );
	}

	auto& meshArea = lod.m_areas[index];

	uint32_t primCount = uint32_t( meshArea.m_primitiveCount );
	for( uint32_t i = 1; i < count; ++i )
	{
		primCount += lod.m_areas[index + i].m_primitiveCount;
	}
	return primCount;
}


TriGeometryRes::TriGeometryRes( IRoot* lockobj ) :
#if WITH_GRANNY
	m_pGrannyFile( NULL ),
#endif
	m_memoryUse( 0 ),
	m_meshes( "TriGeometryRes/m_meshes" ),
	m_skeletons( "TriGeometryRes/m_skeletons" ),
	m_useCMF( true )
{
}

TriGeometryRes::~TriGeometryRes()
{
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

unsigned int TriGeometryRes::GetMeshCount() const
{
	return unsigned( m_meshes.size() );
}

TriGeometryResMeshData* TriGeometryRes::GetMeshData( unsigned int meshIx ) const
{
	if( !m_isGood || meshIx >= m_meshes.size() )
	{
		return nullptr;
	}

	return m_meshes[meshIx].get();
}

TriGeometryResLodData* TriGeometryRes::GetMeshLod( unsigned int meshIx, float screenSize ) const
{
	auto mesh = GetMeshData( meshIx );

	if( !mesh )
	{
		return nullptr;
	}

	int lodIndex = GetLodIndexForScreenSize( meshIx, screenSize );
	return mesh->m_lods[lodIndex].get();
}

TriGeometryResLodData* TriGeometryRes::GetMeshLod( unsigned int meshIx, int lodIndex ) const
{

	auto mesh = GetMeshData( meshIx );

	if( !mesh || lodIndex < 0 || lodIndex >= mesh->m_lods.size() )
	{
		return nullptr;
	}

	return mesh->m_lods[lodIndex].get();
}

int TriGeometryRes::GetLodIndexForScreenSize( unsigned int meshIx, float screenSize ) const
{
	auto mesh = GetMeshData( meshIx );

	if( !mesh )
	{
		return -1;
	}

	int32_t lastLod = static_cast<int32_t>( mesh->m_lods.size() ) - 1;
	if( m_forceLod && m_forcedLodIndex >= 0 )
	{
		return int( std::min( m_forcedLodIndex, lastLod ) );
	}

	for( int32_t i = lastLod; i >= 0; i-- )
	{
		if( mesh->m_lods[i]->m_maxScreenSize >= screenSize )
		{
			return i;
		}
	}
	//If we end up here, it's because we requested a LOD size larger than the best one we have.
	//Just return the highest quality LOD in this case.
	return 0;
}

const AudioGeometryResData* TriGeometryRes::GetAudioGeometry( unsigned int meshIx ) const
{
	if( !g_eveIsAudioOcclusionGeometryEnabled )
		return nullptr;

	auto mesh = GetMeshData( meshIx );
	if( !mesh || !mesh->m_audioGeometry )
	{
		return nullptr;
	}

	return mesh->m_audioGeometry.get();
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

bool TriGeometryRes::GetBoundingBox( unsigned int meshIx, Vector3& min, Vector3& max ) const
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return false;
	}

	min = mesh->m_minBounds;
	max = mesh->m_maxBounds;

	return true;
}

Be::Result<std::string> TriGeometryRes::GetBoundingBoxFromScript( unsigned int meshIx, std::pair<Vector3, Vector3>& bounds ) const
{
	if( meshIx >= m_meshes.size() )
	{
		return Be::Result<std::string>( "Mesh index out of bounds" );
	}
	TriGeometryResMeshData* mesh = m_meshes[meshIx].get();
	if( !mesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	bounds = std::make_pair( mesh->m_minBounds, mesh->m_maxBounds );

	return Be::Result<std::string>();
}

bool TriGeometryRes::GetAreaBoundingBox( unsigned int meshIx, unsigned int areaIx, Vector3& min, Vector3& max ) const
{
	auto mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return false;
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	// Bail out if the area index is out of range
	if( areaIx >= lod->m_areas.size() )
	{
		return false;
	}

	// Finally, get the min and max bounds for the mesh area
	min = lod->m_areas[areaIx].m_minBounds;
	max = lod->m_areas[areaIx].m_maxBounds;

	return true;
}


bool TriGeometryRes::GetBoundingSphere( unsigned int meshIx, Vector4& sphere ) const
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return false;
	}

	sphere = mesh->m_boundingSphere;

	return true;
}

unsigned int TriGeometryRes::GetAreaCount( unsigned int meshIx ) const
{
	auto mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return 0;
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	return (unsigned int)lod->m_areas.size();
}

TriGeometryResAreaData* TriGeometryRes::GetAreaData( unsigned int meshIx, unsigned int areaIx ) const
{
	auto mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return NULL;
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	if( areaIx < 0 || areaIx >= lod->m_areas.size() )
	{
		return NULL;
	}

	return &lod->m_areas[areaIx];
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
	if( s & TRISTORAGE_MANAGEDMEMORY )
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
	CCP_STATS_ZONE( __FUNCTION__ );

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
	CCP_STATS_ZONE( __FUNCTION__ );
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
	CCP_STATS_ZONE( __FUNCTION__ );

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

bool TriGeometryRes::SetupMeshes( const cmf::Data& cmfData )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	int minimumLOD = gTriDev->GetMinimumModelLOD();

	for( const cmf::Mesh& cmfMesh : cmfData.meshes )
	{
		Tr2VertexDefinition vertexDefinition = BuildFromCMFVertexDecl( cmfMesh.decl );
		const uint32_t vertexDeclarationHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( vertexDefinition );
		unsigned int bytesPerVertex = cmfMesh.lods[0].vb.stride;

		TriGeometryResMeshData* mesh = new TriGeometryResMeshData();

		mesh->m_name = cmf::ToStdString( cmfMesh.name );

		mesh->m_minBounds = cmfMesh.bounds.m_min;
		mesh->m_maxBounds = cmfMesh.bounds.m_max;

		CcpMath::Sphere sphere( cmfMesh.bounds );
		mesh->m_boundingSphere = Vector4( sphere.center, sphere.radius );

		mesh->m_vertexDeclarationHandle = vertexDeclarationHandle;
		mesh->m_bytesPerVertex = bytesPerVertex;

		for( const auto& joint : cmfMesh.boneBindings )
		{
			TriJointBinding binding;
			binding.m_name = cmf::ToStdString( joint.name );
			binding.m_obbMin = joint.bounds.m_min;
			binding.m_obbMax = joint.bounds.m_max;
			mesh->m_jointBindings.push_back( binding );
		}

		int32_t firstLodIndex = ClampInt( minimumLOD, 0, int32_t( cmfMesh.lods.size() ) - 1 );

		for( int32_t i = firstLodIndex; i < cmfMesh.lods.size(); i++ )
		{
			const cmf::MeshLod& cmfLod = cmfMesh.lods[i];

			//Setup a new LOD, which will be added as an LOD of the mesh.
			std::unique_ptr<TriGeometryResLodData> lod( new TriGeometryResLodData() );

			lod->m_mesh = mesh;

			// TODO: intern, delete field once granny has been removed.
			lod->m_grannyMeshIndex = 0xFFFFFFFF;

			lod->m_name = cmf::ToStdString( cmfMesh.name );

			lod->m_maxScreenSize = float( cmfLod.threshold );

			lod->m_vertexCount = cmf::GetStreamElementCount( cmfLod.vb );

			lod->m_primitiveCount = cmf::GetStreamElementCount( cmfLod.ib ) / 3;

			// Get the UV densities, which are used for texture streaming.
			lod->m_uvDensities = std::vector<float>( cmfMesh.uvDensities.begin(), cmfMesh.uvDensities.end() );

			lod->m_areas.resize( cmfLod.areas.size() );

			for( int groupIndex = 0; groupIndex < cmfLod.areas.size(); ++groupIndex )
			{
				const cmf::LodMeshArea& cmfLodMeshArea = cmfLod.areas[groupIndex];
				const cmf::MeshArea& cmfMeshArea = cmfMesh.areas[groupIndex];

				TriGeometryResAreaData& area = lod->m_areas[groupIndex];
				area.m_name = cmf::ToStdString( cmfMeshArea.name );

				area.m_firstIndex = cmfLodMeshArea.firstElement * 3;
				area.m_primitiveCount = cmfLodMeshArea.elementCount;

				area.m_isSkinned = cmfMeshArea.affectedByBones;
				area.m_isMorphed = cmfMeshArea.affectedByMorphTargets;

				area.m_minBounds = cmfMeshArea.bounds.m_min;
				area.m_maxBounds = cmfMeshArea.bounds.m_max;
			}

			lod->m_originalLodIndex = i;

			mesh->m_lods.push_back( std::move( lod ) );
		}

		uint32_t lodMask = 0u;
		for( const auto& lod : mesh->m_lods )
		{
			lodMask |= 1u << lod->m_originalLodIndex;
		}
		mesh->m_lodMask = lodMask;

		m_meshes.push_back( std::unique_ptr<TriGeometryResMeshData>( mesh ) );
	}

	return true;
}

#if WITH_GRANNY
bool TriGeometryRes::SetupMeshes( granny_file_info* gi )
{
	CCP_STATS_ZONE( __FUNCTION__ );

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

void TriGeometryRes::SetupSkeletons( const cmf::Data& cmfData )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_skeletons.resize( cmfData.skeletons.size() );
	for( int skelIx = 0; skelIx < cmfData.skeletons.size(); ++skelIx )
	{
		const cmf::Skeleton& skel = cmfData.skeletons[skelIx];
		std::unique_ptr<TriGeometryResSkeletonData> pSkelData( new TriGeometryResSkeletonData() );

		pSkelData->m_name = cmf::ToStdString( skel.name );
		pSkelData->m_joints.resize( skel.bones.size() );
		for( int jointIx = 0; jointIx < skel.bones.size(); ++jointIx )
		{
			TriGeometryResJointData& joint = pSkelData->m_joints[jointIx];
			joint.m_name = cmf::ToStdString( skel.bones[jointIx] );
			joint.m_parentJoint = skel.parents[jointIx];
			joint.m_inverseWorldTransform = *(Matrix*)&skel.invBindTransforms[jointIx];
		}

		std::swap( m_skeletons[skelIx], pSkelData );
	}
}

#if WITH_GRANNY
void TriGeometryRes::SetupSkeletons( granny_file_info* gi )
{
	CCP_STATS_ZONE( __FUNCTION__ );

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
	CCP_STATS_ZONE( __FUNCTION__ );

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
	CCP_STATS_ZONE( __FUNCTION__ );

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
	CCP_STATS_ZONE( __FUNCTION__ );
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

//This entire function is sus. We should be able to compute this when we upload the mesh to the GPU, as it can never change unless the vertex data changes.
void TriGeometryRes::RecalculateBoundingSphere()
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	for( auto& mesh : m_meshes )
	{
		//Try to get the vertex declaration elements
		Tr2VertexDefinition decl;
		if( mesh == nullptr || !Tr2EffectStateManager::GetVertexDeclarationElements( mesh->m_vertexDeclarationHandle, decl ) )
		{
			continue;
		}

		uint32_t vertSize = mesh->m_bytesPerVertex;

		//For now, we just compute the bounding sphere of the first LOD and ignore the lower LODs, assuming that they should fit in the highest quality LOD's bounding sphere.
		auto& lod = mesh->m_lods[0];


		//Get the vertex data of the

		const uint8_t* pVertices;
		if( !lod->m_allocationsValid || FAILED( lod->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
		{
			continue;
		}
		auto position = decl.Find( decl.POSITION, 0 );

		// build a list of pointers to the positions
		std::vector<const Vector3*> points( lod->m_vertexCount );
		for( uint32_t p = 0; p < lod->m_vertexCount; ++p )
		{
			points[p] = (const Vector3*)&pVertices[p * vertSize + position->m_offset];
		}

		// all is done in this recursive function
		BoundingSphereFromPoints( mesh->m_boundingSphere, &points[0], points.size() );
		lod->m_vertexAllocation.UnmapForReading( renderContext );
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

	for( int i = 0; i < 3; ++i )
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

static void ConvertTriangleData( Tr2VertexDefinition::DataType elementType, unsigned int vertexByteSize, const uint8_t* vertexBase, unsigned int index1, unsigned int index2, unsigned int index3, Vector3* v1, Vector3* v2, Vector3* v3 )
{

	ConvertDataToVector3( elementType, vertexBase + index1 * vertexByteSize, v1 );
	ConvertDataToVector3( elementType, vertexBase + index2 * vertexByteSize, v2 );
	ConvertDataToVector3( elementType, vertexBase + index3 * vertexByteSize, v3 );
}

void TriGeometryRes::ProcessMeshTriangles( int meshIx, PerTriangleCallback cb, void* cbContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	int i = meshIx;

	if( m_meshes[i] == NULL )
	{
		return;
	}

	auto& mesh = m_meshes[i];

	auto& lod = mesh->m_lods[0];

	if( !lod->m_allocationsValid )
	{
		return;
	}

	const uint8_t* pVertices;
	const uint8_t* pIndices;

	int vertSize = mesh->m_bytesPerVertex;

	if( FAILED( lod->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
	{
		return;
	}
	ON_BLOCK_EXIT( [&] { lod->m_vertexAllocation.UnmapForReading( renderContext ); } );
	if( FAILED( lod->m_indexAllocation.MapForReading( pIndices, renderContext ) ) )
	{
		return;
	}
	ON_BLOCK_EXIT( [&] { lod->m_indexAllocation.UnmapForReading( renderContext ); } );

	const uint16_t* pShortIndices = (uint16_t*)pIndices;
	const uint32_t* pLongIndices = (uint32_t*)pIndices;


	Tr2VertexDefinition decl;
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( mesh->m_vertexDeclarationHandle, decl ) )
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

	int numPrim = lod->m_primitiveCount;
	for( int j = 0; j < numPrim; j++ )
	{
		unsigned int index1 = 0;
		unsigned int index2 = 0;
		unsigned int index3 = 0;
		Vector3 p1;
		Vector3 p2;
		Vector3 p3;
		if( lod->m_indexAllocation.GetStride() == 2 )
		{
			index1 = pShortIndices[j * 3];
			index2 = pShortIndices[( j * 3 ) + 1];
			index3 = pShortIndices[( j * 3 ) + 2];
		}
		else
		{
			index1 = pLongIndices[j * 3];
			index2 = pLongIndices[( j * 3 ) + 1];
			index3 = pLongIndices[( j * 3 ) + 2];
		}

		ConvertTriangleData( declType, vertSize, pVertices + positionByteOffset, index1, index2, index3, &p1, &p2, &p3 );

		( *cb )( cbContext, p1, p2, p3 );
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
		return Be::Result<std::string>( "Invalid area index" );
	}

	// In Python 3, passing a signed value to an unsigned C exposed attribute is an error.
	// In order to maintain backwards compatible behavior with Python 2
	// this method got changed to accept signed integers and cast them to unsigned.
	auto unsignedAreaIndex = static_cast<unsigned int>( areaIx );

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
	float* u,
	float* v,
	float* dist )
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

bool TriGeometryRes::GetIntersectionPoints( const Vector3* pos, const Vector3* dir, Vector3* hitpointNear, Vector3* hitpointNearNormal, Vector3* hitpointFar, Vector3* hitpointFarNormal, int* boneIndexNear, int* boneIndexFar, unsigned int areaIx )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	*boneIndexFar = *boneIndexNear = -1;
	int boneIndex = 0;
	float minDist = FLT_MAX;
	float maxDist = FLT_MIN;
	bool result = false;

	for( size_t i = 0; i < m_meshes.size(); i++ )
	{
		if( m_meshes[i] == NULL )
		{
			continue;
		}

		//Get the first LOD.
		auto& lod = m_meshes[i]->m_lods[0];

		const uint8_t* pVertices;
		const uint8_t* pIndices;

		int vertSize = m_meshes[i]->m_bytesPerVertex;
		if( FAILED( lod->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
		{
			return 0;
		}
		ON_BLOCK_EXIT( [&] { lod->m_vertexAllocation.UnmapForReading( renderContext ); } );
		if( FAILED( lod->m_indexAllocation.MapForReading( pIndices, renderContext ) ) )
		{
			return false;
		}
		ON_BLOCK_EXIT( [&] { lod->m_indexAllocation.UnmapForReading( renderContext ); } );

		const uint16_t* pShortIndices = (uint16_t*)pIndices;
		const uint32_t* pLongIndices = (uint32_t*)pIndices;

		Tr2VertexDefinition decl;
		if( !Tr2EffectStateManager::GetVertexDeclarationElements( m_meshes[i]->m_vertexDeclarationHandle, decl ) )
		{
			return false;
		}
		const Tr2VertexDefinition::Item* const position = decl.Find( decl.POSITION );
		if( !position )
		{
			return false;
		}

		const Tr2VertexDefinition::Item* const blendIndices = decl.Find( decl.BLENDINDICES );
		int numPrim = lod->m_primitiveCount;
		auto currentIndex = 0;
		if( areaIx != -1 )
		{
			if( areaIx >= lod->m_areas.size() )
			{
				continue;
			}
			currentIndex = lod->m_areas[areaIx].m_firstIndex;
			numPrim = lod->m_areas[areaIx].m_primitiveCount;
		}

		for( int j = 0; j < numPrim; j++ )
		{
			unsigned int index1 = 0;
			unsigned int index2 = 0;
			unsigned int index3 = 0;
			Vector3 p1;
			Vector3 p2;
			Vector3 p3;
			float pu, pv, dist;
			if( lod->m_indexAllocation.GetStride() == 2 )
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

			if( IntersectTri( &p1, &p2, &p3, pos, dir, &pu, &pv, &dist ) )
			{
				float v1 = 1.0f - ( pu + pv );
				Vector3 avec = p2 - p1;
				Vector3 bvec = p3 - p1;
				if( minDist > dist )
				{
					*hitpointNear = p1 * v1 + p2 * pu + p3 * pv;
					*hitpointNearNormal = Normalize( Cross( avec, bvec ) );
					minDist = dist;
					if( blendIndices && GetBoneIndex( blendIndices->m_dataType, pVertices + index1 * vertSize + blendIndices->m_offset, boneIndex ) )
					{
						*boneIndexNear = boneIndex;
					}
				}
				if( maxDist < dist )
				{
					*hitpointFar = p1 * v1 + p2 * pu + p3 * pv;
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

	if( minDist == FLT_MAX )
	{
		*hitpointNear = *hitpointFar;
		*hitpointNearNormal = *hitpointFarNormal;
		*boneIndexNear = *boneIndexFar;
	}

	if( maxDist == FLT_MIN )
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
	//TODO: Do we need to clear skeletons here too?

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
#if WITH_GRANNY
	ClearGrannyData();
#endif
	ClearCMFData();
	BlueAsyncRes::Reload();
}

TriGeometryResSkeletonData::TriGeometryResSkeletonData() :
	m_joints( "TriGeometryResSkeletonData/m_joints" )
{
}

TriGeometryResAreaData::TriGeometryResAreaData() :
	m_firstIndex( 0 ),
	m_primitiveCount( 0 ),
	m_jointBindings( "TriGeometryResAreaData/m_jointBindings" ),
	m_isSkinned( false ),
	m_isMorphed( false )
{
}

TriGeometryResLodData::TriGeometryResLodData() :
	m_vertexCount( 0 ),
	m_primitiveCount( 0 ),
	m_areas( "TriGeometryResMeshData/m_areas" ),
	m_morphVertexDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_allocationsValid( false ),
	m_reversedIndicesValid( false ),
	m_uvDensities(),
	m_bytesPerMorphTargetVertex( 0 )
{
}


TriGeometryResMeshData::TriGeometryResMeshData() :

	m_vertexDeclarationHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_bytesPerVertex( -1 ),
	m_jointBindings( "TriGeometryResMeshData/m_jointBindings" ),
	m_lods( "TriGeometryResModelData/m_lods" )
{
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

	TriGeometryResMeshData* mesh = m_meshes[meshIx].get();
	if( !mesh )
	{
		return false;
	}

	TriGeometryResLodData* lod = GetMeshLod( meshIx, screenSize );

	unsigned int primCount = GetPrimitiveCount( *lod, areaIx, areaCount );

	if( primCount )
	{
		if( reversed )
		{
			if( !lod->m_reversedIndicesValid )
			{
				return false;
			}
		}

		const TriGeometryResAreaData& area = lod->m_areas[areaIx];
		renderContext.m_esm.ApplyVertexDeclaration( mesh->m_vertexDeclarationHandle );
		renderContext.m_esm.ApplyStreamSource( 0, lod->m_vertexAllocation );

		auto& indices = reversed ? lod->m_reversedIndexAllocation : lod->m_indexAllocation;
		renderContext.m_esm.ApplyIndexBuffer( indices );

		renderContext.SetTopology( TOP_TRIANGLES );
		if( reversed )
		{
			renderContext.DrawIndexedPrimitive( lod->m_vertexCount, indices.GetStartIndex() + lod->m_primitiveCount * 3 - area.m_firstIndex - primCount * 3, primCount );
		}
		else
		{
			renderContext.DrawIndexedPrimitive( lod->m_vertexCount, indices.GetStartIndex() + area.m_firstIndex, primCount );
		}
	}

	return true;
}

namespace
{
TriMorphTargetGeometryConstants CreateMorphGeometryConstants( const Tr2VertexDefinition& vertexDefinition, uint32_t stride, uint32_t vertexCount, Tr2RenderContext& renderContext )
{
	TriMorphTargetGeometryConstants data = TriMorphTargetGeometryConstants{};
	for( const auto& item : vertexDefinition.m_items )
	{
		if( item.m_usage == Tr2VertexDefinition::POSITION && item.m_usageIndex == 0 && item.m_stream == 0 )
		{
			data.positionOffset = item.m_offset;
			data.positionType = item.m_dataType;

			uint32_t type = data.positionType;
			CCP_ASSERT_M(
				type == Tr2VertexDefinition::DataType::FLOAT32_3,
				"position type has to be FLOAT32_3!" );
		}

		if( item.m_usage == Tr2VertexDefinition::TANGENT && item.m_usageIndex == 0 && item.m_stream == 0 )
		{
			data.tangentOffset = item.m_offset;
			data.tangentType = item.m_dataType;

			uint32_t type = data.tangentType;
			CCP_ASSERT_M(
				type == Tr2VertexDefinition::DataType::FLOAT16_3 ||
					type == Tr2VertexDefinition::DataType::FLOAT32_3 ||
					type == Tr2VertexDefinition::DataType::UBYTE_4_NORM ||
					type == Tr2VertexDefinition::DataType::USHORT_4_NORM ||
					type == Tr2VertexDefinition::DataType::SHORT_4_NORM,
				"tangent type has to be FLOAT16_3 or FLOAT32_3 or UBYTE_4_NORM or USHORT_4_NORM or SHORT_4_NORM!" );
		}
	}

	data.vertexBufferStride = stride;
	data.vertexCount = vertexCount;
	return data;
}
}

#if WITH_GRANNY
float CalculateMorphDeformationAmount( bool dataIsDeltas, int32_t vertexCount, uint8_t* pMorphSrc, Tr2VertexDefinition morphDecl, uint8_t* pVertices, Tr2VertexDefinition decl )
{
	CCP_STATS_ZONE( __FUNCTION__ );

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

bool TriGeometryRes::CreateLodFromCMFMesh( Tr2CmfContents& cmfContents, const cmf::Mesh& cmfMesh, const cmf::MeshLod& cmfMeshLod, TriGeometryResLodData* lod, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	Tr2VertexDefinition vertexDefinition = BuildFromCMFVertexDecl( cmfMesh.decl );
	const unsigned bytesPerVertex = cmfMeshLod.vb.stride;
	const unsigned bytesPerIndex = cmfMeshLod.ib.stride;

	int vertexCount = cmf::GetStreamElementCount( cmfMeshLod.vb );
	int vbSize = cmfMeshLod.vb.size;

	CCP_ASSERT_M( cmfContents, "TriGeometryRes::CreateLodFromCMFMesh: cmfContents must be valid!" );

	const void* pSrcVB = cmfContents.GetViewData( cmfMeshLod.vb );

	int indexCount = cmf::GetStreamElementCount( cmfMeshLod.ib );

	CR_RETURN_VAL( g_sharedBuffer.Allocate( bytesPerVertex, vertexCount, const_cast<void*>( pSrcVB ), renderContext, lod->m_vertexAllocation ), false );

	// create d3d index buffer, this one is shared, either for dynamic or static geometry
	int ibSize = indexCount * bytesPerIndex;

	const void* pSrcIB = cmfContents.GetViewData( cmfMeshLod.ib );

	if( indexCount > 0 )
	{
		ALResult hr = g_sharedBuffer.Allocate( bytesPerIndex, indexCount, const_cast<void*>( pSrcIB ), renderContext, lod->m_indexAllocation );
		if( FAILED( hr ) )
		{
			g_sharedBuffer.Free( lod->m_vertexAllocation );
			return false;
		}

		std::vector<uint8_t> tempBuffer( indexCount * bytesPerIndex );
		memcpy( tempBuffer.data(), pSrcIB, tempBuffer.size() );
		if( bytesPerIndex == 2 )
		{
			std::reverse( reinterpret_cast<uint16_t*>( tempBuffer.data() ), reinterpret_cast<uint16_t*>( tempBuffer.data() + cmfMeshLod.ib.size ) );
		}
		else
		{
			std::reverse( reinterpret_cast<uint32_t*>( tempBuffer.data() ), reinterpret_cast<uint32_t*>( tempBuffer.data() + cmfMeshLod.ib.size ) );
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
		if( cmfMesh.morphTargets.targets.size() > 0 )
		{
			// Allocate the morph target array based on the size of the first morph target
			Tr2VertexDefinition morphTargetVertexDefinition = BuildFromCMFVertexDecl( cmfMesh.morphTargets.decl );
			lod->m_bytesPerMorphTargetVertex = cmfMeshLod.morphTargets[0].vb.stride;
			uint32_t morphDataSize = vertexCount * lod->m_bytesPerMorphTargetVertex;
			uint32_t morphTargetBufferSize = (uint32_t)( morphDataSize * cmfMesh.morphTargets.targets.size() + sizeof( TriMorphTargetGeometryConstants ) );

			CR_RETURN_VAL( g_sharedBuffer.Allocate(
							   4,
							   morphTargetBufferSize >> 2,
							   nullptr,
							   renderContext,
							   lod->m_morphTargetAllocation ),
						   false );

			TriMorphTargetGeometryConstants morphTargetConstants = CreateMorphGeometryConstants(
				morphTargetVertexDefinition,
				lod->m_bytesPerMorphTargetVertex,
				vertexCount,
				renderContext );

			lod->m_morphTargetAllocation.Update( &morphTargetConstants, 0, sizeof( TriMorphTargetGeometryConstants ), renderContext );

			for( int i = 0; i < cmfMesh.morphTargets.targets.size(); ++i )
			{
				const cmf::MorphTarget& morphTarget = cmfMesh.morphTargets.targets[i];

				size_t nameLength = morphTarget.name.size();
				// TODO: intern, this "Shape" suffix will likely be gone later
				// By convention (due to the exporter), the morph target name ends with "Shape". Assert that it does, and also that it is not an empty string!
				std::string morphTargetName = cmf::ToStdString( morphTarget.name );
				if( nameLength > 5 && strcmp( morphTargetName.c_str() + nameLength - 5, "Shape" ) == 0 )
				{
					morphTargetName = std::string( morphTarget.name.data(), nameLength - 5 );
				}

				lod->m_morphTargetNames.push_back( morphTargetName );

				lod->m_isBakedMorphTarget.push_back( false );

				const void* pMorphSrc = cmfContents.GetViewData( cmfMeshLod.morphTargets[i].vb );
				lod->m_morphTargetAllocation.Update(
					const_cast<void*>( pMorphSrc ),
					morphDataSize * i + sizeof( TriMorphTargetGeometryConstants ),
					morphDataSize,
					renderContext );

				lod->m_morphTargetDeformationAmounts.push_back( morphTarget.maxDisplacement );
			}

			lod->m_morphVertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( morphTargetVertexDefinition );
		}
	}

	lod->m_allocationsValid = true;

	m_memoryUse += vbSize + ibSize; // Memory use is only approximate as a hint for the resource cache

	// Append reversed index buffer size also
	m_memoryUse += ibSize;

	return true;
}

#if WITH_GRANNY
bool TriGeometryRes::CreateLodFromGrannyMesh( granny_mesh* grannyMesh, TriGeometryResLodData* lod, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext, void* pVBOverride )
{
	CCP_STATS_ZONE( __FUNCTION__ );

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

bool TriGeometryRes::CreateMeshesFromCMFFile( Tr2CmfContents& cmfContents, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !cmfContents.GetData() )
	{
		return false;
	}

	for( int32_t i = 0; i < m_meshes.size(); i++ )
	{
		auto& mesh = m_meshes[i];

		for( const auto& lod : mesh->m_lods )
		{
			CreateLodFromCMFMesh( cmfContents, cmfContents.GetData()->meshes[i], cmfContents.GetData()->meshes[i].lods[lod->m_originalLodIndex], lod.get(), cpuUsage, renderContext );
		}

		if( g_eveIsAudioOcclusionGeometryEnabled )
		{
			const auto& audioMesh = cmfContents.GetData()->meshes[i].audioOcclusionMesh;
			if( !audioMesh.vertices.empty() && !audioMesh.indices.empty() )
			{
				auto audioGeometry = std::make_unique<AudioGeometryResData>();
				audioGeometry->m_indices.insert( audioGeometry->m_indices.end(), audioMesh.indices.begin(), audioMesh.indices.end() );
				audioGeometry->m_vertices.insert( audioGeometry->m_vertices.end(), audioMesh.vertices.begin(), audioMesh.vertices.end() );
				audioGeometry->m_minBounds = audioMesh.bounds.m_min;
				audioGeometry->m_maxBounds = audioMesh.bounds.m_max;

				mesh->m_audioGeometry = std::move( audioGeometry );
			}
		}
	}
	CCP_STATS_ADD( geometryResBytes, m_memoryUse );

	return true;
}

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
	CCP_STATS_ZONE( __FUNCTION__ );

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

bool TriGeometryRes::SaveMeshToCMFFile( TriGeometryResMeshData& mesh, const char* filename )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( mesh.m_vertexDeclarationHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	auto& lod = mesh.m_lods[0];

	if( !lod->m_allocationsValid )
	{
		return false;
	}

	Tr2VertexDefinition decl;
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( mesh.m_vertexDeclarationHandle, decl ) )
	{
		return false;
	}

	cmf::MemoryAllocator allocator;
	cmf::BufferManager bufferAllocator( allocator );
	cmf::Span<cmf::VertexElement> cmfVertexDecl = allocator.AllocateSpan<cmf::VertexElement>( decl.m_items.size() );

	if( !ConvertVertexDeclToCMF( decl, cmfVertexDecl, (uint32_t)cmfVertexDecl.size() ) )
	{
		return false;
	}

	uint32_t vertexBufferSize = lod->m_vertexAllocation.GetSize();
	uint32_t vertexBufferStride = lod->m_vertexAllocation.GetStride();
	const void* pVertices;
	if( FAILED( lod->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { lod->m_vertexAllocation.UnmapForReading( renderContext ); } );

	uint32_t indexBufferSize = lod->m_indexAllocation.GetSize();
	uint32_t indexBufferStride = lod->m_indexAllocation.GetStride();
	const void* pIndices;
	if( FAILED( lod->m_indexAllocation.MapForReading( pIndices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { lod->m_indexAllocation.UnmapForReading( renderContext ); } );


	cmf::Data cmfData;
	cmfData.meshes = allocator.AllocateSpan<cmf::Mesh>( 1 );

	cmf::Mesh& cmfMesh = cmfData.meshes[0] = {};
	cmfMesh.name = allocator.AllocateString( mesh.m_name );
	cmfMesh.decl = cmfVertexDecl;
	cmfMesh.lods = allocator.AllocateSpan<cmf::MeshLod>( 1 );
	cmfMesh.areas = allocator.AllocateSpan<cmf::MeshArea>( lod->m_areas.size() );
	cmfMesh.boneBindings = allocator.AllocateSpan<cmf::BoneBinding>( mesh.m_jointBindings.size() );
	cmfMesh.uvDensities = allocator.AllocateSpan<float>( lod->m_uvDensities.size() );
	cmfMesh.bounds = AxisAlignedBoundingBox( mesh.m_minBounds, mesh.m_maxBounds );

	memcpy( cmfMesh.uvDensities.data(), lod->m_uvDensities.data(), lod->m_uvDensities.size() * sizeof( float ) );

	cmf::MeshLod& cmfMeshLod = cmfMesh.lods[0];
	cmfMeshLod.vb = bufferAllocator.AddBuffer( (void*)pVertices, vertexBufferSize, vertexBufferStride );
	cmfMeshLod.ib = bufferAllocator.AddBuffer( (void*)pIndices, indexBufferSize, indexBufferStride );
	cmfMeshLod.areas = allocator.AllocateSpan<cmf::LodMeshArea>( lod->m_areas.size() );
	cmfMeshLod.morphTargets = cmf::Span<cmf::LodMorphTarget>();
	cmfMeshLod.threshold = (uint32_t)lod->m_maxScreenSize;

	for( int32_t i = 0; i < cmfMesh.areas.size(); i++ )
	{
		cmf::MeshArea& cmfMeshArea = cmfMesh.areas[i];
		cmfMeshArea.name = allocator.AllocateString( lod->m_areas[i].m_name );
		cmfMeshArea.bounds = AxisAlignedBoundingBox( lod->m_areas[i].m_minBounds, lod->m_areas[i].m_maxBounds );
		cmfMeshArea.bones = allocator.AllocateSpan<uint16_t>( lod->m_areas[i].m_jointBindings.size() );
		cmfMeshArea.affectedByBones = lod->m_areas[i].m_isSkinned;
		cmfMeshArea.affectedByMorphTargets = false;

		for( int32_t j = 0; j < lod->m_areas[i].m_jointBindings.size(); j++ )
		{
			cmfMeshArea.bones[j] = (uint16_t)lod->m_areas[i].m_jointBindings[j];
		}

		cmf::LodMeshArea& lodMeshArea = cmfMeshLod.areas[i];
		lodMeshArea.firstElement = lod->m_areas[i].m_firstIndex / 3;
		lodMeshArea.elementCount = lod->m_areas[i].m_primitiveCount;
	}

	for( int32_t i = 0; i < cmfMesh.boneBindings.size(); i++ )
	{
		cmf::BoneBinding& cmfBoneBinding = cmfMesh.boneBindings[i];
		cmfBoneBinding.name = allocator.AllocateString( mesh.m_jointBindings[i].m_name );
		cmfBoneBinding.bounds = AxisAlignedBoundingBox( mesh.m_jointBindings[i].m_obbMin, mesh.m_jointBindings[i].m_obbMax );
	}

	auto file = cmf::BuildFile( cmfData, bufferAllocator );

	std::wstring filenameW = (const wchar_t*)CA2W( filename );
	std::wstring fullPath = BePaths->ResolvePathForWritingW( filenameW );

	FILE* outFile;
	auto err = fopen_s( &outFile, CW2A( fullPath.c_str() ), "wb" );

	if( err != 0 )
	{
		CCP_LOGERR( "Failed to open output file: %ls\n", fullPath.c_str() );
		return false;
	}
	if( fwrite( file.data(), 1, file.size(), outFile ) != file.size() )
	{
		CCP_LOGERR( "Failed to write output file: %ls\n", fullPath.c_str() );
		fclose( outFile );
		return false;
	}
	fclose( outFile );

	return true;
}

#if WITH_GRANNY
bool TriGeometryRes::SaveMeshToGrannyFile( TriGeometryResMeshData& mesh, const char* filename )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( mesh.m_vertexDeclarationHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	auto& lod = mesh.m_lods[0];

	if( !lod->m_allocationsValid )
	{
		return false;
	}

	granny_data_type_definition grannyVertexDecl[16];

	Tr2VertexDefinition decl;
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( mesh.m_vertexDeclarationHandle, decl ) )
	{
		return false;
	}

	if( !ConvertVertexDeclToGranny( decl, grannyVertexDecl, sizeof( grannyVertexDecl ) / sizeof( grannyVertexDecl[0] ) ) )
	{
		return false;
	}

	const void* pVertices;
	if( FAILED( lod->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { lod->m_vertexAllocation.UnmapForReading( renderContext ); } );

	const void* pIndices;
	if( FAILED( lod->m_indexAllocation.MapForReading( pIndices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { lod->m_indexAllocation.UnmapForReading( renderContext ); } );


	granny_vertex_data myVertexData;
	memset( &myVertexData, 0, sizeof( myVertexData ) );
	myVertexData.Vertices = (granny_uint8*)pVertices;
	myVertexData.VertexCount = lod->m_vertexCount;
	myVertexData.VertexType = grannyVertexDecl;

	granny_tri_topology myTopology;
	memset( &myTopology, 0, sizeof( myTopology ) );

	if( lod->m_vertexCount <= 65535 )
	{
		myTopology.Indices16 = (granny_uint16*)pIndices;
		myTopology.Index16Count = lod->m_primitiveCount * 3;
	}
	else
	{
		myTopology.Indices = (granny_int32*)pIndices;
		myTopology.IndexCount = lod->m_primitiveCount * 3;
	}

	myTopology.GroupCount = (granny_int32)lod->m_areas.size();
	myTopology.Groups = CCP_NEW( "myTopology.Groups" ) granny_tri_material_group[myTopology.GroupCount];
	if( myTopology.Groups )
	{
		for( int i = 0; i < myTopology.GroupCount; ++i )
		{
			myTopology.Groups[i].MaterialIndex = i;
			myTopology.Groups[i].TriFirst = lod->m_areas[i].m_firstIndex / 3;
			myTopology.Groups[i].TriCount = lod->m_areas[i].m_primitiveCount;
		}
	}
	else
	{
		myTopology.GroupCount = 0;
	}

	granny_mesh grannyMesh;
	memset( &grannyMesh, 0, sizeof( grannyMesh ) );

	grannyMesh.PrimaryVertexData = &myVertexData;
	grannyMesh.PrimaryTopology = &myTopology;

	granny_file_info fileInfo;
	memset( &fileInfo, 0, sizeof( fileInfo ) );

	granny_mesh* meshes[] = { &grannyMesh };
	granny_tri_topology* topologies[] = { grannyMesh.PrimaryTopology };
	granny_vertex_data* vertexDatas[] = { grannyMesh.PrimaryVertexData };

	fileInfo.ModelCount = 0;
	fileInfo.Models = 0;
	fileInfo.MeshCount = 1;
	fileInfo.Meshes = meshes;
	fileInfo.VertexDataCount = 1;
	fileInfo.VertexDatas = vertexDatas;
	fileInfo.TriTopologyCount = 1;
	fileInfo.TriTopologies = topologies;

	fileInfo.MaterialCount = (granny_int32)lod->m_areas.size();
	fileInfo.Materials = CCP_NEW( "fileInfo.Materials" ) granny_material * [fileInfo.MaterialCount];
	if( fileInfo.Materials == nullptr )
	{
		fileInfo.MaterialCount = 0;
	}

	grannyMesh.MaterialBindingCount = fileInfo.MaterialCount;
	grannyMesh.MaterialBindings = CCP_NEW( "grannyMesh.MaterialBindings" ) granny_material_binding[fileInfo.MaterialCount];
	if( grannyMesh.MaterialBindings == nullptr )
	{
		grannyMesh.MaterialBindingCount = 0;
	}

	granny_material* pMaterials = nullptr;
	if( fileInfo.MaterialCount )
	{
		pMaterials = CCP_NEW( "pMaterials" ) granny_material[fileInfo.MaterialCount];
		if( pMaterials )
		{
			memset( pMaterials, 0, sizeof( granny_material ) * fileInfo.MaterialCount );

			for( int i = 0; i < fileInfo.MaterialCount; ++i )
			{
				fileInfo.Materials[i] = &pMaterials[i];
				fileInfo.Materials[i]->Name = lod->m_areas[i].m_name.c_str();
				grannyMesh.MaterialBindings[i].Material = fileInfo.Materials[i];
			}
		}
	}

	// Bones
	grannyMesh.BoneBindingCount = (granny_int32)mesh.m_jointBindings.size();
	grannyMesh.BoneBindings = CCP_NEW( "grannyMesh.BoneBindings" ) granny_bone_binding[grannyMesh.BoneBindingCount];
	if( grannyMesh.BoneBindings == nullptr )
	{
		grannyMesh.BoneBindingCount = 0;
	}
	if( grannyMesh.BoneBindingCount > 0 )
	{
		memset( grannyMesh.BoneBindings, 0, sizeof( granny_bone_binding ) * grannyMesh.BoneBindingCount );
		for( int i = 0; i < grannyMesh.BoneBindingCount; ++i )
		{
			grannyMesh.BoneBindings[i].BoneName = mesh.m_jointBindings[i].m_name.c_str();
			grannyMesh.BoneBindings[i].OBBMin[0] = mesh.m_jointBindings[i].m_obbMin.x;
			grannyMesh.BoneBindings[i].OBBMin[1] = mesh.m_jointBindings[i].m_obbMin.y;
			grannyMesh.BoneBindings[i].OBBMin[2] = mesh.m_jointBindings[i].m_obbMin.z;
			grannyMesh.BoneBindings[i].OBBMax[0] = mesh.m_jointBindings[i].m_obbMax.x;
			grannyMesh.BoneBindings[i].OBBMax[1] = mesh.m_jointBindings[i].m_obbMax.y;
			grannyMesh.BoneBindings[i].OBBMax[2] = mesh.m_jointBindings[i].m_obbMax.z;
		}
	}


	granny_file_builder* Builder = GrannyBeginFile( 1, GrannyCurrentGRNStandardTag, GrannyGRNFileMV_32Bit_LittleEndian, GrannyGetTemporaryDirectory(), "GRNFileTemp" );
	granny_file_data_tree_writer* Writer =
		GrannyBeginFileDataTreeWriting( GrannyFileInfoType, &fileInfo, 0, 0 );
	GrannyWriteDataTreeToFileBuilder( Writer, Builder );
	GrannyEndFileDataTreeWriting( Writer );

	std::wstring filenameW = (const wchar_t*)CA2W( filename );
	std::wstring fullPath = BePaths->ResolvePathForWritingW( filenameW );
	GrannyEndFile( Builder, CW2A( fullPath.c_str() ) );

	CCP_DELETE[] pMaterials;
	CCP_DELETE[] grannyMesh.MaterialBindings;
	CCP_DELETE[] grannyMesh.BoneBindings;
	CCP_DELETE[] fileInfo.Materials;

	return true;
}
#endif

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
	auto mesh = GetMeshData( bufferIndex );

	if( !mesh )
	{
		static Tr2BufferAL nullVB;
		return { nullVB };
	}

	auto lod = GetMeshLod( bufferIndex, screenSize );
	return {
		lod->m_vertexAllocation.GetBuffer(), lod->m_vertexAllocation.GetOffset(), mesh->m_bytesPerVertex, lod->m_vertexCount
	};
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
		return m_meshes[bufferIndex]->m_vertexDeclarationHandle;
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

Be::Result<std::string> TriGeometryRes::GetMeshName( unsigned int meshIx, std::string& name ) const
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return Be::Result<std::string>( "Mesh index out of range, or invalid mesh" );
	}

	name = mesh->m_name;

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGeometryRes::GetMeshAreaCount( unsigned int meshIx, int& count ) const
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return Be::Result<std::string>( "Mesh index out of range, or invalid mesh" );
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	count = (int)lod->m_areas.size();

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGeometryRes::GetMeshAreaName( unsigned int meshIx, unsigned int areaIx, std::string& name ) const
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return Be::Result<std::string>( "Mesh index out of range, or invalid mesh" );
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	if( areaIx < 0 || areaIx >= lod->m_areas.size() )
	{
		return Be::Result<std::string>( "Area index out of range" );
	}

	name = lod->m_areas[areaIx].m_name;

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGeometryRes::GetAreaBoundingBoxFromScript( unsigned int meshIx, unsigned int areaIx, std::pair<Vector3, Vector3>& bounds )
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return Be::Result<std::string>( "Mesh index out of range, or invalid mesh" );
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	if( areaIx < 0 || areaIx >= lod->m_areas.size() )
	{
		return Be::Result<std::string>( "Area index out of range" );
	}

	bounds = std::make_pair( lod->m_areas[areaIx].m_minBounds, lod->m_areas[areaIx].m_maxBounds );

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGeometryRes::GetBoundingSphereFromScript( unsigned int meshIx, std::pair<Vector3, float>& bounds ) const
{
	if( meshIx >= m_meshes.size() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}
	auto& mesh = m_meshes[meshIx];
	if( !mesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	bounds = std::make_pair( Vector3( mesh->m_boundingSphere.x, mesh->m_boundingSphere.y, mesh->m_boundingSphere.z ), mesh->m_boundingSphere.w );
	return Be::Result<std::string>();
}

Be::Result<std::string> TriGeometryRes::CalculateBoundingBoxFromTransform( unsigned int meshIx, const Matrix& transform, std::pair<Vector3, Vector3>& bounds )
{
	if( meshIx >= m_meshes.size() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}
	auto& mesh = m_meshes[meshIx];
	if( !mesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	CalcBoundingBoxContext ctx;
	ctx.min = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );
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
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( mesh->m_vertexDeclarationHandle, decl ) )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "could not retrieve vertex declaration" );
	}

	for( auto it = std::begin( decl.m_items ); it != std::end( decl.m_items ); ++it )
	{
		elements.push_back( std::make_pair( it->m_usage, it->m_usageIndex ) );
	}
	return BLUE_STD_RESULT_OK;
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