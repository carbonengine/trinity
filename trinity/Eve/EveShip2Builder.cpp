#include "StdAfx.h"

#include "EveShip2Builder.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"


CcpMutex s_writeGrannyFileMutex( "EveShip2Builder", "s_writeGrannyFileMutex" );

namespace
{

bool StartsWith( const std::string& string, const char* prefix )
{
	return strncmp( string.c_str(), prefix, strlen( prefix ) ) == 0;
}

}

EveShip2Builder::EveShip2Builder( IRoot* lockobj ) :
	PARENTLOCK( m_grannyResources ),
	PARENTLOCK( m_hulls ),
	m_areaOffset( 0 ),
	m_vertexSize( 0 ),
	m_weldThreshold( 0.03f )
{
}

EveShip2Builder::~EveShip2Builder()
{
}

void EveShip2Builder::Weld( granny_uint8* referenceVB, int referenceCount, granny_uint8* vb, int count ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Vector3* referencePositions = static_cast<Vector3*>( CCP_MALLOC( "Weld/referencePositions", referenceCount * sizeof( Vector3 ) ) );
	Vector3* positions = static_cast<Vector3*>( CCP_MALLOC( "Weld/referencePositions", count * sizeof( Vector3 ) ) );

	{
		CCP_STATS_ZONE( "GrannyConvertVertexLayouts" );

		GrannyConvertVertexLayouts( referenceCount, m_grannyVertexData.VertexType, referenceVB, GrannyP3VertexType, referencePositions );
		GrannyConvertVertexLayouts( count, m_grannyVertexData.VertexType, vb, GrannyP3VertexType, positions );
	}

	{
		CCP_STATS_ZONE( "Weld" );

		XMVECTOR epsilon = XMVectorSet( m_weldThreshold, m_weldThreshold, m_weldThreshold, m_weldThreshold );

		for( int vertexIx = 0; vertexIx < count; ++vertexIx )
		{
			Vector3& vertexPos = positions[vertexIx];
			for( int referenceIx = 0; referenceIx < referenceCount; ++referenceIx )
			{
				Vector3& referencePos = referencePositions[referenceIx];
				if( XMVector3NearEqual( vertexPos, referencePos, epsilon ) )
				{
					vertexPos = referencePos;

					// Break out of the inner loop - we've matched the vertex to one of the 
					// reference positions and moved it to its location - no need to look
					// at the rest, it just has to match one of them.
					break;
				}
			}
		}
	}

	granny_uint8* dst = vb;
	granny_uint8* src;
	int sz;

	// Assume position is first element in vertex structure
	if( m_grannyVertexData.VertexType->Type == GrannyReal16Member )
	{
		CCP_STATS_ZONE( "FloatToHalf" );

		Float_16* halfPositions = static_cast<Float_16*>( CCP_MALLOC( "Weld/halfPositions", count * sizeof( Float_16 ) ) );
		std::transform( 
			reinterpret_cast<float*>( positions ), 
			reinterpret_cast<float*>( positions ) + count, 
			halfPositions, 
			[]( float x ) { return Float_16( x ); } );

		src = (granny_uint8*)halfPositions;
		sz = 6;
	}
	else
	{
		src = (granny_uint8*)positions;
		sz = 12;
	}

	{
		CCP_STATS_ZONE( "Copy" );

		for( int i = 0; i < count; ++i )
		{
			memcpy( dst, src, sz );
			dst += m_vertexSize;
			src += sz;
		}
	}

	CCP_FREE( positions );
	CCP_FREE( referencePositions );
}


void EveShip2Builder::InitializeGrannyFile()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	memset( &m_grannyFileInfo, 0, sizeof( m_grannyFileInfo ) );
	memset( &m_finalGrannyMesh, 0, sizeof( m_finalGrannyMesh ) );
	memset( &m_grannyVertexData, 0, sizeof( m_grannyVertexData ) );
	memset( &m_grannyTopology, 0, sizeof( m_grannyTopology ) );
	m_vertexSize = 0;
}

bool EveShip2Builder::CombineGrannyGeometry(int grnResIdx, const Matrix& offsetTransform)
{
	// compute offset
	Vector3 translation = offsetTransform.GetTranslation();
	Matrix inverseTransform = Inverse(offsetTransform);

	granny_real32 affine[3] = { translation.x, translation.y, translation.z };

	granny_real32 matrix[3][3] = { { offsetTransform._11, offsetTransform._21, offsetTransform._31 },
	{ offsetTransform._12, offsetTransform._22, offsetTransform._32 },
	{ offsetTransform._13, offsetTransform._23, offsetTransform._33 } };

	granny_real32 invMatrix[3][3] = { { inverseTransform._11, inverseTransform._21, inverseTransform._31 },
	{ inverseTransform._12, inverseTransform._22, inverseTransform._32 },
	{ inverseTransform._13, inverseTransform._23, inverseTransform._33 } };

	return AddGeometry(m_grannyResources[grnResIdx], affine, (granny_real32*)matrix, (granny_real32*)invMatrix);
}


bool EveShip2Builder::CombineHullGeometry()
{
	CCP_LOGERR("Start building...");

	if (m_hulls.size() != m_grannyResources.size())
	{
		CCP_LOGERR("EveShip2Builder: Hull vector length must match gr2 resource vector length!");
		return false;
	}

	InitializeGrannyFile();

	Vector3 offset(0.f, 0.f, 0.f);
	for (size_t grnResIdx = 0; grnResIdx != m_grannyResources.size(); ++grnResIdx)
	{
		EveSOFDataHullPtr sofHull = m_hulls[grnResIdx];

		// compute offset
		granny_real32* affine = (granny_real32*)&offset;
		granny_real32 matrix[3][3] = { { 1.f, 0.f, 0.f },{ 0.f, 1.f, 0.f },{ 0.f, 0.f, 1.f } };
		granny_real32 invMatrix[3][3] = { { 1.f, 0.f, 0.f },{ 0.f, 1.f, 0.f },{ 0.f, 0.f, 1.f } };

		if ( !AddGeometry(m_grannyResources[grnResIdx], affine, (granny_real32*)matrix, (granny_real32*)invMatrix) )
		{
			CCP_LOGERR("EveShip2Builder: Failed to add geometry!");
			return false;
		}

		// offset is in a locator set on the hull
		for (auto it = sofHull->m_locatorSets.begin(); it != sofHull->m_locatorSets.end(); ++it)
		{
			if( EveSOFDataHullLocatorSetPtr locatorSet = BlueCastPtr( ( *it ) ) )
			{
				if( locatorSet->m_name == BlueSharedString( "next_subsystem" ) )
				{
					if( !locatorSet->m_locators.empty() )
					{
						offset += locatorSet->m_locators[0]->m_position;
					}
				}
			}
		}
	}

	// save it
	FinalizeGrannyFile(m_outputFilename);

	return true;
}

bool EveShip2Builder::AddGeometry(const TriGrannyResPtr grnRes, const granny_real32* affine, const granny_real32* matrix, const granny_real32* invMatrix)
{
	// grannies must be already loaded
	if (!grnRes->IsGood())
	{
		CCP_LOGERR("EveShip2Builder: GrannyRes failed to load!");
		return false;
	}

	granny_file* grannyFile = grnRes->GetGrannyFile();
	granny_file_info* fileInfo = GrannyGetFileInfo(grannyFile);
	granny_mesh* grannyMesh = fileInfo->Meshes[0];

	if (!m_grannyVertexData.VertexType)
	{
		m_grannyVertexData.VertexType = grannyMesh->PrimaryVertexData->VertexType;
		m_vertexSize = GrannyGetTotalObjectSize(m_grannyVertexData.VertexType);
	}
	else
	{
		int vertexSize = GrannyGetTotalObjectSize(grannyMesh->PrimaryVertexData->VertexType);
		if (vertexSize != m_vertexSize)
		{
			CCP_LOGERR("EveShip2Builder: Meshes being merged have differing vertex formats");
			return false;
		}
	}

	// add each geom at a time
	int prevCount = m_grannyVertexData.VertexCount;
	int currentCount = grannyMesh->PrimaryVertexData->VertexCount;
	int newCount = prevCount + currentCount;
	int vbSize = m_vertexSize * newCount;

	granny_uint8* newVertices = (granny_uint8*)CCP_MALLOC("EveShip2Builder/vertices", vbSize);
	granny_uint8* dstVb = newVertices;

	if (m_grannyVertexData.Vertices)
	{
		memcpy(newVertices, m_grannyVertexData.Vertices, prevCount * m_vertexSize);
		dstVb += prevCount * m_vertexSize;
	}

	memcpy(dstVb, grannyMesh->PrimaryVertexData->Vertices, currentCount * m_vertexSize);

	// offset the geom
	GrannyTransformVertices(currentCount, m_grannyVertexData.VertexType, dstVb, affine, matrix, invMatrix, false, false);

	if (m_grannyVertexData.Vertices)
	{
		CCP_FREE(m_grannyVertexData.Vertices);
	}

	if (prevCount)
	{
		Weld(newVertices, prevCount, dstVb, currentCount);
	}
	else
	{
		Weld(dstVb, currentCount, dstVb, currentCount);
	}

	m_grannyVertexData.Vertices = newVertices;
	m_grannyVertexData.VertexCount = newCount;

	int prevIxCount = m_grannyTopology.IndexCount;
	bool is16Bit = false;
	int currentIxCount = grannyMesh->PrimaryTopology->IndexCount;
	if (!currentIxCount)
	{
		currentIxCount = grannyMesh->PrimaryTopology->Index16Count;
		is16Bit = true;
	}
	int newIxCount = prevIxCount + currentIxCount;

	int ibSize = newIxCount * sizeof(granny_int32);
	granny_int32* newIndices = (granny_int32*)CCP_MALLOC("EveShip2Builder/indices", ibSize);
	granny_int32* dstIb = newIndices;
	if (m_grannyTopology.Indices)
	{
		memcpy(newIndices, m_grannyTopology.Indices, prevIxCount * sizeof(granny_int32));
		dstIb += prevIxCount;
	}

	for (int ixIx = 0; ixIx < currentIxCount; ++ixIx)
	{
		int currentIx;
		if (is16Bit)
		{
			currentIx = grannyMesh->PrimaryTopology->Indices16[ixIx];
		}
		else
		{
			currentIx = grannyMesh->PrimaryTopology->Indices[ixIx];
		}
		currentIx += prevCount;
		dstIb[ixIx] = currentIx;
	}

	if (m_grannyTopology.Indices)
	{
		CCP_FREE(m_grannyTopology.Indices);
	}

	m_grannyTopology.Indices = newIndices;
	m_grannyTopology.IndexCount = newIxCount;

	for (int groupIx = 0; groupIx < grannyMesh->PrimaryTopology->GroupCount; ++groupIx)
	{
		granny_tri_material_group grp = grannyMesh->PrimaryTopology->Groups[groupIx];
		grp.TriFirst += prevIxCount / 3;
		m_grannyGroups.push_back(grp);
	}

	m_areaOffset += grannyMesh->MaterialBindingCount;

	return true;
}

void EveShip2Builder::FinalizeGrannyFile( const std::string& outputName, const bool& combineGroups /*= false*/ )
{
	CCP_STATS_ZONE(__FUNCTION__);

	if ( m_grannyGroups.size() == 0 )
	{
		CCP_LOGERR("%s: No granny groups found", __FUNCTION__);
		return;
	}

	CcpAutoMutex guardGrannyWriteAccess(s_writeGrannyFileMutex);

	std::wstring outputNameW = (const wchar_t*)CA2W(outputName.c_str());
	std::wstring fullName = BePaths->ResolvePathForWritingW(outputNameW);
	if (fullName.empty())
	{
		CCP_LOGERR("%s: '%s' is not a valid filename", __FUNCTION__, outputName.c_str());
		return;
	}

	if ( combineGroups )
	{
		// Combine all groups into one
		granny_tri_material_group grannyGroup = granny_tri_material_group(m_grannyGroups[0]);
		for (size_t grpIdx = 1; grpIdx < m_grannyGroups.size(); grpIdx++)
		{
			grannyGroup.TriCount += m_grannyGroups[grpIdx].TriCount;
		}

		m_grannyTopology.GroupCount = 1;
		m_grannyTopology.Groups = &grannyGroup;
	}
	else
	{
		m_grannyTopology.GroupCount = (unsigned int)m_grannyGroups.size();
		m_grannyTopology.Groups = &m_grannyGroups[0];
	}

	m_finalGrannyMesh.PrimaryTopology = &m_grannyTopology;
	m_finalGrannyMesh.PrimaryVertexData = &m_grannyVertexData;

	granny_mesh* meshes[] = { &m_finalGrannyMesh };
	granny_tri_topology* topologies[] = { m_finalGrannyMesh.PrimaryTopology };
	granny_vertex_data* vertexDatas[] = { m_finalGrannyMesh.PrimaryVertexData };

	m_grannyFileInfo.ModelCount = 0;
	m_grannyFileInfo.Models = 0;
	m_grannyFileInfo.MeshCount = 1;
	m_grannyFileInfo.Meshes = meshes;
	m_grannyFileInfo.VertexDataCount = 1;
	m_grannyFileInfo.VertexDatas = vertexDatas;
	m_grannyFileInfo.TriTopologyCount = 1;
	m_grannyFileInfo.TriTopologies = topologies;
	m_grannyFileInfo.MaterialCount = (unsigned int)m_grannyMaterials.size();
	m_grannyFileInfo.Materials = &m_grannyMaterials[0];

	m_finalGrannyMesh.MaterialBindingCount = m_grannyFileInfo.MaterialCount;
	m_finalGrannyMesh.MaterialBindings = (granny_material_binding*)CCP_MALLOC("myMesh.MaterialBindings", sizeof( granny_material_binding ) * m_grannyFileInfo.MaterialCount );

	for( int i = 0; i < m_grannyFileInfo.MaterialCount; ++i )
	{
		m_finalGrannyMesh.MaterialBindings[i].Material = m_grannyFileInfo.Materials[i];
	}

	CCP_LOG( "%s: GrannyBeginFileInMemory", __FUNCTION__ );

	granny_file_builder *Builder = GrannyBeginFileInMemory(1, GrannyCurrentGRNStandardTag, GrannyGRNFileMV_32Bit_LittleEndian, 8192);
	if( !Builder )
	{
		CCP_LOGERR( "%s: Could not create granny_file_builder", __FUNCTION__ );
		return;
	}

	CCP_LOG( "%s: GrannyBeginFileDataTreeWriting", __FUNCTION__ );

	granny_file_data_tree_writer *Writer = GrannyBeginFileDataTreeWriting(GrannyFileInfoType, &m_grannyFileInfo, 0, 0);
	if( !Writer )
	{
		CCP_LOGERR( "%s: Could not get granny_file_data_tree_writer", __FUNCTION__ );
		return;
	}

	CCP_LOG( "%s: GrannyWriteDataTreeToFileBuilder", __FUNCTION__ );

	if( !GrannyWriteDataTreeToFileBuilder(Writer, Builder) )
	{
		CCP_LOGERR( "%s: GrannyWriteDataTreeToFileBuilder failed", __FUNCTION__ );
		return;
	}

	CCP_LOG( "%s: GrannyEndFileDataTreeWriting", __FUNCTION__ );

	GrannyEndFileDataTreeWriting(Writer);

	CCP_LOG( "%s: GrannyEndFileToWriter", __FUNCTION__ );

	granny_file_writer* memWriter = GrannyCreateMemoryFileWriter( 8192 );

	GrannyEndFileToWriter( Builder, memWriter );

	CCP_FREE( m_finalGrannyMesh.MaterialBindings );
	CCP_FREE( m_grannyTopology.Indices );
	CCP_FREE( m_grannyVertexData.Vertices );

	IResFilePtr outputStream;

	BeClasses->CreateInstanceFromName( "ResFile", BlueInterfaceIID<IResFile>(), (void**)&outputStream );

	if( !outputStream->CreateW( fullName.c_str() ) )
	{
		CCP_LOGERR( "%s: Couldn't create file %S", __FUNCTION__, fullName.c_str() );
		return;
	}

	granny_uint8* buffer;
	granny_int32x bufferSize;
	GrannyStealMemoryWriterBuffer( memWriter, &buffer, &bufferSize );
	outputStream->Write( (void*)buffer, (size_t)bufferSize );
	outputStream->Close();

	GrannyFreeMemoryWriterBuffer( buffer );	

	CCP_LOG( "%s: Done", __FUNCTION__ );
}

