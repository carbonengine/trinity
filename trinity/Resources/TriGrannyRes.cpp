// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriGrannyRes.h"

#include "Utilities/GeometryUtils.h"
#include "Tr2VertexDefinitionUtilities.h"

#include <cctype>


extern int g_grannyDeprecationLevel;


CmfVertexReader::CmfVertexReader( cmf::Span<cmf::VertexElement> decl )
{
	posElem = cmf::FindElement( decl, cmf::Usage::Position );
	normElem = cmf::FindElement( decl, cmf::Usage::Normal );
	tanElem = cmf::FindElement( decl, cmf::Usage::Tangent );
	binormElem = cmf::FindElement( decl, cmf::Usage::Binormal );
	pkdTanElem = cmf::FindElement( decl, cmf::Usage::PackedTangent );
	pkdLegElem = cmf::FindElement( decl, cmf::Usage::PackedTangentLegacy );
}

void ReadCmfVertexAttributes( const CmfVertexReader& reader, const uint8_t* data, uint32_t vertexCount, uint32_t stride, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<Vector3>* tangents, std::vector<Vector3>* binormals )
{
	std::optional<cmf::ConstBufferElementStream<Vector3>> posStream;
	std::optional<cmf::ConstBufferElementStream<Vector3>> normStream;
	std::optional<cmf::ConstBufferElementStream<Vector3>> tanStream;
	std::optional<cmf::ConstBufferElementStream<Vector3>> binormStream;
	std::optional<cmf::ConstBufferElementStream<Vector4>> pkdTanStream;

	if( reader.posElem )
	{
		posStream.emplace( *reader.posElem, data, vertexCount, stride );
	}
	if( reader.normElem )
	{
		normStream.emplace( *reader.normElem, data, vertexCount, stride );
	}
	if( reader.tanElem )
	{
		tanStream.emplace( *reader.tanElem, data, vertexCount, stride );
	}
	if( reader.binormElem )
	{
		binormStream.emplace( *reader.binormElem, data, vertexCount, stride );
	}

	if( reader.pkdTanElem )
	{
		pkdTanStream.emplace( *reader.pkdTanElem, data, vertexCount, stride );
	}
	else if( reader.pkdLegElem )
	{
		pkdTanStream.emplace( *reader.pkdLegElem, data, vertexCount, stride );
	}

	for( uint32_t i = 0; i < vertexCount; i++ )
	{
		if( reader.posElem )
		{
			positions[i] = ( *posStream )[i];
		}

		if( reader.pkdTanElem )
		{
			Vector4 packed = ( *pkdTanStream )[i];
			Vector3 tangent, binormal;
			std::tie( normals[i], tangent, binormal ) = cmf::UnpackTangents( cmf::TangentCompression::PackedTangent, packed );
			if( tangents )
			{
				( *tangents )[i] = tangent;
			}
			if( binormals )
			{
				( *binormals )[i] = binormal;
			}
		}
		else if( reader.pkdLegElem )
		{
			Vector4 packed = ( *pkdTanStream )[i];
			Vector3 tangent, binormal;
			std::tie( normals[i], tangent, binormal ) = cmf::UnpackTangents( cmf::TangentCompression::PackedTangentLegacy, packed );
			if( tangents )
			{
				( *tangents )[i] = tangent;
			}
			if( binormals )
			{
				( *binormals )[i] = binormal;
			}
		}
		else
		{
			if( normStream )
			{
				normals[i] = ( *normStream )[i];
			}
			if( tanStream && tangents )
			{
				( *tangents )[i] = ( *tanStream )[i];
			}
			if( binormStream && binormals )
			{
				( *binormals )[i] = ( *binormStream )[i];
			}
		}
	}
}


IBlueResource* CreateGrannyResource( const wchar_t* name )
{
	TriGrannyResPtr p;
	p.CreateInstance();
	return p.Detach();
}

#if WITH_GRANNY
BLUE_REGISTER_RESOURCE_EXTENSION( L"gr2raw", CreateGrannyResource );
#endif
BLUE_REGISTER_RESOURCE_EXTENSION( L"cmfraw", CreateGrannyResource );


Tr2GrannyIntersectionResult::Tr2GrannyIntersectionResult( IRoot* )
{
}

Tr2GrannyIntersectionResult::Result::Result() :
	position( 0, 0, 0 ),
	normal( 0, 0, 0 ),
	uv( 0, 0 ),
	meshIndex( -1 ),
	areaIndex( -1 ),
	boneIndex( -1 ),
	hasPosition( false ),
	hasNormal( false ),
	hasUv( false ),
	hasBoneIndex( false )
{
}


TriGrannyRes::TriGrannyRes( IRoot* lockobj ) :
#if WITH_GRANNY
	m_grannyFile( NULL ),
	m_grannyArena( NULL ),
	m_data( NULL ),
	m_dataSize( 0 ),
#endif
	m_memoryUsage( 0 ),
	m_useCMF( true )
{
}

TriGrannyRes::~TriGrannyRes()
{
#if WITH_GRANNY
	if( m_grannyFile )
	{
		GrannyFreeFile( m_grannyFile );
	}

	if( m_grannyArena )
	{
		// The granny dynamic memory management
		GrannyFreeMemoryArena( m_grannyArena );
	}
#endif
}

bool TriGrannyRes::Load( const std::string& path )
{
	m_path = CA2W( path.c_str() );
	m_ext = L"raw";
	m_dataStream = nullptr;
	BePaths->GetStreamFromPathW( m_path.c_str(), &m_dataStream );
	if( !m_dataStream )
	{
		CCP_LOGERR( "TriGrannyRes::Load: Failed to open stream for path %ls", m_path.c_str() );
		return false;
	}
	m_isLoading = true;
	m_isPrepared = false;
	m_isGood = false;
	auto loaded = DoLoad();
	m_isLoading = false;
	CleanupLoadData();
	CloseStream();
	if( loaded != LR_SUCCESS )
	{
		return false;
	}
	m_isGood = DoPrepare();
	m_isPrepared = true;
	return m_isGood;
}

BlueAsyncRes::LoadingResult TriGrannyRes::DoLoad()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_path.size() >= 4 && m_path.compare( m_path.size() - 4, 4, L".cmf" ) == 0 )
	{
		m_useCMF = true;

		m_cmfContents = Tr2CmfContents( *m_dataStream, CW2A( m_path.c_str() ) );
		if( !m_cmfContents )
		{
			return LR_FAILED;
		}
		m_memoryUsage = m_dataStream->GetSize();
	}
#if WITH_GRANNY
	else
	{
		m_useCMF = false;

		HandleGrannyDeprecation( m_path );

		if( !m_dataStream->LockData( &m_data, 0 ) )
		{
			return LR_FAILED;
		}

		m_dataSize = m_dataStream->GetSize();

		{
			if( m_grannyFile )
			{
				GrannyFreeFile( m_grannyFile );
				m_grannyFile = NULL;
			}

			CCP_STATS_ZONE( "TriGrannyRes::DoLoad reading Granny file" );

			m_grannyFile = ProtectedGrannyReadEntireFileFromMemory( m_path.c_str(), (uint32_t)m_dataSize, m_data );
		}

		if( !m_grannyFile )
		{
			return LR_FAILED;
		}

		granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );
		if( !fi )
		{
			CCP_LOGERR( "TriGrannyRes::GetGrannyMesh: Granny file has no file info" );
			return LR_FAILED;
		}

		m_memoryUsage = 0;
		granny_grn_section* sections = GrannyGetGRNSectionArray( m_grannyFile->Header );
		for( int i = 0; i < m_grannyFile->SectionCount; ++i )
		{
			m_memoryUsage += sections[i].ExpandedDataSize;
		}
	}
#else
	else
	{
		HandleGrannyDeprecation( m_path );
		return LR_FAILED;
	}
#endif

	return LR_SUCCESS;
}

bool TriGrannyRes::DoPrepare()
{
	return true;
}

#if WITH_GRANNY
const granny_mesh* TriGrannyRes::GetGrannyMesh( int meshIx ) const
{
	// helper function to safely access a granny_mesh struct
	if( !m_grannyFile )
	{
		CCP_LOGERR( "TriGrannyRes::GetGrannyMesh: Object has no Granny file" );
		return NULL;
	}

	granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );
	if( !fi )
	{
		CCP_LOGERR( "TriGrannyRes::GetGrannyMesh: Granny file has no file info" );
		return NULL;
	}

	if( fi->MeshCount <= (granny_int32)meshIx )
	{
		CCP_LOGERR( "TriGrannyRes::GetGrannyMesh: meshindex too high" );
		return NULL;
	}

	return fi->Meshes[meshIx];
}
#endif

#if WITH_GRANNY
granny_data_type_definition* TriGrannyRes::GetGrannyVertexType( int meshIx ) const
{
	const granny_mesh* mesh = GetGrannyMesh( meshIx );
	if( !mesh )
	{
		CCP_LOGERR( "TriGrannyRes::GetGrannyVertexType: Invalid mesh index" );
		return NULL;
	}

	return mesh->PrimaryVertexData->VertexType;
}
#endif

int TriGrannyRes::GetVertexSize( int meshIx ) const
{
	if( IsUsingCMF() )
	{
		if( !m_cmfContents )
		{
			return 0;
		}

		if( meshIx < 0 || meshIx >= m_cmfContents.GetData()->meshes.size() )
		{
			return 0;
		}
		return m_cmfContents.GetData()->meshes[meshIx].lods[0].vb.stride;
	}
#if WITH_GRANNY
	else
	{
		const granny_data_type_definition* vertexFormat = GetGrannyVertexType( meshIx );
		if( !vertexFormat )
		{
			return 0;
		}
		return GrannyGetTotalObjectSize( vertexFormat );
	}
#else
	else
	{
		return 0;
	}
#endif
}

#if WITH_GRANNY
granny_skeleton* TriGrannyRes::GetGrannySkeleton( int skeletonIx ) const
{
	if( !m_grannyFile )
	{
		CCP_LOGERR( "TriGrannyRes::GetGrannySkeleton: Object has no Granny file" );
		return NULL;
	}

	granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );
	if( !fi )
	{
		CCP_LOGERR( "TriGrannyRes::GetGrannySkeleton: Granny file has no file info" );
		return NULL;
	}

	if( !fi->SkeletonCount )
	{
		return NULL;
	}

	if( fi->SkeletonCount <= (granny_int32)skeletonIx )
	{
		CCP_LOGERR( "TriGrannyRes::GetGrannySkeleton: skeletonindex too high" );
		return NULL;
	}

	return fi->Skeletons[skeletonIx];
}
#endif

#if WITH_GRANNY
int TriGrannyRes::GetVertexComponentOffset( int meshIx, const char* componentName ) const
{
	CCP_ASSERT_M( !m_useCMF, "This should never be called on a cmf codepath!" );

	const granny_mesh* mesh = GetGrannyMesh( meshIx );
	if( !mesh )
	{
		CCP_LOGERR( "TriGrannyRes::GetVertexComponentOffset: Invalid mesh index" );
		return -1;
	}

	// now scan granny's vertex-declaration for the bone index part and count the offsets
	granny_data_type_definition* vertexFormat = mesh->PrimaryVertexData->VertexType;
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
	return -1;
}
#endif

bool TriGrannyRes::BakeBlendshape( unsigned int meshIx, const std::vector<float>& weights, Tr2SuballocatedBuffer::Allocation& pVertexData, Tr2RenderContextAL& renderContext, unsigned int vertexDataSize )
{
	return BakeBlendshape( meshIx, weights, pVertexData, renderContext, vertexDataSize, NULL, false );
}

bool TriGrannyRes::BakeBlendshape( unsigned int meshIx, const NameToWeightMap& nameToWeight, Tr2SuballocatedBuffer::Allocation& pVertexData, Tr2RenderContextAL& renderContext, unsigned int vertexDataSize )
{
	std::vector<float> dummyWeights;
	return BakeBlendshape( meshIx, dummyWeights, pVertexData, renderContext, vertexDataSize, &nameToWeight, false );
}

bool TriGrannyRes::BakeBlendshape( unsigned int meshIx, const std::vector<float>& weights, Tr2SuballocatedBuffer::Allocation& pVertexData, Tr2RenderContextAL& renderContext, unsigned int vertexDataSize, const NameToWeightMap* const nameToWeight, bool deltaOnly )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( IsUsingCMF() )
	{
		CCP_ASSERT_M( !nameToWeight, "TriGrannyRes::BakeBlendshape: intern messed up and thought nameToWeight was not being used anymore." );
		CCP_ASSERT_M( !deltaOnly, "TriGrannyRes::BakeBlendshape: intern messed up and thought deltaOnly was not being used anymore." );

		if( meshIx >= m_cmfContents.GetData()->meshes.size() )
		{
			CCP_LOGERR( "TriGrannyRes::BakeBlendshape: Invalid mesh index" );
			return false;
		}

		CCP_ASSERT_M( weights.size() == m_cmfContents.GetData()->meshes[meshIx].morphTargets.targets.size(), "" );

		auto vb = m_cmfContents.GetData()->meshes[meshIx].lods[0].vb;
		auto vbDecl = m_cmfContents.GetData()->meshes[meshIx].decl;

		uint32_t vertexCount = cmf::GetStreamElementCount( vb );
		std::vector<Vector3> positions( vertexCount, Vector3( 0.f, 0.f, 0.f ) );
		std::vector<Vector3> normals( vertexCount, Vector3( 0.f, 0.f, 0.f ) );
		std::vector<Vector3> tangents( vertexCount, Vector3( 0.f, 0.f, 0.f ) );
		std::vector<Vector3> binormals( vertexCount, Vector3( 0.f, 0.f, 0.f ) );

		std::vector<uint8_t> result( vb.size );

		CmfVertexReader baseReader( vbDecl );

		if( !baseReader.posElem )
		{
			CCP_LOGERR( "TriGrannyRes::BakeBlendshape: mesh has no Position element" );
			return false;
		}

		{ // read vb data and unpack it. put copy of original data into result.
			const uint8_t* vbData = static_cast<const uint8_t*>( m_cmfContents.GetViewData( vb ) );
			ReadCmfVertexAttributes( baseReader, vbData, vertexCount, vb.stride, positions, normals, &tangents, &binormals );
			memcpy( result.data(), vbData, result.size() );
		}

		// copy original normals to be able to convert from absolute to relative...
		std::vector<Vector3> basePositions( positions );
		std::vector<Vector3> baseNormals( normals );
		std::vector<Vector3> baseTangents( tangents );
		std::vector<Vector3> baseBinormals( binormals );

		{ // read morph target data and unpack it. then use it.
			auto morphDecl = m_cmfContents.GetData()->meshes[meshIx].morphTargets.decl;
			std::vector<Vector3> morphPositions, morphNormals, morphTangents, morphBinormals;
			CmfVertexReader morphReader( morphDecl );

			for( int32_t j = 0; j < weights.size(); j++ )
			{
				float weight = weights[j];

				if( weight < 1e-4f )
				{
					continue;
				}

				morphPositions = basePositions;
				morphNormals = baseNormals;
				morphTangents = baseTangents;
				morphBinormals = baseBinormals;

				auto morphVB = m_cmfContents.GetData()->meshes[meshIx].lods[0].morphTargets[j].vb;
				const uint8_t* morphData = static_cast<const uint8_t*>( m_cmfContents.GetViewData( morphVB ) );
				ReadCmfVertexAttributes( morphReader, morphData, vertexCount, morphVB.stride, morphPositions, morphNormals, &morphTangents, &morphBinormals );

				for( uint32_t i = 0; i < vertexCount; i++ )
				{
					positions[i] += weight * ( morphPositions[i] - basePositions[i] );
					normals[i] += weight * ( morphNormals[i] - baseNormals[i] );
					tangents[i] += weight * ( morphTangents[i] - baseTangents[i] );
					binormals[i] += weight * ( morphBinormals[i] - baseBinormals[i] );
				}
			}
		}

		{ // overwrite entries in result with morphed data
			cmf::BufferElementStream<Vector3> posOutStream( *baseReader.posElem, result.data(), vertexCount, vb.stride );

			std::optional<cmf::BufferElementStream<Vector3>> normOutStream, tanOutStream, binormOutStream;
			if( baseReader.normElem )
			{
				normOutStream.emplace( *baseReader.normElem, result.data(), vertexCount, vb.stride );
			}
			if( baseReader.tanElem )
			{
				tanOutStream.emplace( *baseReader.tanElem, result.data(), vertexCount, vb.stride );
			}
			if( baseReader.binormElem )
			{
				binormOutStream.emplace( *baseReader.binormElem, result.data(), vertexCount, vb.stride );
			}

			std::optional<cmf::BufferElementStream<Vector4>> pkdTanOutStream;
			if( baseReader.pkdTanElem )
			{
				pkdTanOutStream.emplace( *baseReader.pkdTanElem, result.data(), vertexCount, vb.stride );
			}
			else if( baseReader.pkdLegElem )
			{
				pkdTanOutStream.emplace( *baseReader.pkdLegElem, result.data(), vertexCount, vb.stride );
			}

			for( uint32_t i = 0; i < vertexCount; i++ )
			{
				posOutStream.set( i, positions[i] );

				if( baseReader.pkdTanElem )
				{
					Vector4 packed = cmf::PackTangents( cmf::TangentCompression::PackedTangent, normals[i], tangents[i], binormals[i] );
					pkdTanOutStream->set( i, packed );
				}
				else if( baseReader.pkdLegElem )
				{
					Vector4 packed = cmf::PackTangents( cmf::TangentCompression::PackedTangentLegacy, normals[i], tangents[i], binormals[i] );
					pkdTanOutStream->set( i, packed );
				}
				else
				{
					if( normOutStream )
					{
						normOutStream->set( i, normals[i] );
					}
					if( tanOutStream )
					{
						tanOutStream->set( i, tangents[i] );
					}
					if( binormOutStream )
					{
						binormOutStream->set( i, binormals[i] );
					}
				}
			}

			pVertexData.Update( result.data(), renderContext );
		}

		return true;
	}
#if WITH_GRANNY
	else
	{
		const granny_mesh* mesh = GetGrannyMesh( meshIx );
		if( !mesh )
		{
			CCP_LOGERR( "TriGrannyRes::BakeBlendshape: Invalid mesh index" );
			return false;
		}

		int vertexCount = mesh->PrimaryVertexData->VertexCount;
		granny_data_type_definition* vertexFormat = mesh->PrimaryVertexData->VertexType;
		unsigned int bytesPerVertex = GrannyGetTotalObjectSize( vertexFormat );
		if( deltaOnly )
		{
			if( vertexCount * 12 != vertexDataSize )
			{
				CCP_LOGERR( "TriGrannyRes::BakeBlendshape: Incorrect vertex buffer size" );
				return false;
			}
		}
		else
		{
			if( vertexCount * bytesPerVertex != vertexDataSize )
			{
				CCP_LOGERR( "TriGrannyRes::BakeBlendshape: Incorrect vertex buffer size" );
				return false;
			}
		}

		bool blendshapesFromAnnotations = false;
		unsigned int numBlends = mesh->MorphTargetCount;
		if( !numBlends )
		{
			numBlends = mesh->PrimaryVertexData->VertexAnnotationSetCount;
			blendshapesFromAnnotations = true;
		}

		if( numBlends == 0 )
		{
			//CCP_LOGWARN( "TriGrannyRes::BakeBlendshape: Attempted to apply blendshapes to a mesh that has none." );
			return true;
		}

		if( weights.size() != numBlends && !nameToWeight )
		{
			CCP_LOGERR( "TriGrannyRes::BakeBlendshape: Incorrect number of weights - %zu given, %d expected", weights.size(), numBlends );
			return false;
		}

		granny_data_type_definition* blendVertexFormat = nullptr;

		if( blendshapesFromAnnotations )
		{
			// Have to iterate over blendshapes until we find a vertex format - the first one might be empty
			for( unsigned int i = 0; i < numBlends; ++i )
			{
				blendVertexFormat = mesh->PrimaryVertexData->VertexAnnotationSets[i].VertexAnnotationType;
				if( blendVertexFormat )
				{
					break;
				}
			}
		}
		else
		{
			blendVertexFormat = mesh->MorphTargets[0].VertexData->VertexType;
		}

		// Copy base data from original vertex buffer. Deltas will be applied on top of this (if any are found)
		void* pSrc = GrannyGetMeshVertices( mesh );

		if( !blendVertexFormat )
		{
			pVertexData.Update( pSrc, renderContext );
			CCP_LOG( "BakeBlendshape called on %S but it has no blendshapes", GetPath() );
			return true;
		}

		// Copy into a temporary buffer; otherwise with every addition we're reading back from
		// pVertexData, which is a locked vertex buffer, so that could be really slow memory.
		std::vector<char> localVertexData;
		localVertexData.insert( localVertexData.end(), (char*)pSrc, (char*)pSrc + vertexDataSize );

		unsigned int blendBytesPerVertex = GrannyGetTotalObjectSize( blendVertexFormat );

		struct DatatypeInfo
		{
			DatatypeInfo() :
				offset( 0xffffffff ), isHalfPrecision( false )
			{
			}

			unsigned int offset;
			bool isHalfPrecision;
		};

		enum DatatypesOfInterest
		{
			DOI_POS,
			DOI_NORMAL,
			DOI_COUNT
		};

		DatatypeInfo typeInfos[DOI_COUNT];
		DatatypeInfo blendTypeInfos[DOI_COUNT];
		const char* typeInfoNames[DOI_COUNT] = { GrannyVertexPositionName, GrannyVertexNormalName };

		// Find offsets for base vert
		{
			int componentIx = 0;
			int offset = 0;
			while( vertexFormat[componentIx].Type != GrannyEndMember )
			{
				granny_data_type_definition& src = vertexFormat[componentIx];
				for( unsigned int ti = 0; ti < DOI_COUNT; ++ti )
				{
					if( strcmp( src.Name, typeInfoNames[ti] ) == 0 )
					{
						typeInfos[ti].offset = offset;
						if( src.Type == GrannyReal16Member )
						{
							typeInfos[ti].isHalfPrecision = true;
						}
					}
				}

				offset += GrannyGetMemberTypeSize( &src );
				++componentIx;
			}
		}

		// Offsets for blend vertex
		{
			int componentIx = 0;
			int offset = 0;
			while( blendVertexFormat[componentIx].Type != GrannyEndMember )
			{
				granny_data_type_definition& src = blendVertexFormat[componentIx];
				for( unsigned int ti = 0; ti < DOI_COUNT; ++ti )
				{
					if( strcmp( src.Name, typeInfoNames[ti] ) == 0 )
					{
						blendTypeInfos[ti].offset = offset;
						if( src.Type == GrannyReal16Member )
						{
							blendTypeInfos[ti].isHalfPrecision = true;
						}
					}
				}

				offset += GrannyGetMemberTypeSize( &src );
				++componentIx;
			}
		}

		// Apply the deltas from the blendshapes
		for( unsigned int j = 0; j < numBlends; ++j )
		{
			float weight_ = 0.0f;

			if( !nameToWeight )
			{
				weight_ = weights[j];
			}
			else if( blendshapesFromAnnotations )
			{
				std::string name = mesh->PrimaryVertexData->VertexAnnotationSets[j].Name;
				if( name.empty() )
				{
					continue;
				}
				int scan = (int)name.size() - 1;
				while( scan > 0 && isdigit( name[scan] ) )
				{
					--scan;
				}
				name.erase( scan + 1, name.size() - scan - 1 );
				std::transform( name.begin(), name.end(), name.begin(), tolower );
				NameToWeightMap::const_iterator it = nameToWeight->find( name );
				if( it != nameToWeight->end() )
				{
					weight_ = it->second;
				}
			}


			const float weight = weight_;

			if( weight < 1e-4f )
			{
				continue;
			}

			if( blendshapesFromAnnotations )
			{
				// Blendshapes reside in vertex annotations - data is only stored for non-zero vertices
				const int blendIndexCount = mesh->PrimaryVertexData->VertexAnnotationSets[j].VertexAnnotationIndexCount;
				if( !blendIndexCount )
				{
					continue;
				}

				const uint8_t* const pMorphVerts = mesh->PrimaryVertexData->VertexAnnotationSets[j].VertexAnnotations;
				const granny_int32* const blendIndices = mesh->PrimaryVertexData->VertexAnnotationSets[j].VertexAnnotationIndices;

				if( deltaOnly )
				{
					const uint8_t* __restrict pDelta = pMorphVerts + blendTypeInfos[DOI_POS].offset;
					const int* __restrict vertexIx = blendIndices;

					if( blendTypeInfos[DOI_POS].isHalfPrecision )
					{
						for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
						{
							uint8_t* __restrict pBase = reinterpret_cast<uint8_t*>( localVertexData.data() ) + *vertexIx * 12;

							Vector3 delta = *reinterpret_cast<const Vector3_16*>( pDelta );

							*reinterpret_cast<Vector3*>( pBase ) += weight * delta;
						}
					}
					else
					{
						for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
						{
							uint8_t* __restrict pBase = reinterpret_cast<uint8_t*>( localVertexData.data() ) + *vertexIx * 12;

							( reinterpret_cast<float*>( pBase ) )[0] += ( reinterpret_cast<const float*>( pDelta ) )[0] * weight;
							( reinterpret_cast<float*>( pBase ) )[1] += ( reinterpret_cast<const float*>( pDelta ) )[1] * weight;
							( reinterpret_cast<float*>( pBase ) )[2] += ( reinterpret_cast<const float*>( pDelta ) )[2] * weight;
						}
					}

					continue;
				}

				for( unsigned componentIx = 0; componentIx < DOI_COUNT; ++componentIx )
				{
					if( typeInfos[componentIx].offset == 0xffffffff || blendTypeInfos[componentIx].offset == 0xffffffff )
					{
						continue;
					}

					const uint8_t* __restrict pDelta = pMorphVerts + blendTypeInfos[componentIx].offset;
					const int* __restrict vertexIx = blendIndices;

					uint8_t* const __restrict pComponentBase = reinterpret_cast<uint8_t*>( &localVertexData[0] ) + typeInfos[componentIx].offset;

					if( typeInfos[componentIx].isHalfPrecision && blendTypeInfos[componentIx].isHalfPrecision )
					{
						for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
						{
							uint8_t* const __restrict pBase = pComponentBase + *vertexIx * bytesPerVertex;

							Vector3 base = *reinterpret_cast<const Vector3_16*>( pBase );
							Vector3 delta = *reinterpret_cast<const Vector3_16*>( pDelta );

							base += weight * delta;

							*reinterpret_cast<Vector3_16*>( pBase ) = Vector3_16( base );
						}
					}
					else if( typeInfos[componentIx].isHalfPrecision )
					{
						for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
						{
							uint8_t* const __restrict pBase = pComponentBase + *vertexIx * bytesPerVertex;

							Vector3 base = *reinterpret_cast<const Vector3_16*>( pBase );
							const Vector3& delta = *reinterpret_cast<const Vector3*>( pDelta );

							base += weight * delta;

							*reinterpret_cast<Vector3_16*>( pBase ) = Vector3_16( base );
						}
					}
					else if( blendTypeInfos[componentIx].isHalfPrecision )
					{
						for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
						{
							uint8_t* const __restrict pBase = pComponentBase + *vertexIx * bytesPerVertex;

							Vector3& base = *reinterpret_cast<Vector3*>( pBase );

							Vector3 delta = *reinterpret_cast<const Vector3_16*>( pDelta );

							base += weight * delta;
						}
					}
					else
					{
						for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
						{
							uint8_t* const __restrict pBase = pComponentBase + *vertexIx * bytesPerVertex;

							( reinterpret_cast<float*>( pBase ) )[0] += ( reinterpret_cast<const float*>( pDelta ) )[0] * weight;
							( reinterpret_cast<float*>( pBase ) )[1] += ( reinterpret_cast<const float*>( pDelta ) )[1] * weight;
							( reinterpret_cast<float*>( pBase ) )[2] += ( reinterpret_cast<const float*>( pDelta ) )[2] * weight;
						}
					}
				}
			}
			else
			{
				// Blendshapes are in the mesh morph data
				void* pMorphVerts = GrannyGetMeshMorphVertices( mesh, j );
				granny_int32x morphVertexCount = GrannyGetMeshMorphVertexCount( mesh, j );
				if( morphVertexCount != vertexCount )
				{
					CCP_LOGERR( "TriGrannyRes::BakeBlendshape: Vertex count of morph target doesn't match vertex count of base vertex data" );
					return false;
				}

				uint8_t* pMV = static_cast<uint8_t*>( pMorphVerts );

				if( deltaOnly )
				{
					uint8_t* __restrict pDst = reinterpret_cast<uint8_t*>( &localVertexData[0] );

					for( int i = 0; i < vertexCount; ++i )
					{
						uint8_t* pDelta = pMV + blendTypeInfos[DOI_POS].offset;

						if( blendTypeInfos[DOI_POS].isHalfPrecision )
						{
							Vector3 delta = *reinterpret_cast<const Vector3_16*>( pDelta );

							*reinterpret_cast<Vector3*>( pDst ) += weights[j] * delta;
						}
						else
						{
							*reinterpret_cast<Vector3*>( pDst ) += weights[j] * *reinterpret_cast<Vector3*>( pDelta );
						}

						pMV += blendBytesPerVertex;
						pDst += 12;
					}

					continue;
				}

				uint8_t* pDst = reinterpret_cast<uint8_t*>( &localVertexData[0] );

				for( int i = 0; i < vertexCount; ++i )
				{
					for( unsigned componentIx = 0; componentIx < DOI_COUNT; ++componentIx )
					{
						if( typeInfos[componentIx].offset == 0xffffffff )
						{
							continue;
						}

						uint8_t* pBase = pDst + typeInfos[componentIx].offset;
						uint8_t* pDelta = pMV + blendTypeInfos[componentIx].offset;

						if( typeInfos[componentIx].isHalfPrecision )
						{
							Vector3 base = *reinterpret_cast<const Vector3_16*>( pBase );
							Vector3 delta = *reinterpret_cast<const Vector3_16*>( pDelta );

							base += weights[j] * delta;

							*reinterpret_cast<Vector3_16*>( pBase ) = Vector3_16( base );
							*reinterpret_cast<Vector3_16*>( pDelta ) = Vector3_16( delta );
						}
						else
						{
							Vector3 delta = *reinterpret_cast<Vector3*>( pDelta );
							delta *= weights[j];
							*reinterpret_cast<Vector3*>( pBase ) += delta;
						}
					}

					pMV += blendBytesPerVertex;
					pDst += bytesPerVertex;
				}
			}
		}

		pVertexData.Update( localVertexData.data(), renderContext );

		return true;
	}
#else
	else
	{
		return false;
	}
#endif
}

int TriGrannyRes::GetModelCount()
{
	if( IsUsingCMF() )
	{
		return IsGood() && m_cmfContents ? (int)m_cmfContents.GetData()->skeletons.size() : 0;
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return 0;
		}

		return fi->AnimationCount;
	}
#else
	else
	{
		return 0;
	}
#endif
}

int TriGrannyRes::GetMeshCount()
{
	if( IsUsingCMF() )
	{
		return IsGood() && m_cmfContents ? (int)m_cmfContents.GetData()->meshes.size() : 0;
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return 0;
		}

		return fi->MeshCount;
	}
#else
	else
	{
		return 0;
	}
#endif
}

Be::Result<std::string> TriGrannyRes::ValidateMeshIndex( unsigned int meshIx )
{
	if( !IsGood() || !m_cmfContents )
	{
		return Be::Result<std::string>( "Tried to use an invalid CMF file" );
	}

	if( meshIx >= m_cmfContents.GetData()->meshes.size() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGrannyRes::GetMeshAreaCount( unsigned int meshIx, int& count )
{
	if( IsUsingCMF() )
	{
		auto errorMessage = ValidateMeshIndex( meshIx );
		if( !BeIsSuccess( errorMessage ) )
		{
			return errorMessage;
		}
		count = (int)m_cmfContents.GetData()->meshes[meshIx].areas.size();
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return Be::Result<std::string>( "Tried to get file info on an invalid Granny file" );
		}

		if( (granny_int32x)meshIx >= fi->MeshCount )
		{
			return Be::Result<std::string>( "Mesh index out of range" );
		}

		if( fi->Meshes[meshIx]->PrimaryTopology )
		{
			count = fi->Meshes[meshIx]->PrimaryTopology->GroupCount;
		}
		else
		{
			count = fi->Meshes[meshIx]->MaterialBindingCount;
		}
	}
#endif

	return Be::Result<std::string>();
}


Be::Result<std::string> TriGrannyRes::GetMeshName( unsigned int meshIx, std::string& name )
{
	if( IsUsingCMF() )
	{
		auto errorMessage = ValidateMeshIndex( meshIx );
		if( !BeIsSuccess( errorMessage ) )
		{
			return errorMessage;
		}
		name = cmf::ToStdString( m_cmfContents.GetData()->meshes[meshIx].name );
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return Be::Result<std::string>( "Tried to get file info on an invalid Granny file" );
		}

		if( (granny_int32x)meshIx >= fi->MeshCount )
		{
			return Be::Result<std::string>( "Mesh index out of range" );
		}

		name = fi->Meshes[meshIx]->Name ? fi->Meshes[meshIx]->Name : ""; // for reference, see CopyGrannyName
	}
#endif

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGrannyRes::GetMeshBoneBindings( unsigned int meshIx, std::vector<std::string>& boneBindings )
{
	if( IsUsingCMF() )
	{
		auto errorMessage = ValidateMeshIndex( meshIx );
		if( !BeIsSuccess( errorMessage ) )
		{
			return errorMessage;
		}
		boneBindings.clear();
		for( const auto& bone : m_cmfContents.GetData()->meshes[meshIx].boneBindings )
		{
			boneBindings.emplace_back( ToStdString( bone.name ) );
		}
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return Be::Result<std::string>( "Tried to get file info on an invalid Granny file" );
		}

		if( (granny_int32x)meshIx >= fi->MeshCount )
		{
			return Be::Result<std::string>( "Mesh index out of range" );
		}

		boneBindings.clear();
		for( granny_int32 i = 0; i < fi->Meshes[meshIx]->BoneBindingCount; i++ )
		{
			boneBindings.emplace_back( fi->Meshes[meshIx]->BoneBindings[i].BoneName );
		}
	}
#endif

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGrannyRes::GetMeshVertexElements( unsigned int meshIx, std::vector<std::pair<uint32_t, uint32_t>>& vertexElements )
{
	if( IsUsingCMF() )
	{
		auto errorMessage = ValidateMeshIndex( meshIx );
		if( !BeIsSuccess( errorMessage ) )
		{
			return errorMessage;
		}
		vertexElements.clear();
		auto decl = BuildFromCMFVertexDecl( m_cmfContents.GetData()->meshes[meshIx].decl );
		for( const auto& elem : decl.m_items )
		{
			vertexElements.emplace_back( static_cast<uint32_t>( elem.m_usage ), static_cast<uint32_t>( elem.m_usageIndex ) );
		}
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return Be::Result<std::string>( "Tried to get file info on an invalid Granny file" );
		}

		if( (granny_int32x)meshIx >= fi->MeshCount )
		{
			return Be::Result<std::string>( "Mesh index out of range" );
		}

		vertexElements.clear();
		auto decl = BuildFromGrannyVertexDecl( fi->Meshes[meshIx]->PrimaryVertexData->VertexType );
		for( const auto& elem : decl.m_items )
		{
			vertexElements.emplace_back( static_cast<uint32_t>( elem.m_usage ), static_cast<uint32_t>( elem.m_usageIndex ) );
		}
	}
#endif

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGrannyRes::GetMeshTriangleCount( unsigned int meshIx, uint32_t& count )
{
	count = 0;
	if( IsUsingCMF() )
	{
		auto errorMessage = ValidateMeshIndex( meshIx );
		if( !BeIsSuccess( errorMessage ) )
		{
			return errorMessage;
		}
		auto& mesh = m_cmfContents.GetData()->meshes[meshIx];
		if( !mesh.lods.empty() )
		{
			for( auto& lodArea : mesh.lods[0].areas )
			{
				count += lodArea.elementCount;
			}
		}
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return Be::Result<std::string>( "Tried to get file info on an invalid Granny file" );
		}

		if( (granny_int32x)meshIx >= fi->MeshCount )
		{
			return Be::Result<std::string>( "Mesh index out of range" );
		}

		if( fi->Meshes[meshIx]->PrimaryTopology )
		{
			for( int i = 0; i < fi->Meshes[meshIx]->PrimaryTopology->GroupCount; i++ )
			{
				count += fi->Meshes[meshIx]->PrimaryTopology->Groups[i].TriCount;
			}
		}
	}
#endif

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGrannyRes::GetMeshMorphCount( unsigned int meshIx, int& count )
{
	if( IsUsingCMF() )
	{
		auto errorMessage = ValidateMeshIndex( meshIx );
		if( !BeIsSuccess( errorMessage ) )
		{
			return errorMessage;
		}
		count = (int)m_cmfContents.GetData()->meshes[meshIx].morphTargets.targets.size();
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return Be::Result<std::string>( "Tried to get file info on an invalid Granny file" );
		}

		if( (granny_int32x)meshIx >= fi->MeshCount )
		{
			return Be::Result<std::string>( "Mesh index out of range" );
		}

		count = fi->Meshes[meshIx]->MorphTargetCount;
		if( !count )
		{
			count = fi->Meshes[meshIx]->PrimaryVertexData->VertexAnnotationSetCount;
		}
	}
#endif

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGrannyRes::GetMeshMorphName( unsigned int meshIx, unsigned int morphIx, std::string& name )
{
	if( IsUsingCMF() )
	{
		auto errorMessage = ValidateMeshIndex( meshIx );
		if( !BeIsSuccess( errorMessage ) )
		{
			return errorMessage;
		}

		if( morphIx >= m_cmfContents.GetData()->meshes[meshIx].morphTargets.targets.size() )
		{
			return Be::Result<std::string>( "Morph target index out of range" );
		}

		name = cmf::ToStdString( m_cmfContents.GetData()->meshes[meshIx].morphTargets.targets[morphIx].name );
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return Be::Result<std::string>( "Tried to get file info on an invalid Granny file" );
		}

		if( (granny_int32x)meshIx >= fi->MeshCount )
		{
			return Be::Result<std::string>( "Mesh index out of range" );
		}

		granny_mesh* mesh = fi->Meshes[meshIx];
		bool isMorphInAnnotation = false;
		unsigned int mtc = mesh->MorphTargetCount;
		if( !mtc )
		{
			mtc = mesh->PrimaryVertexData->VertexAnnotationSetCount;
			isMorphInAnnotation = true;
		}

		if( morphIx >= mtc )
		{
			return Be::Result<std::string>( "Morph target index out of range" );
		}

		if( isMorphInAnnotation )
		{
			name = mesh->PrimaryVertexData->VertexAnnotationSets[morphIx].Name;
		}
		else
		{
			name = mesh->MorphTargets[morphIx].ScalarName;
		}
	}
#endif

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGrannyRes::GetAllMeshMorphNamesNoDigits( unsigned int meshIx, std::vector<std::string>& names )
{
	if( IsUsingCMF() )
	{
		auto errorMessage = ValidateMeshIndex( meshIx );
		if( !BeIsSuccess( errorMessage ) )
		{
			return errorMessage;
		}

		names.resize( m_cmfContents.GetData()->meshes[meshIx].morphTargets.targets.size() );

		for( int32_t i = 0; i < m_cmfContents.GetData()->meshes[meshIx].morphTargets.targets.size(); i++ )
		{
			const auto& cmfName = m_cmfContents.GetData()->meshes[meshIx].morphTargets.targets[i].name;
			std::string name = cmf::ToStdString( cmfName );
			std::transform( name.begin(), name.end(), name.begin(), []( unsigned char c ) { return std::tolower( c ); } );
			name.erase( std::remove_if( name.begin(), name.end(), []( unsigned char c ) { return std::isdigit( c ); } ), name.end() );
			names[i] = name;
		}
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return Be::Result<std::string>( "Tried to get file info on an invalid Granny file" );
		}

		if( (granny_int32x)meshIx >= fi->MeshCount )
		{
			return Be::Result<std::string>( "Mesh index out of range" );
		}

		granny_mesh* mesh = fi->Meshes[meshIx];

		const bool isMorphInAnnotation = ( mesh->MorphTargetCount == 0 );
		const int mtc = isMorphInAnnotation ? mesh->PrimaryVertexData->VertexAnnotationSetCount : mesh->MorphTargetCount;

		names.resize( mtc );

		const unsigned maxLen = 1024;
		char buffer[maxLen];

		for( int i = 0; i != mtc; ++i )
		{
			const char* name;
			if( isMorphInAnnotation )
			{
				name = mesh->PrimaryVertexData->VertexAnnotationSets[i].Name;
			}
			else
			{
				name = mesh->MorphTargets[i].ScalarName;
			}

			const char* __restrict in = name;
			char* __restrict dst = buffer;
			const char* __restrict const max = buffer + maxLen - 1;
			while( dst != max && *in )
			{
				const char c = *in++;
				if( c >= '0' && c <= '9' )
				{
					continue;
				}
				if( c >= 'A' && c <= 'Z' )
				{
					*dst++ = c - 'A' + 'a';
				}
				else
				{
					*dst++ = c;
				}
			}
			*dst++ = 0;

			names[i] = buffer;
		}
	}
#endif

	return Be::Result<std::string>();
}

#if WITH_GRANNY
int TriGrannyRes::GetVectorTrackCount( int groupIdx )
{
	if( IsUsingCMF() )
	{
		CCP_ASSERT_M( false, "TriGrannyRes::GetVectorTrackCount: Not supported with CMF!" );
		return 0;
	}
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return 0;
		}

		if( groupIdx < fi->TrackGroupCount )
		{
			return fi->TrackGroups[groupIdx]->VectorTrackCount;
		}
		return 0;
	}
}

std::string TriGrannyRes::GetVectorTrackName( int groupIdx, int trackIdx )
{
	if( IsUsingCMF() )
	{
		CCP_ASSERT_M( false, "TriGrannyRes::GetVectorTrackName: Not supported with CMF!" );
		return "";
	}
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return "";
		}

		if( groupIdx < fi->TrackGroupCount )
		{
			if( trackIdx < fi->TrackGroups[groupIdx]->VectorTrackCount )
			{
				return fi->TrackGroups[groupIdx]->VectorTracks[trackIdx].Name;
			}
		}
		return "";
	}
}

int TriGrannyRes::GetEventTrackCount( int groupIdx )
{
	if( IsUsingCMF() )
	{
		CCP_ASSERT_M( false, "TriGrannyRes::GetEventTrackCount: Not supported with CMF!" );
		return 0;
	}
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return 0;
		}

		if( groupIdx < fi->TrackGroupCount )
		{
			return fi->TrackGroups[groupIdx]->TextTrackCount;
		}
		return 0;
	}
}

std::string TriGrannyRes::GetEventTrackName( int groupIdx, int trackIdx )
{
	if( IsUsingCMF() )
	{
		CCP_ASSERT_M( false, "TriGrannyRes::GetEventTrackName: Not supported with CMF!" );
		return "";
	}
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return "";
		}

		if( groupIdx < fi->TrackGroupCount )
		{
			if( trackIdx < fi->TrackGroups[groupIdx]->TextTrackCount )
			{
				return fi->TrackGroups[groupIdx]->TextTracks[trackIdx].Name;
			}
		}
		return "";
	}
}

int TriGrannyRes::GetTrackGroupCount()
{
	if( IsUsingCMF() )
	{
		CCP_ASSERT_M( false, "TriGrannyRes::GetTrackGroupCount: Not supported with CMF!" );
		return 0;
	}
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return 0;
		}

		return fi->TrackGroupCount;
	}
}

std::string TriGrannyRes::GetTrackGroupName( int groupIdx )
{
	if( IsUsingCMF() )
	{
		CCP_ASSERT_M( false, "TriGrannyRes::GetTrackGroupName: Not supported with CMF!" );
		return "";
	}
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return "";
		}

		if( groupIdx < fi->TrackGroupCount )
		{
			return fi->TrackGroups[groupIdx]->Name;
		}
		return "";
	}
}
#endif

int TriGrannyRes::GetAnimationCount()
{
	if( IsUsingCMF() )
	{
		if( !IsGood() || !m_cmfContents )
		{
			return 0;
		}

		return (int)m_cmfContents.GetData()->animations.size();
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return 0;
		}

		return fi->AnimationCount;
	}
#else
	else
	{
		return 0;
	}
#endif
}

std::string TriGrannyRes::GetAnimationName( int ix )
{
	if( IsUsingCMF() )
	{
		if( !IsGood() || !m_cmfContents )
		{
			return "";
		}

		if( ix < 0 || ix >= m_cmfContents.GetData()->animations.size() )
		{
			return "";
		}

		return cmf::ToStdString( m_cmfContents.GetData()->animations[ix].name );
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateAnimationIx( ix );
		if( !fi )
		{
			return "";
		}

		return fi->Animations[ix]->Name;
	}
#else
	else
	{
		return "";
	}
#endif
}

float TriGrannyRes::GetAnimationDuration( int ix )
{
	if( IsUsingCMF() )
	{
		if( !IsGood() || !m_cmfContents )
		{
			return 0.0f;
		}

		if( ix < 0 || ix >= m_cmfContents.GetData()->animations.size() )
		{
			return 0.0f;
		}

		return m_cmfContents.GetData()->animations[ix].duration;
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateAnimationIx( ix );
		if( !fi )
		{
			return 0.0f;
		}

		return fi->Animations[ix]->Duration;
	}
#else
	else
	{
		return 0.0f;
	}
#endif
}

#if WITH_GRANNY
granny_file_info* TriGrannyRes::ValidateAnimationIx( int ix )
{
	granny_file_info* fi = ValidateFileInfo();
	if( !fi )
	{
		return 0;
	}

	if( ( ix < 0 ) || ( ix >= fi->AnimationCount ) )
	{
		CCP_LOGERR( "Animation index out of bounds" );
		return NULL;
	}

	return fi;
}
#endif

#if WITH_GRANNY
granny_file_info* TriGrannyRes::ValidateFileInfo()
{
	if( !m_grannyFile )
	{
		CCP_LOGERR( "No Granny file loaded" );
		return 0;
	}

	granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );
	if( !fi )
	{
		CCP_LOGERR( "Invalid Granny file" );
		return 0;
	}

	return fi;
}
#endif

std::string TriGrannyRes::GetModelName( unsigned int ix )
{
	if( IsUsingCMF() )
	{
		if( !IsGood() || !m_cmfContents )
		{
			return "";
		}

		if( ix >= m_cmfContents.GetData()->skeletons.size() )
		{
			return "";
		}

		return cmf::ToStdString( m_cmfContents.GetData()->skeletons[ix].name );
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* fi = ValidateFileInfo();
		if( !fi )
		{
			return "";
		}

		if( ix >= (unsigned int)fi->ModelCount )
		{
			CCP_LOGERR( "Model index out of bounds" );
			return "";
		}

		return fi->Models[ix]->Name;
	}
#else
	else
	{
		return "";
	}
#endif
}

bool TriGrannyRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t TriGrannyRes::GetMemoryUsage()
{
	return m_memoryUsage;
}

Be::Result<std::string> TriGrannyRes::CreateGeometryRes( TriGeometryRes** result )
{
	TriGeometryResPtr p;
	p.CreateInstance();

	if( !p )
	{
		return Be::Result<std::string>( "Couldn't create an instance of TriGeometryRes" );
	}

	p->PrepareFromGrannyRes( this );

	*result = p.Detach();

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGrannyRes::BakeBlendshapeFromScript( unsigned int meshIx, const std::vector<float>& weights, TriGeometryRes* geom )
{
	if( IsUsingCMF() )
	{
		auto errorMessage = ValidateMeshIndex( meshIx );
		if( !BeIsSuccess( errorMessage ) )
		{
			return errorMessage;
		}
	}
#if WITH_GRANNY
	else
	{
		granny_file* gf = GetGrannyFile();
		if( !gf )
		{
			return Be::Result<std::string>( "No granny_file structure" );
		}

		granny_file_info* fi = GrannyGetFileInfo( gf );

		if( (granny_int32x)meshIx >= fi->MeshCount )
		{
			return Be::Result<std::string>( "Mesh index out of range" );
		}
	}
#endif
	USE_MAIN_THREAD_RENDER_CONTEXT();



	TriGeometryResLodData* lod = geom->GetMeshLod( meshIx, 0 );
	if( !lod )
	{
		return Be::Result<std::string>( "Trying to bake using geometryRes with NULL lod" );
	}
	if( !lod->m_allocationsValid )
	{
		return Be::Result<std::string>( "Trying to bake to a null vertex buffer" );
	}

	bool success = BakeBlendshape( meshIx, weights, lod->m_vertexAllocation, renderContext, lod->m_vertexCount * lod->m_mesh->m_bytesPerVertex );

	return success ? Be::Result<std::string>() : Be::Result<std::string>( " TriGrannyRes::BakeBlendshape encountered problems. " );
}

bool TriGrannyRes::IsUsingCMF() const
{
#if WITH_GRANNY != 1
	CCP_ASSERT_M( m_useCMF, "TriGrannyRes: Some place was about to take the granny code path, even though it's disabled! Make sure to use CMF!" );
	return true;
#endif
	return m_useCMF;
}

const cmf::Data* TriGrannyRes::GetCMFData() const
{
	return m_cmfContents.GetData();
}

const void* TriGrannyRes::GetCMFViewData( const cmf::BufferView& view )
{
	return m_cmfContents.GetViewData( view );
}

Tr2CmfContents& TriGrannyRes::GetCMFContents()
{
	return m_cmfContents;
}


bool RayTriangleIntersection( float& dist, float& u, float& v, const Vector3& pos, const Vector3& dir, const Vector3& p0, const Vector3& p1, const Vector3& p2 )
{
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

#if WITH_GRANNY
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

	return RayTriangleIntersection( dist, u, v, pos, dir, p0, p1, p2 );
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

Tr2GrannyIntersectionResultPtr GrannyRayIntersection( granny_file_info* fi, const Vector3& pos, const Vector3& dir, int32_t meshIndex, int32_t areaIndex )
{
	if( !fi )
	{
		return nullptr;
	}

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
}
}

Tr2GrannyIntersectionResultPtr GrannyRayIntersectionWrap( uintptr_t fileinfo, const Vector3& pos, const Vector3& dir, int32_t meshIndex, int32_t areaIndex )
{
	if( !fileinfo )
	{
		return nullptr;
	}

	auto fi = reinterpret_cast<granny_file_info*>( fileinfo );

	return GrannyRayIntersection( fi, pos, dir, meshIndex, areaIndex );
}
#endif

Tr2GrannyIntersectionResultPtr TriGrannyRes::RayIntersection( const Vector3& pos, const Vector3& dir, int32_t meshIndex, int32_t areaIndex )
{
	if( IsUsingCMF() )
	{
		if( !m_cmfContents )
		{
			return nullptr;
		}

		if( meshIndex >= (int32_t)m_cmfContents.GetData()->meshes.size() )
		{
			return nullptr;
		}
		if( areaIndex >= 0 && meshIndex < 0 )
		{
			return nullptr;
		}
		if( meshIndex >= 0 && areaIndex >= (int32_t)m_cmfContents.GetData()->meshes[meshIndex].areas.size() )
		{
			return nullptr;
		}

		uint32_t meshCount, meshOffset;
		if( meshIndex >= 0 )
		{
			meshCount = 1;
			meshOffset = meshIndex;
		}
		else
		{
			meshCount = (uint32_t)m_cmfContents.GetData()->meshes.size();
			meshOffset = 0;
		}

		Tr2GrannyIntersectionResult::Result result;
		float closestDist = std::numeric_limits<float>::max();
		bool foundIntersection = false;

		for( uint32_t meshIdx = 0; meshIdx < meshCount; meshIdx++ )
		{
			const auto& mesh = m_cmfContents.GetData()->meshes[meshIdx + meshOffset];

			auto ib = mesh.lods[0].ib;
			auto vb = mesh.lods[0].vb;
			auto decl = mesh.decl;

			auto ibSectionData = m_cmfContents.GetSection( ib.index );
			auto vbData = m_cmfContents.GetViewData( vb );

			const cmf::VertexElement* positionElement = cmf::FindElement( decl, cmf::Usage::Position );

			if( !positionElement )
			{
				continue;
			}

			if( mesh.topology != cmf::MeshTopology::TriangleList )
			{
				continue;
			}

			uint32_t numVerts = cmf::GetStreamElementCount( vb );

			cmf::ConstBufferElementStream<Vector3> positionStream( *positionElement, vbData, numVerts, vb.stride );
			std::optional<cmf::ConstBufferElementStream<Vector3>> normalStream;
			std::optional<cmf::ConstBufferElementStream<Vector2>> texCoordStream;
			std::optional<cmf::ConstBufferElementStream<std::array<uint32_t, 4>>> boneIndicesStream;

			if( auto normalElement = cmf::FindElement( decl, cmf::Usage::Normal ) )
			{
				normalStream = cmf::ConstBufferElementStream<Vector3>( *normalElement, vbData, numVerts, vb.stride );
			}
			if( auto texCoordElement = cmf::FindElement( decl, cmf::Usage::TexCoord ) )
			{
				texCoordStream = cmf::ConstBufferElementStream<Vector2>( *texCoordElement, vbData, numVerts, vb.stride );
			}
			if( auto boneIndicesElement = cmf::FindElement( decl, cmf::Usage::BoneIndices ) )
			{
				boneIndicesStream = cmf::ConstBufferElementStream<std::array<uint32_t, 4>>( *boneIndicesElement, vbData, numVerts, vb.stride );
			}

			uint32_t areaCount, areaOffset;
			if( areaIndex >= 0 )
			{
				areaCount = 1;
				areaOffset = areaIndex;
			}
			else
			{
				areaCount = (uint32_t)mesh.areas.size();
				areaOffset = 0;
			}

			auto indices = cmf::ConstIndexBufferStream( ibSectionData, ib );

			for( uint32_t areaIdx = 0; areaIdx < areaCount; areaIdx++ )
			{
				auto area = mesh.lods[0].areas[areaIdx + areaOffset];

				for( uint32_t elementIdx = area.firstElement; elementIdx < area.firstElement + area.elementCount; elementIdx++ )
				{
					uint32_t triangleIndices[3];
					triangleIndices[0] = indices[3 * elementIdx + 0];
					triangleIndices[1] = indices[3 * elementIdx + 1];
					triangleIndices[2] = indices[3 * elementIdx + 2];

					Vector3 p0 = positionStream[triangleIndices[0]];
					Vector3 p1 = positionStream[triangleIndices[1]];
					Vector3 p2 = positionStream[triangleIndices[2]];

					float u, v, dist;
					if( RayTriangleIntersection( dist, u, v, pos, dir, p0, p1, p2 ) )
					{
						if( dist < closestDist )
						{
							closestDist = dist;
							result.meshIndex = meshIdx + meshOffset;
							result.areaIndex = areaIdx + areaOffset;

							result.position = p0 + ( p1 - p0 ) * u + ( p2 - p0 ) * v;
							result.hasPosition = true;

							if( normalStream.has_value() )
							{
								Vector3 n0 = normalStream.value()[triangleIndices[0]];
								Vector3 n1 = normalStream.value()[triangleIndices[1]];
								Vector3 n2 = normalStream.value()[triangleIndices[2]];
								result.normal = Normalize( n0 + ( n1 - n0 ) * u + ( n2 - n0 ) * v );
								result.hasNormal = true;
							}
							else
							{
								result.normal = Vector3( 0, 0, 0 );
								result.hasNormal = false;
							}
							if( texCoordStream.has_value() )
							{
								Vector2 t0 = texCoordStream.value()[triangleIndices[0]];
								Vector2 t1 = texCoordStream.value()[triangleIndices[1]];
								Vector2 t2 = texCoordStream.value()[triangleIndices[2]];
								result.uv = t0 + ( t1 - t0 ) * u + ( t2 - t0 ) * v;
								result.hasUv = true;
							}
							else
							{
								result.uv = Vector2( 0, 0 );
								result.hasUv = false;
							}
							if( boneIndicesStream.has_value() )
							{
								result.boneIndex = boneIndicesStream.value()[triangleIndices[0]][0];
								result.hasBoneIndex = true;
							}
							else
							{
								result.boneIndex = -1;
								result.hasBoneIndex = false;
							}

							foundIntersection = true;
						}
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
	}
#if WITH_GRANNY
	else
	{
		return GrannyRayIntersection( GrannyGetFileInfo( m_grannyFile ), pos, dir, meshIndex, areaIndex );
	}
#endif

	return nullptr;
}
