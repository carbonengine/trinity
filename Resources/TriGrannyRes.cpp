#include "StdAfx.h"
#include "TriGrannyRes.h"
#include "TriGeometryRes.h"

#if INTERIORS_ENABLED
#include "UmbraTypes.h"
#include "TriEnlightenUtils.h"
#include "TriEnlightenStream.h"

#endif

#include "Utilities/GeometryUtils.h"

extern bool g_geometryResNormalizeOnLoad;
extern bool g_outputEnlightenDebugBuildInfo;
extern float g_grannyWarnLoadTime;

granny_data_type_definition TriPackedGeometryDataType[] = {
	{ GrannyReferenceToArrayMember, "m_buffer", GrannyUInt8Type },
	{ GrannyReal32Member, "m_pixelSize", GrannyReal32Type },
	{ GrannyUInt32Member, "m_geometryHash1", GrannyUInt32Type },
	{ GrannyUInt32Member, "m_geometryHash2", GrannyUInt32Type },
	{ GrannyEndMember }
};

IBlueResource* CreateGrannyResource( const wchar_t* name )
{
	TriGrannyResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"gr2raw", CreateGrannyResource );

TriGrannyRes::TriGrannyRes( IRoot* lockobj ) : 
	m_grannyFile( NULL ),
	m_data( NULL ),
	m_dataSize( 0 ),
	m_grannyArena( NULL ),
	m_memoryUsage( 0 )
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

#if INTERIORS_ENABLED
Umbra::MeshModel* TriGrannyRes::CreateUmbraMeshFromGrannyMesh( granny_mesh* mesh, bool clockwise )
{
	CCP_STATS_ZONE( __FUNCTION__ );

    int vertexCount = mesh->PrimaryVertexData->VertexCount;

    Vector3* verts = CCP_NEW( "CreateUmbraMeshFromGrannyMesh/verts" ) Vector3[vertexCount];
    GrannyCopyMeshVertices( mesh, GrannyP3VertexType, verts );

    int indexCount = mesh->PrimaryTopology->IndexCount;
    if( indexCount == 0 )
    {
        indexCount = mesh->PrimaryTopology->Index16Count;
    }
    int* indices = CCP_NEW( "CreateUmbraMeshFromGrannyMesh/indices") int[indexCount];
    GrannyCopyMeshIndices( mesh, 4, indices );

    Umbra::MeshModel* result = Umbra::MeshModel::create( reinterpret_cast<Umbra::Vector3*>( verts ), reinterpret_cast<Umbra::Vector3i*>( indices ), vertexCount, indexCount / 3, clockwise );

    CCP_DELETE[] indices;
    CCP_DELETE[] verts;
	
	return result;
}
#endif

bool TriGrannyRes::DoOpenStream()
{
    BePaths->GetStreamFromPathW( GetPath(), &m_dataStream );

    if( !m_dataStream )
    {
        return false;
    }

    return true;
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

		BeTimer t;

		CCP_STATS_ZONE( __FUNCTION__ " reading Granny file" );
		m_grannyFile = ProtectedGrannyReadEntireFileFromMemory( m_path.c_str(), (uint32_t)m_dataSize, m_data );

		const float secs = (float)t.GetSeconds();
		if( secs > g_grannyWarnLoadTime )
		{
			CCP_LOGWARN( "TriGrannyRes - GrannyRead '%S' took %f seconds", GetPath(), secs );
		}
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

	if( g_geometryResNormalizeOnLoad && fi->ArtToolInfo )
	{
		NormalizeGrannyFile( fi );
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

void TriGrannyRes::DoCloseStream()
{
	if( m_dataStream )
	{
		m_dataStream->UnlockData();
		m_data = NULL;
		m_dataStream = nullptr;
	}
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

granny_member_type TriGrannyRes::GetVertexComponentType( int meshIx, const char* componentName ) const
{
	const granny_mesh* mesh = GetGrannyMesh( meshIx );
	if( !mesh )
	{
		CCP_LOGERR( "TriGrannyRes::GetVertexComponentType: Invalid mesh index" );
		return GrannyEndMember;
	}

	// now scan granny's vertex-declaration for the bone index part and count the offsets
	granny_data_type_definition* vertexFormat = mesh->PrimaryVertexData->VertexType;
	int componentIx = 0;
	while( vertexFormat[componentIx].Type != GrannyEndMember )
	{
		granny_data_type_definition& src = vertexFormat[componentIx];
		if( strcmp( src.Name, componentName ) == 0 )
		{
			// found it!
			return src.Type;
		}
		// next
		++componentIx;
	}
	return GrannyEndMember;
}

const granny_morph_target* TriGrannyRes::GetBlendshape( int meshIx, const char* blendshapeName ) const
{
	const granny_mesh* mesh = GetGrannyMesh( meshIx );
	if( !mesh )
	{
		CCP_LOGERR( "TriGrannyRes::GetBlendshape: Invalid mesh index" );
		return NULL;
	}

	for( int i = 0; i < mesh->MorphTargetCount; ++i )
	{
		if( strcmp( blendshapeName, mesh->MorphTargets[i].ScalarName ) == 0 )
		{
			return &mesh->MorphTargets[i];
		}
	}
	return NULL;
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

bool TriGrannyRes::GetBlendDeltas( unsigned meshIx, const std::vector<float>& weights, std::vector<float>& deltaXyz )
{
	const granny_mesh* mesh = GetGrannyMesh( meshIx );
	if( !mesh )
	{
		CCP_LOGERR( "TriGrannyRes::GetBlendDeltas: Invalid mesh index" );
		return false;
	}

	unsigned vertexCount = (unsigned)mesh->PrimaryVertexData->VertexCount;
	if( !vertexCount )
	{
		return false;
	}

	deltaXyz.resize( vertexCount * 3 );

	return BakeBlendshape( meshIx, weights, &deltaXyz[0], (unsigned int)deltaXyz.size() * sizeof( deltaXyz[0] ), NULL, true );
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

	granny_data_type_definition* blendVertexFormat;

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
						const uint8_t* const __restrict pBase = (uint8_t*)pVertexData + *vertexIx * 12;

						Vector3 delta;
						D3DXFloat16To32Array( (float*)&delta, (const D3DXFLOAT16*)pDelta, 3 );

						*(Vector3*)pBase += weight * delta;
					}
				}
				else
				{
					for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
					{
						const uint8_t* const __restrict pBase = (uint8_t*)pVertexData + *vertexIx * 12;

						((float*)pBase)[0] += ((float*)pDelta)[0] * weight;
						((float*)pBase)[1] += ((float*)pDelta)[1] * weight;
						((float*)pBase)[2] += ((float*)pDelta)[2] * weight;
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

				const uint8_t* const __restrict pComponentBase = (uint8_t*)&localVertexData[0] + typeInfos[componentIx].offset;

				if( typeInfos[ componentIx ].isHalfPrecision && blendTypeInfos[ componentIx ].isHalfPrecision )
				{
					for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
					{
						const uint8_t* const __restrict pBase = pComponentBase + *vertexIx * bytesPerVertex;					

						Vector3 base;
						Vector3 delta;

						D3DXFloat16To32Array( (float*)&base,  (const D3DXFLOAT16*)pBase, 3 );
						D3DXFloat16To32Array( (float*)&delta, (const D3DXFLOAT16*)pDelta, 3 );

						base += weight * delta;

						D3DXFloat32To16Array( (D3DXFLOAT16*)pBase, (float*)&base, 3 );
					}
				}
				else
				if( typeInfos[ componentIx ].isHalfPrecision )
				{
					for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
					{
						const uint8_t* const __restrict pBase = pComponentBase + *vertexIx * bytesPerVertex;					

						Vector3 base;
						const Vector3 &delta = *(Vector3*)pDelta;

						D3DXFloat16To32Array( (float*)&base, (const D3DXFLOAT16*)pBase, 3 );
						
						base += weight * delta;

						D3DXFloat32To16Array( (D3DXFLOAT16*)pBase, (float*)&base, 3 );
					}
				}
				else
				if( blendTypeInfos[ componentIx ].isHalfPrecision )
				{
					for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
					{
						const uint8_t* const __restrict pBase = pComponentBase + *vertexIx * bytesPerVertex;					

						Vector3& base = *(Vector3*)pBase;

						Vector3 delta;
						D3DXFloat16To32Array( (float*)&delta, (const D3DXFLOAT16*)pDelta, 3 );

						base += weight * delta;
					}
				}
				else
				{
					for( int i = 0; i < blendIndexCount; ++i, ++vertexIx, pDelta += blendBytesPerVertex )
					{
						const uint8_t* const __restrict pBase = pComponentBase + *vertexIx * bytesPerVertex;

						((float*)pBase)[0] += ((float*)pDelta)[0] * weight;
						((float*)pBase)[1] += ((float*)pDelta)[1] * weight;
						((float*)pBase)[2] += ((float*)pDelta)[2] * weight;
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

			uint8_t* pMV = (uint8_t*)pMorphVerts;

			if( deltaOnly )
			{
				const uint8_t* __restrict pDst = (uint8_t*)pVertexData;

				for( int i = 0; i < vertexCount; ++i )
				{
					uint8_t* pDelta = pMV + blendTypeInfos[ DOI_POS ].offset;

					if( blendTypeInfos[ DOI_POS ].isHalfPrecision )
					{
						Vector3 delta;
						D3DXFloat16To32Array( (float*)&delta, (const D3DXFLOAT16*)pDelta, 3 );

						*(Vector3*)pDst += weights[j] * delta;
					}
					else
					{
						*(Vector3*)pDst += weights[j] * *(Vector3*)pDelta;
					}

					pMV += blendBytesPerVertex;
					pDst += 12;
				}

				continue;
			}

			uint8_t* pDst = (uint8_t*)&localVertexData[0];

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
						Vector3 base;
						Vector3 delta;

						D3DXFloat16To32Array( (float*)&base, (const D3DXFLOAT16*)pBase, 3 );
						D3DXFloat16To32Array( (float*)&delta, (const D3DXFLOAT16*)pDelta, 3 );

						base += weights[j] * delta;

						D3DXFloat32To16Array( (D3DXFLOAT16*)pBase, (float*)&base, 3 );
						D3DXFloat32To16Array( (D3DXFLOAT16*)pDelta, (float*)&delta, 3 );
					}
					else
					{
						Vector3 delta = *(Vector3*)pDelta;
						delta *= weights[j];
						*(Vector3*)pBase += delta;
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

bool TriGrannyRes::GetVertexPositions( unsigned meshIx, std::vector<float>& xyz )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	const granny_mesh* mesh = GetGrannyMesh( meshIx );
	if( !mesh )
	{
		CCP_LOGERR( "TriGrannyRes::GetVertexPositions: Invalid mesh index" );
		return false;
	}

	unsigned vertexCount = (unsigned)mesh->PrimaryVertexData->VertexCount;
	granny_data_type_definition* vertexFormat = mesh->PrimaryVertexData->VertexType;
	const unsigned bytesPerVertex = GrannyGetTotalObjectSize( vertexFormat );
	
	void* pSrc = GrannyGetMeshVertices( mesh );

	xyz.resize( vertexCount * 3 );

	struct DatatypeInfo
	{
		DatatypeInfo() : offset( 0xffffffff ), isHalfPrecision( false ) {}

		unsigned int offset;
		bool isHalfPrecision;
	};

	enum DatatypesOfInterest
	{
		DOI_POS, DOI_COUNT
	};

	DatatypeInfo typeInfos[DOI_COUNT];
	const char* typeInfoNames[DOI_COUNT] = { GrannyVertexPositionName };

	// Find offsets for base vert
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

	if( typeInfos[DOI_POS].offset == 0xffffffff )
	{
		return false;
	}

	const uint8_t* __restrict pComponentBase = (uint8_t*)pSrc + typeInfos[ DOI_POS ].offset;

	if( typeInfos[ DOI_POS ].isHalfPrecision )
	{
		float* out = &xyz[0];
		for( unsigned i = 0; i < vertexCount; ++i, pComponentBase += bytesPerVertex )
		{
			Vector3 base;
			D3DXFloat16To32Array( (float*)&base,  (const D3DXFLOAT16*)pComponentBase, 3 );
			*out++ = base.x;
			*out++ = base.y;
			*out++ = base.z;
		}
	}
	else
	{
		float* out = &xyz[0];
		for( unsigned i = 0; i < vertexCount; ++i, pComponentBase += bytesPerVertex )
		{
			float* in = (float*)pComponentBase;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
		}
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

	count = fi->Meshes[meshIx]->MaterialBindingCount;

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

bool TriGrannyRes::SaveToGr2( const std::string& path )
{
	// Verify that we have a granny file
	if( !m_grannyFile )
	{
		CCP_LOGERR( "TriGrannyRes::SaveToGr2: Object has no Granny file" );
		return false;
	}

	// Verify that the file has file-info
	granny_file_info* info = GrannyGetFileInfo( m_grannyFile );
	if( info == NULL )
	{
		CCP_LOGERR( "TriGrannyRes::SaveToGr2: Granny file has no file info" );
		return false;
	}

	// Write the info out to the file
	granny_file_builder *builder = GrannyBeginFile( 1, GrannyCurrentGRNStandardTag,
		GrannyGRNFileMV_32Bit_LittleEndian,
		GrannyGetTemporaryDirectory(),
		"GRNFileTemp" );
	granny_file_data_tree_writer *writer =
		GrannyBeginFileDataTreeWriting( GrannyFileInfoType, info, 0, 0 );
	GrannyWriteDataTreeToFileBuilder( writer, builder );
	GrannyEndFileDataTreeWriting( writer );

	std::wstring pathW = (const wchar_t*)CA2W( path.c_str() );
	std::wstring fullPath = BePaths->ResolvePathForWritingW( pathW );
	bool result = GrannyEndFile( builder, CW2A( fullPath.c_str() ) );

	return result;
}

bool TriGrannyRes::CreateShadowMesh( )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Verify that we have a granny file
	if( !m_grannyFile )
	{
		CCP_LOGERR( "TriGrannyRes::CreateShadowMesh: Object has no Granny file" );
		return false;
	}

	// Verify that the file has file-info
	granny_file_info* info = GrannyGetFileInfo( m_grannyFile );
	if( info == NULL )
	{
		CCP_LOGERR( "TriGrannyRes::CreateShadowMesh: Granny file has no file info" );
		return false;
	}

	// Give the new mesh a nice name
	const char* name = "shadow_area";
	granny_uint8* vertices = NULL;
	granny_int32* indices = NULL;
	granny_uint16* indices16 = NULL;
	granny_int32x numVertices = 0;
	granny_int32x numIndices = 0;

	// Start by counting all the verts and indices for the final shadow mesh
	for( granny_int32x meshIx = 0; meshIx < info->MeshCount; ++meshIx )
	{
		granny_mesh* mesh = info->Meshes[meshIx];
		// We dont want the target mesh
		if( mesh == NULL || ( NULL != strstr( mesh->Name, "Target_" ) ) || ( NULL != strstr( mesh->Name, "target_" ) )  )
		{
			continue;
		}
		
		if( NULL != strstr( mesh->Name, name ) )
		{
			CCP_LOGERR( "TriGrannyRes::CreateShadowMesh: Granny file already has a shadow mesh" );
			return false;
		}

		granny_int32x const meshIndexCount  = GrannyGetMeshIndexCount( mesh );
		granny_int32x const meshVertexCount = GrannyGetMeshVertexCount( mesh );
	
		numVertices += meshVertexCount;
		numIndices += meshIndexCount;
	}	

	if( m_grannyArena == NULL )
	{
		m_grannyArena = GrannyNewMemoryArena();	
	}

	granny_mesh* shadowMesh = ( granny_mesh* )GrannyMemoryArenaPush( m_grannyArena, sizeof( granny_mesh ) );
	memset( shadowMesh, 0, sizeof( granny_mesh ) );

	granny_tri_topology* topology = ( granny_tri_topology* )GrannyMemoryArenaPush( m_grannyArena, sizeof( granny_tri_topology ) );
	memset( topology, 0, sizeof( granny_tri_topology ) );

	granny_vertex_data* vertexData = ( granny_vertex_data* )GrannyMemoryArenaPush( m_grannyArena, sizeof( granny_vertex_data ) );
	memset( vertexData, 0, sizeof( granny_vertex_data ) );	

	shadowMesh->Name = ( char* )GrannyMemoryArenaPush( m_grannyArena, strlen( name )+1 );
	memset( ( void* )shadowMesh->Name, 0, strlen( name )+1 );	
	strcpy_s( ( char* )shadowMesh->Name, strlen( name )+1, name );

	vertices = ( granny_uint8* )GrannyMemoryArenaPush( m_grannyArena, sizeof( granny_pt32_vertex ) * numVertices );
	indices = ( granny_int32* )malloc( numIndices*sizeof( granny_int32 ) );

	// Create a single group for all the triangles
	granny_tri_material_group* group = ( granny_tri_material_group* )GrannyMemoryArenaPush( m_grannyArena, sizeof( granny_tri_material_group ) );
	group->MaterialIndex = 0;
	group->TriFirst = 0;
	group->TriCount = numIndices/3;
	topology->GroupCount = 1;
	topology->Groups = group;
		
	// reset the counters
	numVertices = 0;
	numIndices = 0;
	
	// Change the vertex declarations and copy to our final buffer
	for( granny_int32x meshIx = 0; meshIx < info->MeshCount; ++meshIx )
	{
		granny_mesh* mesh = info->Meshes[meshIx];
		if( mesh == NULL || ( NULL != strstr( mesh->Name, "Target_" ) ) || ( NULL != strstr( mesh->Name, "target_" ) ) )
		{
			continue;
		}

		granny_int32x const meshIndexCount  = GrannyGetMeshIndexCount( mesh );
		granny_int32x const meshVertexCount = GrannyGetMeshVertexCount( mesh );

		// Convert the vertex layout
		granny_uint8* tempVerts = ( granny_uint8* )malloc( sizeof( granny_pt32_vertex ) * meshVertexCount );

        GrannyConvertVertexLayouts( meshVertexCount,
                                   GrannyGetMeshVertexType( mesh ),
                                   GrannyGetMeshVertices( mesh ),
                                   GrannyPT32VertexType,
                                   tempVerts );			

		// move the new verts to our final vertex buffer
		memcpy( vertices + (sizeof( granny_pt32_vertex )*numVertices), tempVerts, meshVertexCount*sizeof( granny_pt32_vertex ) );			
		free( tempVerts );

		// need to change all the indices since all the vertices go into the same buffer
		for( int i = 0; i < meshIndexCount; i++ )
		{
			if( mesh->PrimaryTopology->Indices )
			{
				indices[i+numIndices] = mesh->PrimaryTopology->Indices[i] + numVertices;
			}			
			else
			{
				indices[i+numIndices] = mesh->PrimaryTopology->Indices16[i] + numVertices;
			}
		}

		numVertices += meshVertexCount;
		numIndices += meshIndexCount;
	}
	
	// if the verts are below this number we only need 16 bits for each index
	if( numVertices <= 65535 )
	{	
		indices16 = ( granny_uint16* )GrannyMemoryArenaPush( m_grannyArena, numIndices*sizeof( granny_uint16 ) );
		for( int i = 0; i < numIndices; i++ )
		{		
			indices16[i] = ( granny_int16 )indices[i];
		}
		
		topology->Indices16 = indices16;
		topology->Index16Count = numIndices;
	}
	else
	{
		granny_int32* arenaIndices = ( granny_int32* )GrannyMemoryArenaPush( m_grannyArena, numIndices*sizeof( granny_int32 ) );
		memcpy( arenaIndices, indices, numIndices*sizeof( granny_int32 ) );
		topology->Indices = arenaIndices;
		topology->IndexCount = numIndices;
	}	
	// free the temp index memory
	free( indices );

	vertexData->Vertices = vertices;
	vertexData->VertexCount = numVertices;
	vertexData->VertexType = GrannyPT32VertexType;

	// Setup the mesh with all the trimmings
	shadowMesh->PrimaryVertexData = vertexData;
	shadowMesh->PrimaryTopology = topology;

	// Material and bone data is just ripped from the first mesh
	shadowMesh->MaterialBindingCount = info->Meshes[0]->MaterialBindingCount;
	shadowMesh->MaterialBindings = info->Meshes[0]->MaterialBindings;

    shadowMesh->BoneBindingCount = 1;
    shadowMesh->BoneBindings = ( granny_bone_binding* )GrannyMemoryArenaPush( m_grannyArena, sizeof( granny_bone_binding ) );
    memset( shadowMesh->BoneBindings, 0, sizeof( granny_bone_binding ) );
    shadowMesh->BoneBindings[0].BoneName = info->Meshes[0]->BoneBindings[0].BoneName;

	// Add the new shadow mesh to our info
	granny_mesh** meshes = ( granny_mesh** )GrannyMemoryArenaPush( m_grannyArena, ( info->MeshCount+1 )*sizeof( granny_mesh* ) );
	memcpy( meshes, info->Meshes, info->MeshCount*sizeof( granny_mesh* ) );
	meshes[info->MeshCount] = shadowMesh;
	info->MeshCount++;
	info->Meshes = meshes;

	// Add the vertex data to the info
	granny_vertex_data** vertexDatas = ( granny_vertex_data** )GrannyMemoryArenaPush( m_grannyArena, ( info->VertexDataCount+1 )*sizeof( granny_vertex_data* ) );
	memcpy( vertexDatas, info->VertexDatas, info->VertexDataCount*sizeof( granny_vertex_data* ) );
	vertexDatas[info->VertexDataCount] = vertexData;
	info->VertexDataCount++;
	info->VertexDatas = vertexDatas;	

	// add the indices and groups to the info
	granny_tri_topology** topologies = ( granny_tri_topology** )GrannyMemoryArenaPush( m_grannyArena, ( info->TriTopologyCount+1 )*sizeof( granny_tri_topology* ) );
	memcpy( topologies, info->TriTopologies, info->TriTopologyCount*sizeof( granny_tri_topology* ) );
	topologies[info->TriTopologyCount] = topology;
	info->TriTopologyCount++;
	info->TriTopologies = topologies;	

	return true;
}

bool TriGrannyRes::ReorderEnlightenMeshes( void )
{
	// Verify that we have a granny file
	if( !m_grannyFile )
	{
		CCP_LOGERR( "TriGrannyRes::ReorderEnlightenMeshes: Object has no Granny file" );
		return false;
	}

	// Verify that the file has file-info
	granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );
	if( fi == NULL )
	{
		CCP_LOGERR( "TriGrannyRes::ReorderEnlightenMeshes: Granny file has no file info" );
		return false;
	}

	// Loop over the meshes and find the target mesh
	granny_int32x meshIx = 0;
	bool bFound = false;
	while( !bFound && meshIx < fi->MeshCount )
	{
		granny_mesh* mesh = fi->Meshes[meshIx];

		// TODO - Make this comparison case-insensitive?
		if( (NULL != strstr( mesh->Name, "Target_" )) || (NULL != strstr( mesh->Name, "target_" )) )
		{
			// Found the target mesh, so set the found flag
			bFound = true;
		}
		else
		{
			// Otherwise increment the index to continue the search
			++meshIx;
		}
	}
	
	// If the target mesh is already at index 0, just return true
	if( meshIx == 0 )
	{
		return true;
	}

	// If we found the target mesh, swap it into the first position in the array
	if( bFound )
	{
		granny_mesh* pTemp = fi->Meshes[0];
		fi->Meshes[0] = fi->Meshes[meshIx];
		fi->Meshes[meshIx] = pTemp;

		return true;
	}
	// Report a warning if there was no target mesh
	else
	{
		CCP_LOGWARN( "TriGrannyRes::ReorderEnlightenMeshes: Could not find a target mesh (mesh name should begin with \'Target_\')" );
		return false;
	}
}

void GetPosition( granny_uint8* pVerts, int vIx, bool isPositionHalfPrecision, int bytesPerVertex, granny_real32 * pos )
{
	if( isPositionHalfPrecision )
	{
		for( int i = 0; i < 3; ++i )
		{
			GrannyReal16ToReal32( ((granny_real16*)(pVerts + bytesPerVertex * vIx))[i], &pos[i] );
		}
	}
	else
	{
		for( int i = 0; i < 3; ++i )
		{
			pos[i] = ((granny_real32*)(pVerts + bytesPerVertex * vIx))[i];
		}
	}
}

void GetTexCoord( granny_uint8* pVerts, int vIx, bool isPositionHalfPrecision, int bytesPerVertex, granny_real32* uv )
{
	if( isPositionHalfPrecision )
	{
		GrannyReal16ToReal32( ((granny_real16*)(pVerts + bytesPerVertex * vIx))[0], &uv[0] );
		GrannyReal16ToReal32( ((granny_real16*)(pVerts + bytesPerVertex * vIx))[1], &uv[1] );
	}
	else
	{
		uv[0] = ((granny_real32*)(pVerts + bytesPerVertex * vIx))[0];
		uv[1] = ((granny_real32*)(pVerts + bytesPerVertex * vIx))[1];
	}
}

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// ---------------------------------------------------------------------------------------
// Description:
//   Creates Enlighten packed geometry and saves resulting granny file.
// Arguments:
//   filename - the path to the output granny file.
//   guid - the desired unique id of the packed geometry.
//   forceRebuild - force a rebuild of the packed geometry regardless.
//   enlightenPixelSize - the size of each enlighten texel in meters. Must be consistent 
//                        with the system that will use it.
// Return Value:
//   true If successfully created packed geometry
//   false If there was an error when creating packed geometry
// ---------------------------------------------------------------------------------------
bool TriGrannyRes::CreateEnlightenPackedGeometry( const std::string& filename, 
												  unsigned guid, 
												  bool forceRebuild /*= false*/, 
												  float enlightenPixelSize /*= 1.0f */ )
{
	if( !m_grannyFile )
	{
		CCP_LOGERR( "TriGrannyRes::CreateEnlightenPackedGeometry: Object has no Granny file" );
		return false;
	}

	granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );
	if( fi == NULL )
	{
		CCP_LOGERR( "TriGrannyRes::CreateEnlightenPackedGeometry: Granny file has no file info" );
		return false;
	}

	if( fi->ExtendedData.Object && !forceRebuild )
	{
		TriPackedGeometryData* p = (TriPackedGeometryData*)fi->ExtendedData.Object;
		if( p->GetVersion() == TriPackedGeometryData::s_versionNumber )
		{
			CCP_LOGERR( "This file already contains packed geometry" );
			return false;
		}
	}

	Enlighten::IPrecompute* pPrecompute = Enlighten::CreatePrecompute();

	if( g_outputEnlightenDebugBuildInfo)
	{
		_mkdir( "C:\\EnlightenDebug" );
		CCP_LOGWARN( "Dumping Enlighten Precompute state to C:\\EnlightenDebug");
		pPrecompute->SetStateDumpFolder( L"C:\\EnlightenDebug" );
		pPrecompute->SetStateDump( pPrecompute->esdAll );
	}

	ON_BLOCK_EXIT( &Enlighten::IPrecompute::Release, pPrecompute );

	if( m_grannyArena == NULL )
	{
		m_grannyArena = GrannyNewMemoryArena();	
	}

	// The conversion from our units (m) to theirs (cm)
	const float enlightenUnitScale = 0.01f;
	// Always follows this formula
	const float enlightenRadiosityPerPixelSurfaceArea = pow( enlightenPixelSize, 2.0f );

	CCP_LOG( "Creating new precompute input geometry with id: %i, %i",1,guid);
	Enlighten::IPrecompInputGeometry* geom = Enlighten::IPrecompInputGeometry::Create( Geo::GeoGuid::Create( 1, guid ) );

	// This bit of the code will eventually need to be moved
	CCP_LOG( "Creating UV Charts" );
	for( granny_int32x meshIx = 0; meshIx < fi->MeshCount; ++meshIx)
	{
		// Add mesh to the geometry
		Enlighten::PrecompMeshProperties precomputeMeshProperties;
		if( meshIx == 0 )
		{
			// Note - these flags are the default, but we'll set them again just to be explicit.
			precomputeMeshProperties.m_IsDirectLightingMesh = true;		// we can illuminate it
			precomputeMeshProperties.m_IsIndirectLightingMesh = true;	// it receives radiosity
			precomputeMeshProperties.m_IsTargetMesh = true;				// it doesn't do any mesh simplification (from the sample)
		}
		else
		{
			// This is a detail mesh
			precomputeMeshProperties.m_IsDirectLightingMesh = false;	// we can illuminate it
			precomputeMeshProperties.m_IsIndirectLightingMesh = true;	// it receives radiosity
			precomputeMeshProperties.m_IsTargetMesh = false;			// it doesn't do any mesh simplification (from the sample)
		}

		Enlighten::IPrecompInputMesh* precomputeMesh = CreateEnlightenInputMesh( meshIx, fi, false );
		if( precomputeMesh )
		{
			geom->AddMesh( precomputeMesh, &precomputeMeshProperties );
		}
		else
		{
			return false;
		}
	}

	geom->SetRadiosityPerPixelSurfaceArea( enlightenRadiosityPerPixelSurfaceArea );

	// The progessbar seems to be somewhat poorly named, it's a progress observer
	TriEnlightenProgressBar prog;

	// PHASE 1: Only really requires the geometry
	// This will need to be redone for different build parameters, along with all the following steps for anything that uses this geometry.
	// Save out the packed geometry here for re-use to make sure it matches the UVs we build in this step
	Enlighten::IPrecompPackedGeometry* packedGeometry = NULL;
	//ScopeGuard packedGeometryReleaseGuard = MakeGuard(&Enlighten::IPrecompPackedGeometry::Release, packedGeometry);
	CCP_LOG("Packing Geometry.");
	Geo::s32 packResult = pPrecompute->PackGeometry( geom, &prog, packedGeometry );

	if( !packedGeometry || (packResult != 0) )
	{
		if( packResult == -1 )
		{
			CCP_LOGERR( "PackGeometry failed on mesh - Invalid mesh input" );
		}
		else if( packResult == -2 )
		{
			CCP_LOGERR( "PackGeometry failed on mesh - Invalid mesh properties" );
		}
		else if( packResult == -3 )
		{
			CCP_LOGERR( "PackGeometry failed on mesh - Precompute failure" );
		}
		else if( packResult == -4 )
		{
			CCP_LOGERR( "PackGeometry failed on mesh - Build empty" );
		}

		return false;
	}
	//////////////////////////////////////////////////////////////////////////
	// Splice in the Final UVs?
	//////////////////////////////////////////////////////////////////////////
	for( granny_int32x meshIx = 0; meshIx < fi->MeshCount; ++meshIx)
	{
		granny_mesh* Mesh = fi->Meshes[meshIx];
				
		if( !Mesh )
		{
			continue;
		}

		bool isHalfPrecision = ( Mesh->PrimaryVertexData->VertexType[0].Type == GrannyReal16Member );

		const Geo::GeoPoint2* outputUVs = packedGeometry->GetOutputUvArray( meshIx );
		void* originalVBufferContents = Mesh->PrimaryVertexData->Vertices;
		int uvOffset2 = GetVertexComponentOffset( meshIx, GrannyVertexTextureCoordinatesName "1" );
		int bytesPerVertex = GrannyGetTotalObjectSize( Mesh->PrimaryVertexData->VertexType );
		for( int i = 0; i < Mesh->PrimaryVertexData->VertexCount; ++i )
		{
			if( isHalfPrecision )
			{
				granny_real16* const uvPtr = (granny_real16*)((uint8_t*)originalVBufferContents +(i*bytesPerVertex) + uvOffset2);
				*uvPtr = GrannyReal32ToReal16( outputUVs[i].U );
				*(uvPtr+1) = GrannyReal32ToReal16( outputUVs[i].V );
			}
			else
			{
				granny_real32* const uvPtr = (granny_real32*)((uint8_t*)originalVBufferContents +(i*bytesPerVertex) + uvOffset2);
				*uvPtr = outputUVs[i].U;
				*(uvPtr+1) = outputUVs[i].V;
			}
		}
	}	

	GeoMemoryStream geoStream = GeoMemoryStream();
	geoStream.Write( &TriPackedGeometryData::s_versionNumber, sizeof( TriPackedGeometryData::s_versionNumber ), 1 );
	packedGeometry->Save( geoStream );

	TriPackedGeometryData PackedData;
	PackedData.m_buffer = geoStream.m_buffer;
	PackedData.m_bufferSize = geoStream.m_size;
	PackedData.m_pixelSize = enlightenPixelSize;
	SetGeometryHashesFromDatetimeAndGuid( PackedData.m_geometryBuildHash1, PackedData.m_geometryBuildHash2, guid );

	fi->ExtendedData.Type = TriPackedGeometryDataType;
	fi->ExtendedData.Object = &PackedData;

	granny_file_builder *Builder = GrannyBeginFile( 1, GrannyCurrentGRNStandardTag,
		GrannyGRNFileMV_32Bit_LittleEndian,
		GrannyGetTemporaryDirectory(),
		"GRNFileTemp");
	granny_file_data_tree_writer *Writer =
		GrannyBeginFileDataTreeWriting(GrannyFileInfoType, fi, 0, 0);
	GrannyWriteDataTreeToFileBuilder(Writer, Builder);
	GrannyEndFileDataTreeWriting(Writer);

	std::wstring filenameW = CA2W( filename.c_str() );
	std::wstring fullPath = BePaths->ResolvePathForWritingW( filenameW );
	bool result = GrannyEndFile( Builder, CW2A( fullPath.c_str() ) );

	return result;

}

// ---------------------------------------------------------------------------------------
// Description:
//   Projects Enlighten detail geometry onto a target geometry in another granny resource 
//   and saves resulting granny file.
// Arguments:
//   filename - the path to the output granny file.
//   targetGeometry - granny resource containing target geometry (assumed to be a mesh 
//                    with index 0).
//   guid - the desired unique id of the projected geometry.
//   forceRebuild - force a rebuild of the projected geometry regardless.
//   enlightenPixelSize - the size of each enlighten texel in meters. Must be consistent 
//                        with the system that will use it.
// Return Value:
//   true If successfully created projected geometry
//   false If there was an error when creating projected geometry
// ---------------------------------------------------------------------------------------
bool TriGrannyRes::ProjectEnlightenGeometry( const std::string& filename, 
											 TriGrannyRes &targetGeometry, 
											 unsigned guid, 
											 bool forceRebuild, 
											 float enlightenPixelSize )
{
	if( !m_grannyFile )
	{
		CCP_LOGERR( "TriGrannyRes::ProjectEnlightenGeometry: Object has no Granny file" );
		return false;
	}

	granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );
	if( fi == NULL )
	{
		CCP_LOGERR( "TriGrannyRes::ProjectEnlightenGeometry: Granny file has no file info" );
		return false;
	}

	if( fi->ExtendedData.Object && !forceRebuild )
	{
		TriPackedGeometryData* p = (TriPackedGeometryData*)fi->ExtendedData.Object;
		if( p->GetVersion() == TriPackedGeometryData::s_versionNumber )
		{
			CCP_LOGERR( "This file already contains packed geometry" );
			return false;
		}
	}

	granny_file_info* targetFi = GrannyGetFileInfo( targetGeometry.m_grannyFile );
	if( targetFi == NULL )
	{
		CCP_LOGERR( "TriGrannyRes::ProjectEnlightenGeometry: Target granny file has no file info" );
		return false;
	}

	Enlighten::IPrecompute* pPrecompute = Enlighten::CreatePrecompute();

#if !defined( NDEBUG )
	_mkdir( "C:\\EnlightenDebug" );
	CCP_LOGWARN( "Dumping Enlighten Precompute state to C:\\EnlightenDebug");
	pPrecompute->SetStateDumpFolder( L"C:\\EnlightenDebug" );
	pPrecompute->SetStateDump( pPrecompute->esdAll );
#endif

	ON_BLOCK_EXIT( &Enlighten::IPrecompute::Release, pPrecompute );

	if( m_grannyArena == NULL )
	{
		m_grannyArena = GrannyNewMemoryArena();	
	}

	// The conversion from our units (m) to theirs (cm)
	const float enlightenUnitScale = 0.01f;
	// Always follows this formula
	const float enlightenRadiosityPerPixelSurfaceArea = pow( enlightenPixelSize, 2.0f );

	CCP_LOG( "Creating new precompute input geometry with id: %i, %i",1,guid);
	Enlighten::IPrecompInputGeometry* geom = Enlighten::IPrecompInputGeometry::Create( Geo::GeoGuid::Create( 1, guid ) );

	// This bit of the code will eventually need to be moved
	CCP_LOG( "Creating UV Charts" );

	// First mesh is taget mesh from targetGeometry file
	{
		Enlighten::PrecompMeshProperties precomputeMeshProperties;
		// Note - these flags are the default, but we'll set them again just to be explicit.
		precomputeMeshProperties.m_IsDirectLightingMesh = true;		// we can illuminate it
		precomputeMeshProperties.m_IsIndirectLightingMesh = true;	// it receives radiosity
		precomputeMeshProperties.m_IsTargetMesh = true;				// it doesn't do any mesh simplification (from the sample)
		Enlighten::IPrecompInputMesh* precomputeMesh = targetGeometry.CreateEnlightenInputMesh( 0, targetFi, true );
		if( precomputeMesh == NULL )
		{
			CCP_LOGERR( "TriGrannyRes::ProjectEnlightenGeometry: failed to get target mesh from target geometry file" );
			return false;
		}
		geom->AddMesh( precomputeMesh, &precomputeMeshProperties );
	}

	for( granny_int32x meshIx = 0; meshIx < fi->MeshCount; ++meshIx)
	{
		Enlighten::IPrecompInputMesh* precomputeMesh;
		Enlighten::PrecompMeshProperties precomputeMeshProperties;

		// This is a detail mesh
		precomputeMeshProperties.m_IsDirectLightingMesh = false;	// we can illuminate it
		precomputeMeshProperties.m_IsIndirectLightingMesh = true;	// it receives radiosity
		precomputeMeshProperties.m_IsTargetMesh = false;			// it doesn't do any mesh simplification (from the sample)
		precomputeMesh = CreateEnlightenInputMesh( meshIx, fi, false );

		if( precomputeMesh )
		{
			geom->AddMesh( precomputeMesh, &precomputeMeshProperties );
		}
	}

	geom->SetRadiosityPerPixelSurfaceArea( enlightenRadiosityPerPixelSurfaceArea );

	// The progessbar seems to be somewhat poorly named, it's a progress observer
	TriEnlightenProgressBar prog;

	// PHASE 1: Only really requires the geometry
	// This will need to be redone for different build parameters, along with all the following steps for anything that uses this geometry.
	// Save out the packed geometry here for re-use to make sure it matches the UVs we build in this step
	Enlighten::IPrecompPackedGeometry* packedGeometry = NULL;
	//ScopeGuard packedGeometryReleaseGuard = MakeGuard(&Enlighten::IPrecompPackedGeometry::Release, packedGeometry);
	CCP_LOG("Projecting Geometry.");
	Geo::s32 packResult = pPrecompute->ProjectGeometry( geom, &prog, packedGeometry );

	if( !packedGeometry || (packResult != 0) )
	{
		if( packResult == -1 )
		{
			CCP_LOGERR( "ProjectGeometry failed on mesh - Invalid mesh input" );
		}
		else if( packResult == -2 )
		{
			CCP_LOGERR( "ProjectGeometry failed on mesh - Invalid mesh properties" );
		}
		else if( packResult == -3 )
		{
			CCP_LOGERR( "ProjectGeometry failed on mesh - Precompute failure" );
		}
		else if( packResult == -4 )
		{
			CCP_LOGERR( "ProjectGeometry failed on mesh - Build empty" );
		}

		return false;
	}
	//////////////////////////////////////////////////////////////////////////
	// Splice in the Final UVs?
	//////////////////////////////////////////////////////////////////////////
	for( granny_int32x meshIx = 0; meshIx < fi->MeshCount; ++meshIx)
	{
		granny_mesh* Mesh = fi->Meshes[meshIx];
				
		if( !Mesh )
		{
			continue;
		}

		bool isHalfPrecision = ( Mesh->PrimaryVertexData->VertexType[0].Type == GrannyReal16Member );

		const Geo::GeoPoint2* outputUVs = packedGeometry->GetOutputUvArray( meshIx + 1 );
		void* originalVBufferContents = Mesh->PrimaryVertexData->Vertices;
		int uvOffset2 = GetVertexComponentOffset( meshIx, GrannyVertexTextureCoordinatesName "1" );
		int bytesPerVertex = GrannyGetTotalObjectSize( Mesh->PrimaryVertexData->VertexType );
		for( int i = 0; i < Mesh->PrimaryVertexData->VertexCount; ++i )
		{
			if( isHalfPrecision )
			{
				granny_real16* const uvPtr = (granny_real16*)((uint8_t*)originalVBufferContents +(i*bytesPerVertex) + uvOffset2);
				*uvPtr = GrannyReal32ToReal16( outputUVs[i].U );
				*(uvPtr+1) = GrannyReal32ToReal16( outputUVs[i].V );
			}
			else
			{
				granny_real32* const uvPtr = (granny_real32*)((uint8_t*)originalVBufferContents +(i*bytesPerVertex) + uvOffset2);
				*uvPtr = outputUVs[i].U;
				*(uvPtr+1) = outputUVs[i].V;
			}
		}
	}	

	GeoMemoryStream geoStream = GeoMemoryStream();
	geoStream.Write( &TriPackedGeometryData::s_versionNumber, sizeof( TriPackedGeometryData::s_versionNumber ), 1 );
	packedGeometry->Save( geoStream );

	TriPackedGeometryData PackedData;
	PackedData.m_buffer = geoStream.m_buffer;
	PackedData.m_bufferSize = geoStream.m_size;
	PackedData.m_pixelSize = enlightenPixelSize;
	SetGeometryHashesFromDatetimeAndGuid( PackedData.m_geometryBuildHash1, PackedData.m_geometryBuildHash2, guid );

	fi->ExtendedData.Type = TriPackedGeometryDataType;
	fi->ExtendedData.Object = &PackedData;

	granny_file_builder *Builder = GrannyBeginFile( 1, GrannyCurrentGRNStandardTag,
		GrannyGRNFileMV_32Bit_LittleEndian,
		GrannyGetTemporaryDirectory(),
		"GRNFileTemp");
	granny_file_data_tree_writer *Writer =
		GrannyBeginFileDataTreeWriting(GrannyFileInfoType, fi, 0, 0);
	GrannyWriteDataTreeToFileBuilder(Writer, Builder);
	GrannyEndFileDataTreeWriting(Writer);

	std::wstring filenameW = CA2W( filename.c_str() );
	std::wstring fullPath = BePaths->ResolvePathForWritingW( filenameW );
	bool result = GrannyEndFile( Builder, CW2A( fullPath.c_str() ) );

	return result;
}

// ---------------------------------------------------------------------------------------
// Description:
//   Creates Enlighten input mesh for a given mesh in granny file.
// Arguments:
//   meshIx - index of the mesh in the granny file.
//   fi - granny file information.
//   copyChartUVFromPackedGeometry - If true - copy input UVs from mesh 2nd UV set 
//                                   (used for taget mesh when projecting geometry),
//                                   if false - generate UVs using Enlighten atlas.
// Return Value:
//   Enlighten input mesh or NULL if the function failed to create it.
// ---------------------------------------------------------------------------------------
Enlighten::IPrecompInputMesh* TriGrannyRes::CreateEnlightenInputMesh( int meshIx, granny_file_info* fi, bool copyChartUVFromPackedGeometry )
{
		granny_mesh* mesh = fi->Meshes[meshIx];
		if( mesh == NULL )
		{
		return NULL;
		}
		CCP_LOG( "Processing mesh %i", meshIx );

		granny_int32x const MeshIndexCount  = GrannyGetMeshIndexCount(mesh);
		granny_int32x const MeshVertexCount = GrannyGetMeshVertexCount(mesh);
		granny_int32x const meshTriangleCount = GrannyGetMeshTriangleCount(mesh);
		granny_data_type_definition* grannyVertexDecl = mesh->PrimaryVertexData->VertexType;
		int bytesPerVertex = GrannyGetTotalObjectSize( grannyVertexDecl );

		// If the first the element is half precision then everything is half precision
	bool isHalfPrecision = ( grannyVertexDecl[0].Type == GrannyReal16Member );


		// Change the vertex format for the output
		// Only if the format has not been changed. If there is an extended object 
		// in the granny file, we can safely assume that it has been processed before.
		if ( !fi->ExtendedData.Object )
		{

			int endIx = 0;
			int texIx = -1;
			// Get the number of declarations and the index where we find the first uv set
			while( grannyVertexDecl[endIx].Type != GrannyEndMember )
			{
				if( texIx == -1 && ( strcmp( grannyVertexDecl[endIx].Name, GrannyVertexTextureCoordinatesName "0" ) == 0 ) )
				{
					texIx = endIx;
				}
				++endIx;
			}

		granny_data_type_definition* newGrannyVertexDecl = (granny_data_type_definition*)GrannyMemoryArenaPush( m_grannyArena, sizeof( granny_data_type_definition ) * (endIx + 2) );

			for( int i = 0; i <= texIx; ++i )
			{
				newGrannyVertexDecl[i] = grannyVertexDecl[i];
			}
			// Inject the new UV declaration
			granny_data_type_definition newTexCoord;
			if ( isHalfPrecision )
			{
				newTexCoord.Type = GrannyReal16Member;
			}
			else
			{
				newTexCoord.Type = GrannyReal32Member;
			}			
			newTexCoord.Name = GrannyVertexTextureCoordinatesName "1";
			newTexCoord.ReferenceType = NULL;
			newTexCoord.ArrayWidth = 2;
			newGrannyVertexDecl[texIx+1] = newTexCoord;
			// Append any remaining declarations
			for( int i = texIx+1; i <= endIx; ++i )
			{
				newGrannyVertexDecl[i+1] = grannyVertexDecl[i];
			}			
			bytesPerVertex = GrannyGetTotalObjectSize( newGrannyVertexDecl );

			// Convert the vertex buffer to match our new format
		granny_uint8* newVerts = (granny_uint8*)GrannyMemoryArenaPush( m_grannyArena, bytesPerVertex * MeshVertexCount );

			GrannyConvertVertexLayouts( 
				MeshVertexCount, 
				mesh->PrimaryVertexData->VertexType, 
				mesh->PrimaryVertexData->Vertices, 
				newGrannyVertexDecl, 
				newVerts 
				);

			// Make sure there are no name clashes
			// We give our new UV set the second place in the declaration .. so if
			// there was already a UV set with the same name, we need to increment its number
			char* texStr = GrannyVertexTextureCoordinatesName "%i";
			int texNameNr = 1;
			for( int i = texIx+2; i <= endIx; ++i )
			{
			char *buffA = (granny_int8*)GrannyMemoryArenaPush( m_grannyArena, strlen( texStr ) );
			sprintf_s( buffA, strlen( texStr ), texStr, texNameNr );
				if( strcmp( newGrannyVertexDecl[i].Name, buffA ) == 0 )
				{
					texNameNr++;
					sprintf_s( buffA, strlen(texStr), texStr, texNameNr );
					newGrannyVertexDecl[i].Name = buffA;
				}
			}	

			mesh->PrimaryVertexData->VertexType = newGrannyVertexDecl;
			mesh->PrimaryVertexData->Vertices = (granny_uint8*)newVerts;

			// The mesh->PrimaryVertexData->VertexComponentNames and newGrannyVertexDecl[i].Name don't match up exactly.
			// the vertex decl will look like
			// position, tangent, binormal, texturecoordinate0, texturecoordinate1
			// but the component names look like
		// position, tangent, binormal, map1, map2
			// Since I have just added a new texturecoordinat to the vertex decl, I should update the component names also
			// and keeping the naming schemes consistent. So adding a map%i to the list
			if ( mesh->PrimaryVertexData->VertexComponentNameCount )
			{
				mesh->PrimaryVertexData->VertexComponentNameCount += 1;

				// Update the names also
			const char** names = (const char**)GrannyMemoryArenaPush( m_grannyArena, sizeof(const char**)*mesh->PrimaryVertexData->VertexComponentNameCount );
				for ( int i = 0; i <= texIx; ++i )
				{
					names[i] = mesh->PrimaryVertexData->VertexComponentNames[i];
				}

			// And make sure that the maps are named correctly
				texStr = "map%i";
				texNameNr = 2;
			for( int i = texIx+1; i <= endIx; ++i )
				{
				char *buffB = (granny_int8*)GrannyMemoryArenaPush( m_grannyArena, strlen( texStr ) );
				sprintf_s( buffB, strlen( texStr ), texStr, texNameNr++ );
				names[i] = buffB;
				}	
				mesh->PrimaryVertexData->VertexComponentNames = names;
			}
		}
		//////////////////////////////////////////////////////////////////////////
		// This first section of code creates the Geo mesh and generates light-map coordinates
		// We don't need to use this if we have authored light-map UVs
		//////////////////////////////////////////////////////////////////////////
		// Create the space for the verts
		Geo::AtlasMeshVertices verts( MeshVertexCount );
		Geo::AtlasMeshTriangles triangles( meshTriangleCount );

		int positionOffset = GetVertexComponentOffset( meshIx, GrannyVertexPositionName );
		int normalOffset = GetVertexComponentOffset( meshIx, GrannyVertexNormalName );
		int uvOffset = GetVertexComponentOffset( meshIx, GrannyVertexTextureCoordinatesName "0" );


		granny_uint8* vertices = mesh->PrimaryVertexData->Vertices;

		for( int i = 0; i < MeshVertexCount; ++i)
		{	
			float p[3];
			float n[3];
		float uv[2] = { 0.0f, 0.0f };
			
			GetPosition( vertices + positionOffset, i, isHalfPrecision, bytesPerVertex, &p[0]);
			GetPosition( vertices + normalOffset, i, isHalfPrecision, bytesPerVertex, &n[0]);
		if( uvOffset != -1 )
		{
			GetTexCoord( vertices + uvOffset, i, isHalfPrecision, bytesPerVertex, &uv[0] );
		}

		Geo::AtlasMeshVertex vert = { {p[0], p[1], p[2]}, {n[0], n[1], n[2]}, {uv[0], uv[1]} };
			verts.Push( vert );
		}

		for( int i = 0; i < meshTriangleCount; ++i )
		{
			int v1,v2,v3;
			if( mesh->PrimaryTopology->Indices16 )
			{
				v1 = mesh->PrimaryTopology->Indices16[i*3];
				v2 = mesh->PrimaryTopology->Indices16[i*3 + 1];
				v3 = mesh->PrimaryTopology->Indices16[i*3 + 2];
			}
			else
			{
				v1 = mesh->PrimaryTopology->Indices[i*3];
				v2 = mesh->PrimaryTopology->Indices[i*3 + 1];
				v3 = mesh->PrimaryTopology->Indices[i*3 + 2];
			}
			Geo::AtlasMeshTriangle triangle = {{v1, v2, v3}, 0};
			triangles.Push( triangle );
		}

		// Now create an atlas map that we can use to automatically pack the geometry
		Geo::AtlasMesh* inputMesh = Geo::AtlasMesh::Create( &verts, &triangles );
		ON_BLOCK_EXIT( &Geo::AtlasMesh::Release, inputMesh );

		if( !inputMesh )
		{
			CCP_LOGERR("Failed to create input mesh");
		return NULL;
		}

		const Geo::AtlasMeshChartingFlags sharedEdgeBreak( 
			Geo::AtlasMeshChartingFlags::CC_SHARED_EDGE,
			Geo::AtlasMeshChartingFlags::CC_BREAK_ON_MATERIAL_BOUNDARY, 
			Geo::AtlasMeshChartingFlags::CC_DEGENERATES_IN_SEPARATE_CHART
			);

		// If you do not perform this step, stuff will explode when packing
	if( !inputMesh->DetectCharts(sharedEdgeBreak) )
	{
		CCP_LOGWARN( "Failed to DetectCharts on mesh #%i of %s", meshIx, GetPath() );
	}

		// ERRRR?
		Geo::AtlasMeshPackingFlags packingFlags;
		packingFlags.m_BlockSize = Geo::GeoFindBestBlockSize(256,256);
		packingFlags.m_TextureWidth = 256;	
		packingFlags.m_TextureHeight = 256;

		//Geo::GeoArray<Geo::AtlasMeshVertexSplitResult> refactorSpec;
		Geo::GeoFlattenAndPackResults* flattenAndPackResults = Geo::GeoFlattenAndPackResults::Create();
		Geo::GeoProgress progressObj = Geo::GeoProgress::CreateProgress();

		Geo::AtlasMesh* finalAtlasMesh = Geo::GeoFlattenAndPack( inputMesh, packingFlags, &progressObj, flattenAndPackResults );

		if( !finalAtlasMesh )
		{
			CCP_LOGERR( "Failed to build mesh.");
			return NULL;
		}
		ON_BLOCK_EXIT( &Geo::AtlasMesh::Release, finalAtlasMesh );

		const unsigned int refactorSize =  flattenAndPackResults->m_VertexSplittings.GetSize();
		CCP_LOG("Splicing UVs. Mesh Requires %u splits", refactorSize);
		//////////////////////////////////////////////////////////////////////////
		// We've now built the GeoMesh and generated UVs, put the UVs back into
		// the mesh so that I can see them
		//////////////////////////////////////////////////////////////////////////
		const unsigned finalVertexBufferSize = MeshVertexCount + refactorSize;

		if( refactorSize > 0 )
		{	
		granny_uint8* newVerts = (granny_uint8*)GrannyMemoryArenaPush( m_grannyArena, bytesPerVertex * finalVertexBufferSize );
			granny_vertex_data* oldVertexData = mesh->PrimaryVertexData;

			// Copy the original contents
			memcpy_s( newVerts, finalVertexBufferSize * bytesPerVertex, mesh->PrimaryVertexData->Vertices, MeshVertexCount * bytesPerVertex );

		mesh->PrimaryVertexData = (granny_vertex_data*)GrannyMemoryArenaPush( m_grannyArena, sizeof( granny_vertex_data ) );
			memcpy( mesh->PrimaryVertexData, oldVertexData, sizeof( granny_vertex_data ) );

			mesh->PrimaryVertexData->Vertices = newVerts;
			mesh->PrimaryVertexData->VertexCount = (granny_int32)finalVertexBufferSize;

			// Set index data pointer to new index data - note that index count is unchanged
			granny_tri_topology* oldTopology = mesh->PrimaryTopology;
		mesh->PrimaryTopology = (granny_tri_topology*)GrannyMemoryArenaPush( m_grannyArena, sizeof( granny_tri_topology ) );
			memcpy( mesh->PrimaryTopology, oldTopology, sizeof( granny_tri_topology ) );

			if( oldTopology->Indices )
			{
				mesh->PrimaryTopology->IndexCount = oldTopology->IndexCount;
			}
			else
			{
				mesh->PrimaryTopology->Index16Count = oldTopology->Index16Count;
			}

			for( Geo::AtlasMeshVertexSplitResult *i = flattenAndPackResults->m_VertexSplittings.Begin(); i != flattenAndPackResults->m_VertexSplittings.End(); ++i )
			{
				const Geo::AtlasMeshVertexSplitResult& duplicationVtx = *i;

				const unsigned oldVertexID = duplicationVtx.m_VertexToDuplicate;
				const unsigned newVertexID = duplicationVtx.m_IndexOfNewVertex;

				// Copy the vertex
				memcpy_s( (uint8_t*)newVerts +(newVertexID*bytesPerVertex), bytesPerVertex,  (uint8_t*)oldVertexData->Vertices + (oldVertexID*bytesPerVertex), bytesPerVertex );

				// Fix up the indices
				for( const Geo::AtlasMeshTriangleCorner* x = duplicationVtx.m_CornersToRelink.Begin(); x != duplicationVtx.m_CornersToRelink.End(); ++x)
				{
					const Geo::AtlasMeshTriangleCorner& corner = *x;
					if( oldTopology->Indices )
					{
						mesh->PrimaryTopology->Indices[(corner.m_TriangleIndex*3) + corner.m_TriangleCorner] = (granny_int32)newVertexID;
					}
					else
					{
						mesh->PrimaryTopology->Indices16[(corner.m_TriangleIndex*3) + corner.m_TriangleCorner] = (granny_uint16)newVertexID;
					}
				}
			}
		}

		flattenAndPackResults->Release();
		flattenAndPackResults = NULL;

		//////////////////////////////////////////////////////////////////////////
		// Start compiling the fixed mesh into an enlighten scene
		//////////////////////////////////////////////////////////////////////////
		Enlighten::IPrecompInputMesh* precomputeMesh = Enlighten::IPrecompInputMesh::Create();
		//ON_BLOCK_EXIT( &Enlighten::IPrecompInputMesh::Release, precomputeMesh );// Can't use CComPtr

		//////////////////////////////////////////////////////////////////////////
		// Create the precompute geometry from the GeoAtlasMesh
		//////////////////////////////////////////////////////////////////////////
		Geo::GeoArray< Enlighten::PrecompInputVertex > precompVerts;
		Geo::GeoArray< Enlighten::PrecompInputFace > precompFaces;

		const int grannyVertexSize = GrannyGetTotalObjectSize(mesh->PrimaryVertexData->VertexType);
		const int uvOffset0 = GetVertexComponentOffset( meshIx, GrannyVertexTextureCoordinatesName "0" );
	int uvOffset1 = 0;
	if( copyChartUVFromPackedGeometry )
	{
		uvOffset1 = GetVertexComponentOffset( meshIx, GrannyVertexTextureCoordinatesName "1" );
	}
		
		for( int i = 0; i < finalAtlasMesh->GetVertexCount(); ++i )
		{
			Geo::AtlasMeshVertex const * const vert = finalAtlasMesh->GetVertex(i);			
		float uvs0[2];
			if( isHalfPrecision	)
			{
			GrannyReal16ToReal32(*(granny_real16*)&mesh->PrimaryVertexData->Vertices[i*grannyVertexSize + uvOffset0], &uvs0[0] );
			GrannyReal16ToReal32(*(granny_real16*)&mesh->PrimaryVertexData->Vertices[i*grannyVertexSize + uvOffset0 + 2], &uvs0[1] );
			}
			else
			{
				granny_real32 *temp = (granny_real32*)&mesh->PrimaryVertexData->Vertices[i*grannyVertexSize + uvOffset0];
			uvs0[0] = temp[0];
			uvs0[1] = temp[1];
		}

		float uvs1[2];
		if( copyChartUVFromPackedGeometry )
		{
			if( isHalfPrecision	)
			{
				GrannyReal16ToReal32(*(granny_real16*)&mesh->PrimaryVertexData->Vertices[i*grannyVertexSize + uvOffset1], &uvs1[0] );
				GrannyReal16ToReal32(*(granny_real16*)&mesh->PrimaryVertexData->Vertices[i*grannyVertexSize + uvOffset1 + 2], &uvs1[1] );
			}
			else
			{
				granny_real32 *temp = (granny_real32*)&mesh->PrimaryVertexData->Vertices[i*grannyVertexSize + uvOffset1];
				uvs1[0] = temp[0];
				uvs1[1] = temp[1];
			}
		}
		else
		{
			uvs1[0] = vert->m_U;
			uvs1[1] = vert->m_V;
			}

			const Enlighten::PrecompInputVertex newVert = { 
				Geo::GeoPoint3( vert->m_Position[0], vert->m_Position[1], vert->m_Position[2] ),
				Geo::GeoVector3( vert->m_Normal[0], vert->m_Normal[1], vert->m_Normal[2]),
				// ChartUV = unique light-map coordinates
			Geo::GeoPoint2( uvs1[0], uvs1[1] ),
				// AlbedoUV = diffuse texture coordinates
			Geo::GeoPoint2( uvs0[0], uvs0[1] )
			};

			precompVerts.Push( newVert );
		}

		precomputeMesh->AddVertices( precompVerts.Begin(), precompVerts.End() );

		for( int i = 0; i < finalAtlasMesh->GetTriangleCount(); ++i )
		{
			Geo::AtlasMeshTriangle const * const face = finalAtlasMesh->GetTriangle(i);

			Enlighten::PrecompInputFace newFace;
			newFace.m_Indices[0] =  face->m_A;
			newFace.m_Indices[1] =  face->m_B;
			newFace.m_Indices[2] =  face->m_C;

			// TODO: Test that this works
			int materialID = 0;
			for( int groupID = 0; groupID < mesh->PrimaryTopology->GroupCount; ++groupID )
			{
				granny_tri_material_group& grp = mesh->PrimaryTopology->Groups[groupID];
				if( grp.TriFirst <= i && grp.TriFirst+grp.TriCount > i )
				{
					materialID = grp.MaterialIndex;
					break;
				}
			}
			
			newFace.m_AlbedoId = materialID;
			precompFaces.Push( newFace );
		}

		precomputeMesh->AddFaces( precompFaces.Begin(), precompFaces.End() );

	return precomputeMesh;
}
#endif

bool TriGrannyRes::HasExtendedData() const
{
	if( !m_grannyFile )
	{
		CCP_LOGERR( "TriGrannyRes::HasExtendedData: Object has no Granny file" );
		return false;
	}

	granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );
	if( fi == NULL )
	{
		CCP_LOGERR( "TriGrannyRes::HasExtendedData: Granny file has no file info" );
		return false;
	}

	if( !fi->ExtendedData.Object  )
	{
		return false;
	}

	return true;
}

float TriGrannyRes::GetEnlightenPixelSize( void ) const
{
#if INTERIORS_ENABLED
	if( !m_grannyFile )
	{
		CCP_LOGERR( "TriGrannyRes::GetEnlightenPixelSize: Object has no Granny file" );
		return 0.0f;
	}

	granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );
	if( fi == NULL )
	{
		CCP_LOGERR( "TriGrannyRes::GetEnlightenPixelSize: Granny file has no file info" );
		return 0.0f;
	}

	if( fi->ExtendedData.Object  )
	{
		TriPackedGeometryData* p = (TriPackedGeometryData*)fi->ExtendedData.Object;
		return p->m_pixelSize;
	}
#endif
	return 0.0f;
}

bool TriGrannyRes::HasValidEnlightenData() const
{
#if INTERIORS_ENABLED
	if( !m_grannyFile )
	{
		return false;
	}

	granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );
	if( fi == NULL )
	{
		return false;
	}

	if( fi->ExtendedData.Object  )
	{
		TriPackedGeometryData* p = (TriPackedGeometryData*)fi->ExtendedData.Object;
		if( p->GetVersion() == TriPackedGeometryData::s_versionNumber )
		{
			return true;
		}
	}
#endif
	return false;
}

float TriGrannyRes::GetMeshSurfaceArea( int meshIx ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_grannyFile )
	{
		CCP_LOGERR( "TriGrannyRes::GetMeshSurfaceArea: Object has no Granny file" );
		return 0.0f;
	}

	granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );
	if( fi == NULL )
	{
		CCP_LOGERR( "TriGrannyRes::GetMeshSurfaceArea: Granny file has no file info" );
		return 0.0f;
	}
	granny_mesh* mesh = fi->Meshes[meshIx];
	if( mesh == NULL )
	{
		CCP_LOGERR( "TriGrannyRes::GetMeshSurfaceArea: Failed to get mesh" );
		return 0.0f;
	}

	granny_int32x const meshTriangleCount = GrannyGetMeshTriangleCount( mesh );
	granny_uint8* vertices = mesh->PrimaryVertexData->Vertices;
	granny_tri_topology* topology = mesh->PrimaryTopology;
	int bytesPerVertex = GrannyGetTotalObjectSize( mesh->PrimaryVertexData->VertexType );
	float result = 0.0f;

	for( int i = 0; i < meshTriangleCount; ++i)
	{	
		Vector3 p1;
		Vector3 p2;
		Vector3 p3;
		int idx1, idx2, idx3;
		bool halfPrecision = false;
		if ( topology->Indices )
		{
			idx1 = topology->Indices[3*i+0];
			idx2 = topology->Indices[3*i+1];
			idx3 = topology->Indices[3*i+2];
		}
		else
		{
			halfPrecision = true;
			idx1 = topology->Indices16[3*i+0];
			idx2 = topology->Indices16[3*i+1];
			idx3 = topology->Indices16[3*i+2];
		}
		GetPosition( vertices, idx1, halfPrecision, bytesPerVertex, p1 );
		GetPosition( vertices, idx2, halfPrecision, bytesPerVertex, p2 );
		GetPosition( vertices, idx3, halfPrecision, bytesPerVertex, p3 );

		Vector3 v1 = p2 - p1;
		Vector3 v2 = p3 - p1;

		Vector3 t;
		D3DXVec3Cross( &t, &v1, &v2 );

		result += 0.5f * D3DXVec3Length( &t );
	}
	
	return result;
}

int TriGrannyRes::GetEnlightenGuid( ) const
{
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	// If we have any extended data... it must be the enlighten
	// returns -1 on failure
	if ( HasExtendedData() )
	{
		granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );

		TriPackedGeometryData* p = (TriPackedGeometryData*)fi->ExtendedData.Object;
		if( p->GetVersion() != TriPackedGeometryData::s_versionNumber )
		{
			CCP_LOGERR( "TriGrannyRes::GetEnlightenPixelSize: Granny file has outdated Enlighten geometry data (version %u; current version is %u)", 
						p->GetVersion(), 
						TriPackedGeometryData::s_versionNumber );
			return -1;
		}
		Enlighten::IPrecompPackedGeometry* packedGeometry = Enlighten::IPrecompPackedGeometry::Create();

		GeoMemoryStream geoStream( p->m_bufferSize, p->m_buffer );
		unsigned int version;
		geoStream.Read( &version, sizeof( version ), 1 );
		if ( packedGeometry->Load( geoStream ))
		{
			return packedGeometry->GetId().Low();
		}
		return -1;
	}
#endif
	return -1;
}

bool TriGrannyRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t TriGrannyRes::GetMemoryUsage()
{
	return m_memoryUsage;
}

void TriGrannyRes::SetGeometryHashesFromDatetimeAndGuid( unsigned int& hash1, unsigned int& hash2, unsigned guid )
{
	time_t localTime;
	time( &localTime );
	struct tm gmtTime;
	gmtime_s( &gmtTime, &localTime );

	const unsigned int yearMask = (1 << 20) - 1;
	const unsigned int guidMask = (1 << 21) - 1;

	hash1 = (gmtTime.tm_min) 			// 0-60 ~= 64 = 6 bits
			| (gmtTime.tm_hour << 6) 	// 0-24 ~= 32 = 5 bits
			| (((unsigned)gmtTime.tm_year & yearMask)  << 11);
	
	hash2 = gmtTime.tm_yday				// 0-365 ~= 512 = 9 bits
			| ((guid & guidMask) << 9);
}

static bool TestStringArray(std::vector<char*> & searchStrings, const char * testString)
{
	std::vector<char*>::const_iterator walker(searchStrings.begin()), endOfList(searchStrings.end());

	while(walker != endOfList)
	{
		const char * w = *walker;

		if (strcmp( w, testString) == 0)
			return(true);

		++walker;
	}

	return(false);

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
					
					sprintf_s(tmpbuf,"%d_%d_%s_%s", meshIndex, areaIndex, materialMap->Usage, defs->Name);
					PyDict_SetItemString(flatStringDictionary, tmpbuf, matString);
					// Adding all strings (used to be a break here) 
				}

				dataPtr += GrannyGetMemberTypeSize(defs);
				defs++;
			}

		}
	}

	// look at top level material extended data too, but only read floats, this is useful for incandesence and ambient colors, namely for enlighten.


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

#if BLUE_WITH_PYTHON
// Description:
//	Helper function for Python thunkers that work on a mesh identified
//	either by its name or index.
// Arguments:
//	meshId - Should be either a PyString or a PyInt
// Return value:
//	The granny mesh associated with meshId. If meshId was a string, this will
//	the first mesh with a matching name, or NULL of no matching names was found.
//	If meshId was an integer, this will be the mesh with that index, or NULL
//	if the index is out of range.
granny_mesh* TriGrannyRes::GetMeshFromNameOrIndex( PyObject* meshId )
{
	granny_file_info* fi = GrannyGetFileInfo( m_grannyFile );

	int ix = int( PyInt_AsLong( meshId ) );
	if( ix >= fi->MeshCount )
	{
		return NULL;
	}
	if( ix < 0 )
	{
		const char* name = PyString_AsString( meshId );
		for( int meshIx = 0; meshIx < fi->MeshCount; ++meshIx )
		{
			if( strcmp( name, fi->Meshes[meshIx]->Name ) == 0 )
			{
				ix = meshIx;
				break;
			}
		}
	}

	if( ix < 0 )
	{
		return NULL;
	}

	return fi->Meshes[ix];
}
#endif

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
	HRESULT hr = meshData->m_vertexBuffer.Lock( 0,0, &pVertexData, Tr2RenderContextEnum::LOCK_WRITEONLY, renderContext );
	if( FAILED( hr ) )
	{
		return Be::Result<std::string>( "Failed to lock vertex buffer" );
	}

	BakeBlendshape( meshIx, weights, pVertexData, meshData->m_vertexCount * meshData->m_bytesPerVertex );
	meshData->m_vertexBuffer.Unlock( renderContext );

	return Be::Result<std::string>();
}
