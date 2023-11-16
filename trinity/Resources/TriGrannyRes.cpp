#include "StdAfx.h"
#include "TriGrannyRes.h"
#include "TriGeometryRes.h"

#include "Utilities/GeometryUtils.h"


IBlueResource* CreateGrannyResource( const wchar_t* name )
{
	TriGrannyResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"gr2raw", CreateGrannyResource );


Tr2GrannyIntersectionResult::Tr2GrannyIntersectionResult( IRoot* )
{
}

Tr2GrannyIntersectionResult::Result::Result()
	:position( 0, 0, 0 ),
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
	m_grannyFile( NULL ),
	m_data( NULL ),
	m_dataSize( 0 ),
	m_grannyArena( NULL ),
	m_memoryUsage( 0 ),
	m_allMaterialStringsDictionary( nullptr )
{
}

TriGrannyRes::~TriGrannyRes()
{
    if( m_grannyFile )
    {
        GrannyFreeFile( m_grannyFile );
    }

	if( m_grannyArena )
	{
		// The granny dynamic memory management
		GrannyFreeMemoryArena( m_grannyArena );
	}
}

BlueAsyncRes::LoadingResult TriGrannyRes::DoLoad()
{
	CCP_STATS_ZONE( __FUNCTION__ );

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
	

	return LR_SUCCESS;
}

bool TriGrannyRes::DoPrepare()
{
    return true;
}

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

int TriGrannyRes::GetVertexSize( int meshIx ) const
{
	const granny_data_type_definition* vertexFormat = GetGrannyVertexType( meshIx );
	if( !vertexFormat )
	{
		return 0;
	}
	return GrannyGetTotalObjectSize( vertexFormat );
}

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

int TriGrannyRes::GetVertexComponentOffset( int meshIx, const char* componentName ) const
{
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

bool TriGrannyRes::BakeBlendshape( unsigned int meshIx, const std::vector<float>& weights  , void* pVertexData, unsigned int vertexDataSize )
{
	return BakeBlendshape( meshIx, weights, pVertexData, vertexDataSize, NULL, false );
}

bool TriGrannyRes::BakeBlendshape( unsigned int meshIx, const NameToWeightMap& nameToWeight, void* pVertexData, unsigned int vertexDataSize )
{
	std::vector<float> dummyWeights;
	return BakeBlendshape( meshIx, dummyWeights, pVertexData, vertexDataSize, &nameToWeight, false );
}

bool TriGrannyRes::BakeBlendshape( unsigned int meshIx, const std::vector<float>& weights, void* pVertexData, unsigned int vertexDataSize, const NameToWeightMap* const nameToWeight, bool deltaOnly )
{
	CCP_STATS_ZONE( __FUNCTION__ );

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
		CCP_LOGERR( "TriGrannyRes::BakeBlendshape: Incorrect number of weights - %d given, %d expected", weights.size(), numBlends );
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
		memcpy( pVertexData, pSrc, vertexDataSize );
		CCP_LOG( "BakeBlendshape called on %S but it has no blendshapes", GetPath() );
		return true;
	}

	// Copy into a temporary buffer; otherwise with every addition we're reading back from
	// pVertexData, which is a locked vertex buffer, so that could be really slow memory.
	std::vector<char> localVertexData;
	if( !deltaOnly )
	{
		localVertexData.insert( localVertexData.end(), (char*)pSrc, (char*)pSrc + vertexDataSize );
	}

	unsigned int blendBytesPerVertex = GrannyGetTotalObjectSize( blendVertexFormat );

	struct DatatypeInfo
	{
		DatatypeInfo() : offset( 0xffffffff ), isHalfPrecision( false ) {}

		unsigned int offset;
		bool isHalfPrecision;
	};

	enum DatatypesOfInterest
	{
		DOI_POS, DOI_NORMAL, DOI_COUNT
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
			int scan = (int)name.size()-1;
			while( scan > 0 && isdigit( name[scan] ) )
			{
				--scan;
			}
			name.erase( scan+1, name.size()-scan-1 );
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
				const uint8_t* __restrict pDelta = pMorphVerts + blendTypeInfos[ DOI_POS ].offset;
				const int * __restrict vertexIx = blendIndices;

				if( blendTypeInfos[ DOI_POS ].isHalfPrecision )
				{
					for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
					{
						uint8_t* __restrict pBase = static_cast<uint8_t*>( pVertexData ) + *vertexIx * 12;

						Vector3 delta = *reinterpret_cast<const Vector3_16*>( pDelta );

						*reinterpret_cast<Vector3*>( pBase ) += weight * delta;
					}
				}
				else
				{
					for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
					{
						uint8_t* __restrict pBase = static_cast<uint8_t*>( pVertexData ) + *vertexIx * 12;

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
				const int * __restrict vertexIx = blendIndices;

				uint8_t* const __restrict pComponentBase = reinterpret_cast<uint8_t*>( &localVertexData[0] ) + typeInfos[componentIx].offset;

				if( typeInfos[ componentIx ].isHalfPrecision && blendTypeInfos[ componentIx ].isHalfPrecision )
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
				else
				if( typeInfos[ componentIx ].isHalfPrecision )
				{
					for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
					{
						uint8_t* const __restrict pBase = pComponentBase + *vertexIx * bytesPerVertex;					

						Vector3 base = *reinterpret_cast<const Vector3_16*>( pBase );
						const Vector3 &delta = *reinterpret_cast<const Vector3*>( pDelta );
						
						base += weight * delta;

						*reinterpret_cast<Vector3_16*>( pBase ) = Vector3_16( base );
					}
				}
				else
				if( blendTypeInfos[ componentIx ].isHalfPrecision )
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
				uint8_t* __restrict pDst = static_cast<uint8_t*>( pVertexData );

				for( int i = 0; i < vertexCount; ++i )
				{
					uint8_t* pDelta = pMV + blendTypeInfos[ DOI_POS ].offset;

					if( blendTypeInfos[ DOI_POS ].isHalfPrecision )
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

	if( !localVertexData.empty() )
	{
		memcpy( pVertexData, &localVertexData[0], localVertexData.size() );
	}

	return true;
}

int TriGrannyRes::GetModelCount()
{
	granny_file_info* fi = ValidateFileInfo();
	if( !fi )
	{
		return 0;
	}

	return fi->AnimationCount;
}

int TriGrannyRes::GetMeshCount()
{
	granny_file_info* fi = ValidateFileInfo();
	if( !fi )
	{
		return 0;
	}

	return fi->MeshCount;
}


Be::Result<std::string> TriGrannyRes::GetMeshAreaCount( unsigned int meshIx, int& count )
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

	return Be::Result<std::string>();
}


Be::Result<std::string> TriGrannyRes::GetMeshName( unsigned int meshIx, std::string& name )
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

	name = fi->Meshes[meshIx]->Name;

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGrannyRes::GetMeshMorphCount( unsigned int meshIx, int& count )
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

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGrannyRes::GetMeshMorphName( unsigned int meshIx, unsigned int morphIx, std::string& name )
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

	return Be::Result<std::string>();
}

Be::Result<std::string> TriGrannyRes::GetAllMeshMorphNamesNoDigits( unsigned int meshIx, std::vector<std::string>& names )
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
			name = mesh->PrimaryVertexData->VertexAnnotationSets[ i ].Name;
		}
		else
		{
			name = mesh->MorphTargets[ i ].ScalarName;
		}

		const char* __restrict in  = name;
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

	return Be::Result<std::string>();
}

int TriGrannyRes::GetTransformTrackCount( int groupIdx )
{
	granny_file_info* fi = ValidateFileInfo();
	if( !fi )
	{
		return 0;
	}
	
	if( groupIdx < fi->TrackGroupCount )
	{
		return fi->TrackGroups[groupIdx]->TransformTrackCount;
	}
	return 0;
}

std::string TriGrannyRes::GetTransformTrackName( int groupIdx, int trackIdx )
{
	granny_file_info* fi = ValidateFileInfo();
	if( !fi )
	{
		return "";
	}

	if( groupIdx < fi->TrackGroupCount  )
	{
		if( trackIdx < fi->TrackGroups[groupIdx]->TransformTrackCount )
		{
			return fi->TrackGroups[groupIdx]->TransformTracks[trackIdx].Name;
		}		
	}
	return "";
}

int TriGrannyRes::GetVectorTrackCount( int groupIdx )
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

std::string TriGrannyRes::GetVectorTrackName( int groupIdx, int trackIdx )
{
	granny_file_info* fi = ValidateFileInfo();
	if( !fi )
	{
		return "";
	}

	if( groupIdx < fi->TrackGroupCount  )
	{
		if( trackIdx < fi->TrackGroups[groupIdx]->VectorTrackCount )
		{
			return fi->TrackGroups[groupIdx]->VectorTracks[trackIdx].Name;
		}		
	}
	return "";
}

int TriGrannyRes::GetEventTrackCount( int groupIdx )
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

std::string TriGrannyRes::GetEventTrackName( int groupIdx, int trackIdx )
{
	granny_file_info* fi = ValidateFileInfo();
	if( !fi )
	{
		return "";
	}

	if( groupIdx < fi->TrackGroupCount  )
	{
		if( trackIdx < fi->TrackGroups[groupIdx]->TextTrackCount )
		{
			return fi->TrackGroups[groupIdx]->TextTracks[trackIdx].Name;
		}		
	}
	return "";
}

int TriGrannyRes::GetTrackGroupCount( )
{
	granny_file_info* fi = ValidateFileInfo();
	if( !fi )
	{
		return 0;
	}

	return fi->TrackGroupCount;
}

std::string TriGrannyRes::GetTrackGroupName( int groupIdx )
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


int TriGrannyRes::GetAnimationCount()
{
	granny_file_info* fi = ValidateFileInfo();
	if( !fi )
	{
		return 0;
	}

	return fi->AnimationCount;
}

std::string TriGrannyRes::GetAnimationName( int ix )
{
	granny_file_info* fi = ValidateAnimationIx(ix);
	if( !fi )
	{
		return "";
	}

	return fi->Animations[ix]->Name;
}

float TriGrannyRes::GetAnimationDuration( int ix )
{
	granny_file_info* fi = ValidateAnimationIx(ix);
	if( !fi )
	{
		return 0.0f;
	}

	return fi->Animations[ix]->Duration;
}

granny_file_info* TriGrannyRes::ValidateAnimationIx( int ix )
{
	granny_file_info* fi = ValidateFileInfo();
	if( !fi )
	{
		return 0;
	}

	if( (ix < 0) || (ix >= fi->AnimationCount) )
	{
		CCP_LOGERR( "Animation index out of bounds" );
		return NULL;
	}

	return fi;
}

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

std::string TriGrannyRes::GetModelName( unsigned int ix )
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

bool TriGrannyRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t TriGrannyRes::GetMemoryUsage()
{
	return m_memoryUsage;
}


#if BLUE_WITH_PYTHON
// This is currently geared toward Maya materials, where things are stored in top level maps, and then extended data
// max might require more or less data to be looked at, but the python user won't care as much, as this turns it all into one string/value pair list (Dict)

GrannyMaterialWrapper::GrannyMaterialWrapper(granny_material * gmat, unsigned int meshIndex, unsigned int areaIndex, PyObject * flatStringDictionary)
{

	m_meshIndex = meshIndex;
	m_areaIndex = areaIndex;

	m_name.assign( gmat->Name );

	m_dictionary = PyDict_New();

	// look at maps...
	if (gmat->MapCount>0)
	{
		unsigned int mapIndex;

		for (mapIndex=0;mapIndex< (unsigned int)(gmat->MapCount);mapIndex++)
		{
			// example maps at this level would be stuff like "color", and "normalCamera"

			// mesh->materialbindings[x]->maps[y]->material->extendeddata..

			granny_material_map * materialMap = &gmat->Maps[mapIndex];
			granny_material * gmap = materialMap->Material;

			granny_data_type_definition *defs = gmap->ExtendedData.Type;
			char * dataPtr = (char*)gmap->ExtendedData.Object;

			// This is a bit of a hack, but making it more flexible for this one case is a matter of debate. 
			// Maya is inconsistent about where it's texture paths are, but only for normal maps, diffuse and specular (and 3dsmax) are all one level deep.
			// in this case, the maya specific string "normalCamera" will have it's data rerouted by this forced pointer path.
			if ( strcmp(materialMap->Usage, "normalCamera" ) == 0 )
			{
				// in this one case, the actual file path is buried down a strange path.
				if ( materialMap->Material->MapCount == 1 )
				{
					defs = materialMap->Material->Maps[0].Material->ExtendedData.Type;
					dataPtr = (char*)materialMap->Material->Maps[0].Material->ExtendedData.Object;
				}
			}

			while(defs->Type != GrannyEndMember)
			{
				if (defs->Type==GrannyStringMember)
				{
					char ** ptr = (char**)dataPtr;
					char * targetString = ptr[0];

					char tmpbuf[512];
					// create an amalgam of the usage string ("color" or "normalCamera"), and add on the string type name "fileTextureName"
					sprintf_s(tmpbuf,"%s_%s", materialMap->Usage, defs->Name);
					
					// set the actual value of this dict entry to the file path.
					PyObject * matString = PyString_FromString(targetString);
					// set into the dict.
					PyDict_SetItemString(m_dictionary, tmpbuf, matString);
					
					sprintf_s(tmpbuf,"%u_%u_%s_%s", meshIndex, areaIndex, materialMap->Usage, defs->Name);
					PyDict_SetItemString(flatStringDictionary, tmpbuf, matString);
					// Adding all strings (used to be a break here) 
				}

				dataPtr += GrannyGetMemberTypeSize(defs);
				defs++;
			}

		}
	}

	// check to see if extended data exists before diving the pointer
	// gmat->ExtendedData is a Struct of {Object void *, Type granny_data_type_def*}

	if (gmat->ExtendedData.Object == 0)
		return;
	
	granny_data_type_definition * defs = gmat->ExtendedData.Type;

	// may not have any types.
	if (defs == 0)
		return;

	uint8_t* dataPtr = static_cast<uint8_t*>( gmat->ExtendedData.Object );

	while(defs->Type != GrannyEndMember)
	{

		if (defs->Type == GrannyReal32Member)
		{
			float floatVal = *reinterpret_cast<float*>( dataPtr );
			double v = double( floatVal );
			PyObject * pythonFloat = PyFloat_FromDouble(v);
			// set into the dict.
			PyDict_SetItemString(m_dictionary, defs->Name, pythonFloat);
		}
		dataPtr += GrannyGetMemberTypeSize(defs);
		defs++;
	}



}
#endif


void TriGrannyRes::CollectGrannyMaterials()
{
	
#if BLUE_WITH_PYTHON
	m_allMaterialStringsDictionary = PyDict_New();
	
	granny_file_info* fileInfo = GrannyGetFileInfo( m_grannyFile );
	if( !fileInfo )
	{
		return;
	}

	if (fileInfo->MeshCount > 0)
	{
		unsigned int meshIndex;
		for (meshIndex=0;meshIndex<(unsigned int)(fileInfo->MeshCount);meshIndex++)
		{

			granny_mesh * mesh = fileInfo->Meshes[meshIndex];

			if (mesh->PrimaryTopology->GroupCount>0) // Groups become 'areas'?
			{
				unsigned int groupIndex;

				for (groupIndex=0;groupIndex<(unsigned int)(mesh->PrimaryTopology->GroupCount);groupIndex++)
				{
					granny_tri_material_group * group = &mesh->PrimaryTopology->Groups[groupIndex];

					unsigned int materialIndex = group->MaterialIndex;

					granny_material * gmat = mesh->MaterialBindings[materialIndex].Material;

					if (gmat)
					{
						GrannyMaterialWrapper * wrapper = CCP_NEW("grannyres/material") GrannyMaterialWrapper(gmat, meshIndex, groupIndex, m_allMaterialStringsDictionary);

						unsigned int key = (meshIndex << 16) | groupIndex;

						m_meshAreaMaterials[key]=wrapper;
					}

				}
			}
		}
	}

#endif
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
	USE_MAIN_THREAD_RENDER_CONTEXT();

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


	TriGeometryResMeshData* meshData = geom->GetMeshData( meshIx );
	if( !meshData )
	{
		return Be::Result<std::string>( "Trying to bake using geometryRes with NULL meshData" );
	}
	if( !meshData->m_vertexBuffer.IsValid() )
	{
		return Be::Result<std::string>( "Trying to bake to a null vertex buffer" );
	}

	void* pVertexData = NULL;
	HRESULT hr = meshData->m_vertexBuffer.MapForWriting( pVertexData, renderContext );
	if( FAILED( hr ) )
	{
		return Be::Result<std::string>( "Failed to lock vertex buffer" );
	}

	bool success = BakeBlendshape( meshIx, weights, pVertexData, meshData->m_vertexCount * meshData->m_bytesPerVertex );
	meshData->m_vertexBuffer.UnmapForWriting( renderContext );

	return success ? Be::Result<std::string>() : Be::Result<std::string>( " TriGrannyRes::BakeBlendshape encountered problems. ");
}

