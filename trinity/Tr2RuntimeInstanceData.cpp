// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2RuntimeInstanceData.h"
#include "Utilities/BoundingBox.h"
#include "Particle/Tr2ParticleSystem.h"
#include "Tr2VertexDefinitionUtilities.h"
#include "Resources/TriGeometryRes.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2RuntimeInstanceData default constructor
// --------------------------------------------------------------------------------------
Tr2RuntimeInstanceData::Tr2RuntimeInstanceData( IRoot* lockobj ) :
	m_count( 0 ),
	m_stride( 0 ),
	m_vertexDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_aabb( Vector3( 0, 0, 0 ), Vector3( 0, 0, 0 ) ),
	m_explicitBoundingBox( false )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Saves current particle data into a CMF file.
// Arguments:
//   resPath - Resource path of the CMF file to save particle data to.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::SaveToCMF( const char* resPath ) const
{
	if( !m_data )
	{
		return;
	}

	std::wstring resPathW = (const wchar_t*)CA2W( resPath );
	std::wstring filename = BePaths->ResolvePathForWritingW( resPathW );

#if WITH_GRANNY
	if( filename.size() >= 4 && filename.compare( filename.size() - 4, 4, L".gr2" ) == 0 )
	{
		return SaveToGranny( resPath );
	}
#endif

	cmf::MemoryAllocator allocator;
	cmf::BufferManager bufferAllocator( allocator );
	cmf::Span<cmf::VertexElement> cmfVertexDecl = allocator.AllocateSpan<cmf::VertexElement>( m_layout.m_items.size() );

	if( !ConvertVertexDeclToCMF( m_layout, cmfVertexDecl, (uint32_t)cmfVertexDecl.size() ) )
	{
		return;
	}

	uint32_t vertexBufferStride = m_stride;
	uint32_t vertexBufferSize = vertexBufferStride * m_count;

	cmf::Data cmfData;
	cmfData.meshes = allocator.AllocateSpan<cmf::Mesh>( 1 );

	cmf::Mesh& cmfMesh = cmfData.meshes[0] = {};
	cmfMesh.name = allocator.AllocateString( m_name );
	cmfMesh.decl = cmfVertexDecl;
	cmfMesh.lods = allocator.AllocateSpan<cmf::MeshLod>( 1 );

	cmf::MeshLod& cmfMeshLod = cmfMesh.lods[0] = {};
	cmfMeshLod.vb = bufferAllocator.AddBuffer( m_data.get(), vertexBufferSize, vertexBufferStride );

	auto file = cmf::BuildFile( cmfData, bufferAllocator );

	FILE* outFile;
	auto err = fopen_s( &outFile, CW2A( filename.c_str() ), "wb" );

	if( err != 0 )
	{
		CCP_LOGERR( "Failed to open output file: %ls\n", filename.c_str() );
		return;
	}
	if( fwrite( file.data(), 1, file.size(), outFile ) != file.size() )
	{
		CCP_LOGERR( "Failed to write output file: %ls\n", filename.c_str() );
		fclose( outFile );
		return;
	}
	fclose( outFile );
}

#if WITH_GRANNY
// --------------------------------------------------------------------------------------
// Description:
//   Saves current particle data into a granny file.
// Arguments:
//   resPath - Resource path of the granny file to save particle data to.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::SaveToGranny( const char* resPath ) const
{
	if( !m_data )
	{
		return;
	}

	std::wstring resPathW = (const wchar_t*)CA2W( resPath );
	std::wstring filename = BePaths->ResolvePathForWritingW( resPathW );

	if( filename.size() >= 4 && filename.compare( filename.size() - 4, 4, L".cmf" ) == 0 )
	{
		return SaveToCMF( resPath );
	}

	granny_data_type_definition* definition = CCP_NEW( "Tr2RuntimeInstanceData::SaveToGranny" ) granny_data_type_definition[m_layout.m_items.size() + 1];
	if( !ConvertVertexDeclToGranny( m_layout, definition, (unsigned int)( m_layout.m_items.size() ) ) )
	{
		return;
	}

	granny_vertex_data* vertexData = CCP_NEW( "Tr2RuntimeInstanceData::SaveToGranny" ) granny_vertex_data;
	memset( vertexData, 0, sizeof( granny_vertex_data ) );
	vertexData->VertexType = definition;
	vertexData->VertexComponentNames = CCP_NEW( "Tr2RuntimeInstanceData::SaveToGranny" ) const char * [m_layout.m_items.size()];
	for( unsigned i = 0; i < m_layout.m_items.size(); ++i )
	{
		vertexData->VertexComponentNames[i] = definition[i].Name;
	}
	vertexData->VertexComponentNameCount = (granny_int32)m_layout.m_items.size();

	if( m_count )
	{
		char* vertices = CCP_NEW( "Tr2RuntimeInstanceData::SaveToGranny" ) char[m_stride * m_count];
		memcpy( vertices, m_data.get(), m_stride * m_count );
		vertexData->Vertices = reinterpret_cast<granny_uint8*>( vertices );
	}
	vertexData->VertexCount = m_count;

	// We don't need index buffer, but TriGeometryRes will complain/crash if tries
	// to load granny without index buffer.
	granny_tri_topology* topology = CCP_NEW( "Tr2RuntimeInstanceData::SaveToGranny" ) granny_tri_topology;
	memset( topology, 0, sizeof( granny_tri_topology ) );
	topology->Index16Count = 3;
	topology->Indices16 = CCP_NEW( "Tr2RuntimeInstanceData::SaveToGranny" ) granny_uint16[3];
	topology->Indices16[0] = 0;
	topology->Indices16[1] = 0;
	topology->Indices16[2] = 0;

	granny_mesh* mesh = CCP_NEW( "Tr2RuntimeInstanceData::SaveToGranny" ) granny_mesh;
	memset( mesh, 0, sizeof( granny_mesh ) );
	mesh->Name = m_name.c_str();
	mesh->PrimaryVertexData = vertexData;
	mesh->PrimaryTopology = topology;

	granny_file_info info;
	memset( &info, 0, sizeof( granny_file_info ) );

	info.MeshCount = 1;
	info.Meshes = &mesh;
	info.VertexDataCount = 1;
	info.VertexDatas = &vertexData;
	info.TriTopologyCount = 1;
	info.TriTopologies = &topology;

	granny_file_builder* builder = GrannyBeginFile(
		1,
		GrannyCurrentGRNStandardTag,
		GrannyGRNFileMV_ThisPlatform,
		GrannyGetTemporaryDirectory(),
		"prefix2" );
	granny_file_data_tree_writer* writer = GrannyBeginFileDataTreeWriting( GrannyFileInfoType, &info, 0, 0 );

	GrannyWriteDataTreeToFileBuilder( writer, builder );
	GrannyEndFileDataTreeWriting( writer );

	GrannyEndFile( builder, CW2A( filename.c_str() ) );

	CCP_DELETE mesh;
	CCP_DELETE[] topology->Indices16;
	CCP_DELETE topology;
	CCP_DELETE[] vertexData->Vertices;
	CCP_DELETE[] vertexData->VertexComponentNames;
	CCP_DELETE vertexData;
	CCP_DELETE[] definition;
}
#endif

// --------------------------------------------------------------------------------------
// Description:
//   Tr2RuntimeInstanceData destructor
// --------------------------------------------------------------------------------------
Tr2RuntimeInstanceData::~Tr2RuntimeInstanceData()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Invalidates vertex declaration.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::ReleaseResources( TriStorage s )
{
	m_vertexDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Recreates instance data vertex buffer
//   and vertex declaration if needed.
// Return value:
//   true If GPU resources are created successfully
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2RuntimeInstanceData::OnPrepareResources()
{
	if( m_data && m_stride && m_count )
	{
		if( !m_vb.IsValid() )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			auto hr = g_sharedBuffer.Allocate( m_stride, m_count, m_data.get(), renderContext, m_vb );
			if( FAILED( hr ) )
			{
				return false;
			}
		}
	}
	if( m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION && !m_layout.m_items.empty() )
	{
		m_vertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( m_layout );
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Query if instance data is available for
//   rendering.
// Return value:
//   true If instance data is available
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2RuntimeInstanceData::IsInstanceDataReady() const
{
	return m_vertexDeclaration != Tr2EffectStateManager::UNINITIALIZED_DECLARATION && m_vb.IsValid();
}

ITr2InstanceData::InstanceData Tr2RuntimeInstanceData::GetInstanceData( unsigned int bufferIndex, float screenSize ) const
{
	return {
		m_vb.GetBuffer(), m_vb.GetOffset(), m_stride, m_count
	};
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns vertex declaration handle for
//   instance buffer.
// Arguments:
//   bufferIndex - (unused) instance buffer index
// Return value:
//   Vertex declaration handle
// --------------------------------------------------------------------------------------
unsigned int Tr2RuntimeInstanceData::GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const
{
	return m_vertexDeclaration;
}

CcpMath::AxisAlignedBox Tr2RuntimeInstanceData::GetInstanceBufferBoundingBox( unsigned int bufferIndex ) const
{
	return m_aabb;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GpuBuffer interface. Returns GPU buffer with particle data.
// Arguments:
//   bufferIndex - instance buffer index
// Return Value:
//   GPU buffer containing instance data
// --------------------------------------------------------------------------------------
Tr2BufferAL* Tr2RuntimeInstanceData::GetGpuBuffer( unsigned bufferIndex )
{
	return &m_vb.GetBuffer();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter interface. Does nothing as this emitter only emits
//   particles on demand.
// Arguments:
//   arguments - (unused) update arguments
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::Update( const UpdateArguments& arguments )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter interface. Does nothing as this emitter only emits
//   particles on demand.
// Arguments:
//   arguments - (unused) update arguments
//   position - (unused) parent position
//   velocity - (unused) parent velocity
//   rateModifier - (unused) emit rate modifier
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::SpawnParticles( const UpdateArguments& arguments,
											 const Vector3* position,
											 const Vector3* velocity,
											 float rateModifier )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter interface. Does nothing as this emitter only emits
//   particles on demand.
// Arguments:
//   arguments - (unused) update arguments
//   positionStart - (unused) parent position at the start of the frame
//   positionEnd - (unused) parent position at the end of the frame
//   velocityStart - (unused) parent velocity at the start of the frame
//   velocityStart - (unused) parent velocity at the end of the frame
//   deltaTime - (unused) frame time
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::SpawnParticles( const UpdateArguments& arguments,
											 const Vector3* positionStart,
											 const Vector3* positionEnd,
											 const Vector3* velocityStart,
											 const Vector3* velocityEnd,
											 float deltaTime )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter. Notify the emitter that it's spawn functions are going
//   to be called in multi-threaded scenario (during particle system update). Since this
//   "emitter" does not emit with SpawnParticles methods we don't need to do anything here.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::SetThreadSafeFlag()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns number of instances.
// Return value:
//   Nmber of instances
// --------------------------------------------------------------------------------------
unsigned Tr2RuntimeInstanceData::GetCount() const
{
	return m_count;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets vertex layout for instance data. This invalidates all data.
// Arguments:
//   layout - new vertex layout
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::SetLayout( const Tr2VertexDefinition& layout )
{
	m_data.reset();
	g_sharedBuffer.Free( m_vb );
	m_stride = 0;
	m_count = 0;
	for( auto it = layout.m_items.begin(); it != layout.m_items.end(); ++it )
	{
		if( it->m_stream != 0 )
		{
			CCP_LOGERR( "Tr2PythonInstanceData: vertex layout needs to reference stream 0 only" );
			return;
		}
		m_stride = std::max( m_stride, it->m_offset + layout.GetDataTypeSizeInBytes( it->m_dataType ) );
	}
	m_layout = layout;
	m_vertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( m_layout );
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns current vertex layout for instance data.
// Return value:
//   Current vertex layout
// --------------------------------------------------------------------------------------
const Tr2VertexDefinition& Tr2RuntimeInstanceData::GetLayout() const
{
	return m_layout;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns current instance data (read-only).
// Return value:
//   Current instance data
// --------------------------------------------------------------------------------------
const void* Tr2RuntimeInstanceData::GetData() const
{
	return m_data.get();
}

// --------------------------------------------------------------------------------------
// Description:
//   Allocates memory for instance data (if passed instance count is different from
//   previous one) and returns it.
// Arguments:
//   count - Number of instances
// Return value:
//   Instance data buffer
// --------------------------------------------------------------------------------------
void* Tr2RuntimeInstanceData::GetData( unsigned count )
{
	if( count != m_count )
	{
		m_count = count;
		if( m_stride && m_count )
		{
			m_data.reset( new char[m_stride * m_count] );
			g_sharedBuffer.Free( m_vb );
		}
		else
		{
			m_data.reset();
			return nullptr;
		}
	}

	if( !m_data )
	{
		return nullptr;
	}

	return m_data.get();
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates GPU vertsion of instance data. Should be called after data is modified.
// Arguments:
//   count - Number of instances
// Return value:
//   Instance data buffer
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::UpdateData()
{
	if( !m_data )
	{
		return;
	}
	if( m_vb.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_vb.Update( m_data.get(), renderContext );
	}

	if( !m_vb.IsValid() )
	{
		PrepareResources();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates bounding box (if m_updateBoundingBox flag is set) after data is modified.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::UpdateBoundingBox()
{
	if( m_explicitBoundingBox )
	{
		return;
	}
	unsigned positionOffset = -1;
	for( auto it = m_layout.m_items.begin(); it != m_layout.m_items.end(); ++it )
	{
		if( it->m_usage == Tr2VertexDefinition::POSITION &&
			it->m_usageIndex == 0 &&
			Tr2VertexDefinition::GetDataTypeSizeInMembers( it->m_dataType ) >= 3 )
		{
			positionOffset = it->m_offset;
			break;
		}
	}
	m_aabb = CcpMath::AxisAlignedBox();
	if( positionOffset != -1 )
	{
		for( unsigned i = 0; i < m_count; ++i )
		{
			const Vector3* pos =
				reinterpret_cast<Vector3*>( m_data.get() + positionOffset + m_stride * i );
			m_aabb.Include( *pos );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Just return the aa bounding box data.
// --------------------------------------------------------------------------------------
bool Tr2RuntimeInstanceData::GetBoundingBox( Vector3& minAabb, Vector3& maxAabb ) const
{
	minAabb = m_aabb.m_min;
	maxAabb = m_aabb.m_max;
	return true;
}

void Tr2RuntimeInstanceData::SetBoundingBox( const CcpMath::AxisAlignedBox& aabb )
{
	m_aabb = aabb;
	m_explicitBoundingBox = true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Destroys instance data.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::DestroyData()
{
	g_sharedBuffer.Free( m_vb );
	m_data.reset();
	m_count = 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns per-instance data size.
// Return value:
//   Per-instance data size
// --------------------------------------------------------------------------------------
unsigned Tr2RuntimeInstanceData::GetStride() const
{
	return m_stride;
}

namespace
{
// --------------------------------------------------------------------------------------
// Description:
//   Mapping between particle element declaration and D3D vertex declaration in particle
//   data vertex buffer.
// --------------------------------------------------------------------------------------
struct DeclarationMapping
{
	// Element offset in vertex buffer
	unsigned inOffset;
	// Buffer type for particle element
	Tr2ParticleElementData::BufferType buffer;
	// Element offset in the particle buffer
	unsigned offset;
	// Length/dimension of particle element
	unsigned length;
	// Is the element in vertex buffer type FLOAT16 (needs convertion to 32 bit float)
	bool isFloat16;
};
}

// --------------------------------------------------------------------------------------
// Description:
//   Spawns particles into the particle system.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::Spawn()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_particleSystem && m_particleSystem->IsValid() &&
		!m_layout.m_items.empty() && m_data )
	{
		// Validate geometry vertex declaration against particle system element declaration
		const Tr2ParticleElementDataMap& particleElements = m_particleSystem->GetElementDeclaration();
		std::vector<DeclarationMapping> geometryDeclarationMap;
		for( auto i = particleElements.begin(); i != particleElements.end(); ++i )
		{
			bool found = false;
			for( const auto& item : m_layout.m_items )
			{
				unsigned usageIndex = 0;
				if( i->first.m_type == Tr2ParticleElementDeclarationName::CUSTOM )
				{
					usageIndex = i->second.m_usageIndex;
				}
				if( item.m_usage == i->first.GetD3DUsage() && item.m_usageIndex == usageIndex )
				{
					DeclarationMapping mapping;
					mapping.inOffset = item.m_offset;
					mapping.buffer = i->second.m_bufferType;
					mapping.offset = i->second.m_offset;
					mapping.length = i->second.m_dimension;

					if( item.m_dataType >= m_layout.FLOAT32_1 + int( i->second.m_dimension ) - 1 &&
						item.m_dataType <= m_layout.FLOAT32_4 )
					{
						mapping.isFloat16 = false;
					}
					else if( ( item.m_dataType == m_layout.FLOAT16_2 && i->second.m_dimension <= 2 ) ||
							 item.m_dataType == m_layout.FLOAT16_4 )
					{
						CCP_LOGWARN(
							"Particle elements \"%s\" has FLOAT16 type in Tr2RuntimeInstanceData \"%s\", this degrades emitter performance",
							i->first.GetName().c_str(),
							m_name.c_str() );
						mapping.isFloat16 = true;
					}
					else
					{
						CCP_LOGERR( "Incompatible type for particle elements \"%s\" in Tr2RuntimeInstanceData \"%s\"",
									i->first.GetName().c_str(),
									m_name.c_str() );
						return;
					}
					geometryDeclarationMap.push_back( mapping );
					found = true;
					break;
				}
			}
			if( !found )
			{
				CCP_LOGERR( "No data for particle elements \"%s\" in Tr2RuntimeInstanceData \"%s\"",
							i->first.GetName().c_str(),
							m_name.c_str() );
				return;
			}
		}

		m_particleSystem->ClearParticles();

		// Spawn particles
		char* data = m_data.get();
		for( unsigned i = 0; i < m_count; ++i )
		{
			float* particle[Tr2ParticleElementData::COUNT];
			if( m_particleSystem->InsertParticle( particle ) )
			{
				for( auto j = geometryDeclarationMap.begin(); j != geometryDeclarationMap.end(); ++j )
				{
					if( j->isFloat16 )
					{
						std::copy(
							static_cast<Float_16*>( static_cast<void*>( data + j->inOffset ) ),
							static_cast<Float_16*>( static_cast<void*>( data + j->inOffset ) ) + j->length,
							particle[j->buffer] + j->offset );
					}
					else
					{
						std::copy(
							static_cast<float*>( static_cast<void*>( data + j->inOffset ) ),
							static_cast<float*>( static_cast<void*>( data + j->inOffset ) ) + j->length,
							particle[j->buffer] + j->offset );
					}
				}
				m_particleSystem->DoneInsertingParticle();
			}
			data += m_stride;
		}
	}
}
