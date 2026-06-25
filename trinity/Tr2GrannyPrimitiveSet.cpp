// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2GrannyPrimitiveSet.h"
#include "Resources/TriGrannyRes.h"
#include "TriDevice.h"
#include "TriRenderBatch.h"

using namespace Tr2RenderContextEnum;

const char* g_pickingMeshName = "picking";

Tr2GrannyPrimitiveSet::Tr2GrannyPrimitiveSet( IRoot* lockobj /*= NULL*/ ) :
	Tr2PrimitiveSet( lockobj ),
	m_renderSolid( FALSE ),
	m_primitiveCount( 0 ),
	m_pickingPrimitiveCount( 0 ),
	m_pickingIndexOffset( 0 )
{
	TriDevice::RegisterResource( this );
	m_localTransform = IdentityMatrix();
}

Tr2GrannyPrimitiveSet::~Tr2GrannyPrimitiveSet()
{
	ReleaseResources( TRISTORAGE_ALL );
	TriDevice::UnregisterResource( this );
	if( m_grannyRes )
	{
		m_grannyRes->RemoveNotifyTarget( this );
		m_grannyRes.Unlock();
	}
}

//////////////////////////////////////////////////////////////////////////////////////
// IInitialize
bool Tr2GrannyPrimitiveSet::Initialize()
{
	SetGrannyResource();
	PrepareResources();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
// ITriDeviceResource
void Tr2GrannyPrimitiveSet::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer = Tr2BufferAL();
	m_lineIndexBuffer = Tr2BufferAL();
	m_triangleIndexBuffer = Tr2BufferAL();
}

bool Tr2GrannyPrimitiveSet::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		Tr2VertexDefinition vd;
		vd.Add( vd.FLOAT32_3, vd.POSITION );
		vd.Add( vd.FLOAT32_3, vd.NORMAL );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD );

		m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );
		if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return false;
		}
	}

	if( m_points.size() )
	{
		if( !m_vertexBuffer.IsValid() )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			CR_RETURN_VAL(
				m_vertexBuffer.Create(
					uint32_t( sizeof( TriangleVertex ) ),
					uint32_t( m_points.size() ),
					Tr2GpuUsage::VERTEX_BUFFER,
					Tr2CpuUsage::WRITE_OFTEN,
					nullptr,
					renderContext ),
				false );
		}

		TriangleVertex* vertexBuffer;
		CR_RETURN_VAL( m_vertexBuffer.MapForWriting( vertexBuffer, renderContext ), false );

		memcpy( vertexBuffer, &m_points[0], sizeof( TriangleVertex ) * m_points.size() );

		Vector3 center( 0.0f, 0.0f, 0.0f );
		float radius = 0.0f;
		ComputeBoundingSphere( &vertexBuffer[0].m_position, (unsigned int)m_points.size(), sizeof( TriangleVertex ), center, radius );

		m_boundingSphere.x = center.x;
		m_boundingSphere.y = center.y;
		m_boundingSphere.z = center.z;
		m_boundingSphere.w = radius;

		m_vertexBuffer.UnmapForWriting( renderContext );
	}

	if( m_triangleIndices.size() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN_VAL(
			m_triangleIndexBuffer.Create(
				4,
				(unsigned int)m_triangleIndices.size(),
				Tr2GpuUsage::INDEX_BUFFER,
				Tr2CpuUsage::NONE,
				&m_triangleIndices[0],
				renderContext ),
			false );
	}

	if( m_lineIndices.size() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN_VAL(
			m_lineIndexBuffer.Create(
				4,
				(unsigned int)m_lineIndices.size(),
				Tr2GpuUsage::INDEX_BUFFER,
				Tr2CpuUsage::NONE,
				&m_lineIndices[0],
				renderContext ),
			false );
	}

	return true;
}

void Tr2GrannyPrimitiveSet::GetBatchesImpl( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData, Tr2Material* effect, GetBatchesReason reason )
{
	if( !m_vertexBuffer.IsValid() )
	{
		return;
	}

	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}

	Tr2RenderBatch batch;
	batch.SetMaterial( effect );
	batch.SetPerObjectData( perObjectData );
	batch.SetVertexDeclaration( m_vertexDeclHandle );
	batch.SetStreamSource( 0, m_vertexBuffer, sizeof( TriangleVertex ) );
	if( m_renderSolid || reason == GetBatchesReason::Picking )
	{
		batch.SetInidices( m_triangleIndexBuffer, m_triangleIndexBuffer.GetDesc().stride );
		if( reason == GetBatchesReason::Picking && ( m_pickingPrimitiveCount > 0 ) )
		{
			batch.SetDrawIndexedInstanced( m_pickingPrimitiveCount * 3, 1, m_pickingIndexOffset, 0, 0 );
		}
		else
		{
			batch.SetDrawIndexedInstanced( ( m_primitiveCount - m_pickingPrimitiveCount ) * 3, 1, 0, 0, 0 );
		}
	}
	else
	{
		batch.SetInidices( m_lineIndexBuffer, m_lineIndexBuffer.GetDesc().stride );
		batch.SetTopology( TOP_LINES );
		batch.SetDrawIndexedInstanced( ( m_primitiveCount - m_pickingPrimitiveCount ) * 6, 1, 0, 0, 0 );
	}
	accumulator->Commit( batch );
}


bool Tr2GrannyPrimitiveSet::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_grannyResPath ) )
	{
		SetGrannyResource();
	}

	return Tr2PrimitiveSet::OnModified( value );
}

void Tr2GrannyPrimitiveSet::ReleaseCachedData( BlueAsyncRes* p )
{
	if( p == m_grannyRes )
	{
		CleanUp();
	}
}


void Tr2GrannyPrimitiveSet::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_grannyRes && p->IsGood() )
	{
		CleanUp();
		CreatePrimitive();
		PrepareResources();
	}
}

void Tr2GrannyPrimitiveSet::CleanUp( void )
{
	m_points.clear();
	m_triangleIndices.clear();
	m_lineIndices.clear();
	m_primitiveCount = 0;
	m_pickingPrimitiveCount = 0;
	m_pickingIndexOffset = 0;
	ReleaseResources( TRISTORAGE_ALL );
}

void Tr2GrannyPrimitiveSet::SetGrannyResource()
{
	if( m_grannyRes )
	{
		m_grannyRes->RemoveNotifyTarget( this );
		m_grannyRes.Unlock();
	}

	if( !m_grannyResPath.empty() )
	{
		BeResMan->GetResource( m_grannyResPath.c_str(), "raw", m_grannyRes );
	}
	else
	{
		ReleaseCachedData( m_grannyRes );
	}

	if( m_grannyRes )
	{
		m_grannyRes->AddNotifyTarget( this );
	}
}

void Tr2GrannyPrimitiveSet::CreatePrimitive()
{
	if( !m_grannyRes )
	{
		return;
	}

	if( m_grannyRes->IsUsingCMF() )
	{
		CreatePrimitiveFromCMF();
	}
#if WITH_GRANNY
	else
	{
		CreatePrimitiveFromGranny();
	}
#endif
}

void Tr2GrannyPrimitiveSet::CreatePrimitiveFromCMF()
{
	if( !m_grannyRes )
	{
		return;
	}

	const cmf::Data* cmfData = m_grannyRes->GetCMFData();

	if( !cmfData )
	{
		CCP_LOGERR( "Tr2GrannyLineSet::CreatePrimitive: '%s' not found or not a valid CMF file", m_grannyResPath.c_str() );
		return;
	}

	uint32_t numVertices = 0;
	uint32_t numIndices = 0;
	std::vector<int32_t> meshIndices;
	int32_t pickingIndex = -1;

	// Sorting the meshes so the picking mesh is processed last
	for( int32_t meshIx = 0; meshIx < cmfData->meshes.size(); meshIx++ )
	{
		auto mesh = cmfData->meshes[meshIx];

		uint32_t meshIndexCount = cmf::GetStreamElementCount( mesh.lods[0].ib );
		uint32_t meshTriangleCount = meshIndexCount / 3;

		if( cmf::ToStdStringView( mesh.name ) == g_pickingMeshName )
		{
			m_pickingPrimitiveCount = meshTriangleCount;
			pickingIndex = meshIx;
		}
		else
		{
			meshIndices.push_back( meshIx );
			numIndices += meshIndexCount;
		}
	}

	if( pickingIndex > -1 )
	{
		m_pickingIndexOffset = numIndices;
		meshIndices.push_back( pickingIndex );
	}

	for( int32_t meshIx = 0; meshIx < cmfData->meshes.size(); meshIx++ )
	{
		auto mesh = cmfData->meshes[meshIndices[meshIx]];

		uint32_t meshIndexCount = cmf::GetStreamElementCount( mesh.lods[0].ib );
		uint32_t meshVertexCount = cmf::GetStreamElementCount( mesh.lods[0].vb );
		uint32_t meshTriangleCount = meshIndexCount / 3;
		m_primitiveCount += meshTriangleCount;

		m_points.reserve( m_points.size() + meshVertexCount );
		m_triangleIndices.reserve( m_triangleIndices.size() + meshIndexCount );
		m_lineIndices.reserve( ( m_triangleIndices.size() + meshIndexCount ) * 2 );

		CmfVertexReader reader( mesh.decl );
		if( !reader.posElem )
		{
			CCP_LOGERR( "Tr2GrannyPrimitiveSet::CreatePrimitiveFromCMF: mesh has no Position element" );
			continue;
		}
		auto vb = mesh.lods[0].vb;
		const auto* vbData = static_cast<const uint8_t*>( m_grannyRes->GetCMFViewData( vb ) );
		std::vector<Vector3> positions( meshVertexCount, Vector3( 0.f, 0.f, 0.f ) );
		std::vector<Vector3> normals( meshVertexCount, Vector3( 0.f, 0.f, 0.f ) );
		ReadCmfVertexAttributes( reader, vbData, meshVertexCount, vb.stride, positions, normals, nullptr, nullptr );

		for( uint32_t i = 0; i < meshVertexCount; i++ )
		{
			TriangleVertex newVertex;
			newVertex.m_position = positions[i];
			newVertex.m_normal = normals[i];
			newVertex.m_color = m_color;
			m_points.push_back( newVertex );
		}

		{
			auto ib = mesh.lods[0].ib;
			auto ibSectionData = m_grannyRes->GetCMFContents().GetSection( ib.index );

			auto indices = cmf::ConstIndexBufferStream( ibSectionData, ib );

			// The index buffer changes a bit since we put all the point into a single buffer
			for( uint32_t i = 0; i < meshIndexCount; i++ )
			{
				m_triangleIndices.push_back( indices[i] + numVertices );
			}
			// the lines just trace through each polygon, 1 - 2, 2 - 3, 3 - 1
			for( uint32_t i = 0; i < meshIndexCount; i += 3 )
			{
				m_lineIndices.push_back( indices[i] + numVertices );
				m_lineIndices.push_back( indices[i + 1] + numVertices );
				m_lineIndices.push_back( indices[i + 1] + numVertices );
				m_lineIndices.push_back( indices[i + 2] + numVertices );
				m_lineIndices.push_back( indices[i + 2] + numVertices );
				m_lineIndices.push_back( indices[i] + numVertices );
			}
		}

		numVertices += meshVertexCount;
	}
}

#if WITH_GRANNY
void Tr2GrannyPrimitiveSet::CreatePrimitiveFromGranny()
{
	if( !m_grannyRes )
	{
		return;
	}

	granny_file* file = m_grannyRes->GetGrannyFile();

	if( !file )
	{
		CCP_LOGERR( "Tr2GrannyLineSet::CreatePrimitive: '%s' not found or not a valid Granny file", m_grannyResPath.c_str() );
		return;
	}

	granny_file_info* info = GrannyGetFileInfo( file );
	if( !info )
	{
		CCP_LOGERR( "Tr2GrannyLineSet::CreatePrimitive: unable to obtain a granny_file_info from the input file\n" );
		return;
	}

	// reset the counters
	granny_int32x numVertices = 0;
	granny_int32x numIndices = 0;
	std::vector<int> meshIndices;
	int pickingIndex = -1;
	// Sorting the meshes so the picking mesh is processed last
	for( granny_int32x meshIx = 0; meshIx < info->MeshCount; ++meshIx )
	{
		granny_mesh* mesh = info->Meshes[meshIx];
		if( mesh == NULL )
		{
			return;
		}

		granny_int32x const meshIndexCount = GrannyGetMeshIndexCount( mesh );
		granny_int32x const meshTriangleCount = GrannyGetMeshTriangleCount( mesh );

		if( mesh->Name && strncmp( mesh->Name, g_pickingMeshName, strlen( g_pickingMeshName ) ) == 0 )
		{
			m_pickingPrimitiveCount = meshTriangleCount;
			pickingIndex = meshIx;
		}
		else
		{
			meshIndices.push_back( meshIx );
			numIndices += meshIndexCount;
		}
	}

	if( pickingIndex > -1 )
	{
		m_pickingIndexOffset = numIndices;
		meshIndices.push_back( pickingIndex );
	}

	for( granny_int32x meshIx = 0; meshIx < info->MeshCount; ++meshIx )
	{
		granny_mesh* mesh = info->Meshes[meshIndices[meshIx]];
		if( mesh == NULL )
		{
			return;
		}

		granny_int32x const meshIndexCount = GrannyGetMeshIndexCount( mesh );
		granny_int32x const meshVertexCount = GrannyGetMeshVertexCount( mesh );
		granny_int32x const meshTriangleCount = GrannyGetMeshTriangleCount( mesh );
		m_primitiveCount += meshTriangleCount;

		m_points.reserve( m_points.size() + meshVertexCount );
		m_triangleIndices.reserve( m_triangleIndices.size() + meshIndexCount );
		m_lineIndices.reserve( ( m_triangleIndices.size() + meshIndexCount ) * 2 );

		bool isHalfPrecision = ( mesh->PrimaryVertexData->VertexType[0].Type == GrannyReal16Member );
		void* vertices = mesh->PrimaryVertexData->Vertices;
		int pointOffset = m_grannyRes->GetVertexComponentOffset( meshIx, GrannyVertexPositionName );
		int normalOffset = m_grannyRes->GetVertexComponentOffset( meshIx, GrannyVertexNormalName );
		int bytesPerVertex = GrannyGetTotalObjectSize( mesh->PrimaryVertexData->VertexType );

		for( int i = 0; i < mesh->PrimaryVertexData->VertexCount; ++i )
		{
			if( isHalfPrecision )
			{
				granny_real16* posPtr = (granny_real16*)( (uint8_t*)vertices + ( i * bytesPerVertex ) + pointOffset );
				granny_real16* normalPtr = (granny_real16*)( (uint8_t*)vertices + ( i * bytesPerVertex ) + normalOffset );
				TriangleVertex newVertex;

				GrannyReal16ToReal32( *( posPtr ), &newVertex.m_position.x );
				GrannyReal16ToReal32( *( posPtr + 1 ), &newVertex.m_position.y );
				GrannyReal16ToReal32( *( posPtr + 2 ), &newVertex.m_position.z );

				GrannyReal16ToReal32( *normalPtr, &newVertex.m_normal.x );
				GrannyReal16ToReal32( *( normalPtr + 1 ), &newVertex.m_normal.y );
				GrannyReal16ToReal32( *( normalPtr + 2 ), &newVertex.m_normal.z );
				newVertex.m_color = m_color;
				m_points.push_back( newVertex );
			}
			else
			{
				granny_real32* const posPtr = (granny_real32*)( (uint8_t*)vertices + ( i * bytesPerVertex ) + pointOffset );
				granny_real32* const normalPtr = (granny_real32*)( (uint8_t*)vertices + ( i * bytesPerVertex ) + normalOffset );
				TriangleVertex newVertex;

				newVertex.m_position.x = *posPtr;
				newVertex.m_position.y = *( posPtr + 1 );
				newVertex.m_position.z = *( posPtr + 2 );

				newVertex.m_normal.x = *normalPtr;
				newVertex.m_normal.y = *( normalPtr + 1 );
				newVertex.m_normal.z = *( normalPtr + 2 );
				newVertex.m_color = m_color;
				m_points.push_back( newVertex );
			}
		}
		// The index buffer changes a bit since we put all the point into a single buffer
		for( int i = 0; i < meshIndexCount; i++ )
		{
			if( mesh->PrimaryTopology->Indices )
			{
				m_triangleIndices.push_back( mesh->PrimaryTopology->Indices[i] + numVertices );
			}
			else
			{
				m_triangleIndices.push_back( mesh->PrimaryTopology->Indices16[i] + numVertices );
			}
		}
		// the lines just trace through each polygon, 1 - 2, 2 - 3, 3 - 1
		for( int i = 0; i < meshIndexCount; i += 3 )
		{
			if( mesh->PrimaryTopology->Indices )
			{
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices[i] + numVertices );
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices[i + 1] + numVertices );
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices[i + 1] + numVertices );
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices[i + 2] + numVertices );
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices[i + 2] + numVertices );
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices[i] + numVertices );
			}
			else
			{
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices16[i] + numVertices );
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices16[i + 1] + numVertices );
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices16[i + 1] + numVertices );
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices16[i + 2] + numVertices );
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices16[i + 2] + numVertices );
				m_lineIndices.push_back( mesh->PrimaryTopology->Indices16[i] + numVertices );
			}
		}

		numVertices += meshVertexCount;
	}
}
#endif

void Tr2GrannyPrimitiveSet::SetCurrentColor( Color& val )
{
	for( auto& point : m_points )
	{
		point.m_color = val;
	}
	PrepareResources();
}
