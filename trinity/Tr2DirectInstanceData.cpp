// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2DirectInstanceData.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2DirectInstanceData default constructor
// --------------------------------------------------------------------------------------
Tr2DirectInstanceData::Tr2DirectInstanceData( IRoot* lockobj ) :
	m_count( 0 ),
	m_stride( 0 ),
	m_vertexDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_aabb( Vector3( 0, 0, 0 ), Vector3( 0, 0, 0 ) )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Invalidates vertex declaration, and
// resets buffer to an invalid state.
// --------------------------------------------------------------------------------------
void Tr2DirectInstanceData::ReleaseResources( TriStorage s )
{
	m_vertexDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer = Tr2BufferAL();
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Recreates vertex buffer and vertex
//   declaration if needed.
// Return value:
//   true If GPU resources are created successfully
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2DirectInstanceData::OnPrepareResources()
{
	if( m_stride && m_count )
	{
		if( !m_vertexBuffer.IsValid() || m_vertexBuffer.GetDesc().count < m_count )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			auto hr = m_vertexBuffer.Create(
				m_stride,
				m_count,
				Tr2GpuUsage::VERTEX_BUFFER,
				Tr2CpuUsage::WRITE_OFTEN,
				nullptr,
				renderContext );
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
bool Tr2DirectInstanceData::IsInstanceDataReady() const
{
	return m_vertexDeclaration != Tr2EffectStateManager::UNINITIALIZED_DECLARATION && m_vertexBuffer.IsValid();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface.
// Arguments:
//   bufferIndex - (unused) instance buffer index
//   screenSize - (unused) screen size
// Return value:
//   Returns instance data: the vertex buffer, offset, stride and count.
// --------------------------------------------------------------------------------------
ITr2InstanceData::InstanceData Tr2DirectInstanceData::GetInstanceData( unsigned int bufferIndex, float screenSize ) const
{
	return {
		m_vertexBuffer, 0, m_stride, m_count
	};
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface.
// Arguments:
//   bufferIndex - (unused) instance buffer index
// Return value:
//   Returns the vertex declaration.
// --------------------------------------------------------------------------------------
unsigned int Tr2DirectInstanceData::GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const
{
	return m_vertexDeclaration;
}

CcpMath::AxisAlignedBox Tr2DirectInstanceData::GetInstanceBufferBoundingBox( unsigned int bufferIndex ) const
{
	return m_aabb;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns number of instances.
// Return value:
//   Nmber of instances
// --------------------------------------------------------------------------------------
unsigned Tr2DirectInstanceData::GetCount() const
{
	return m_count;
}

// --------------------------------------------------------------------------------------
// Description:
//   Invalidates vertex buffer, prepares instance data for a new vertex format,
//   ensures vertex declaration matches the layout provided, and that all attributes
//   come from stream 0.
// --------------------------------------------------------------------------------------
void Tr2DirectInstanceData::SetLayout( const Tr2VertexDefinition& layout )
{
	m_vertexBuffer = Tr2BufferAL();
	m_stride = 0;
	m_count = 0;
	for( auto it = layout.m_items.begin(); it != layout.m_items.end(); ++it )
	{
		if( it->m_stream != 0 )
		{
			CCP_LOGERR( "Tr2DirectInstanceData: vertex layout needs to reference stream 0 only" );
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
const Tr2VertexDefinition& Tr2DirectInstanceData::GetLayout() const
{
	return m_layout;
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
void* Tr2DirectInstanceData::GetData( unsigned count )
{
	if( count != m_count )
	{
		m_count = count;
	}

	if( !m_stride || !m_count )
	{
		return nullptr;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_vertexBuffer.IsValid() || m_vertexBuffer.GetDesc().count < m_count )
	{
		m_vertexBuffer = Tr2BufferAL();
		auto hr = m_vertexBuffer.Create(
			m_stride,
			m_count,
			Tr2GpuUsage::VERTEX_BUFFER,
			Tr2CpuUsage::WRITE_OFTEN,
			nullptr,
			renderContext );
		if( FAILED( hr ) )
		{
			return nullptr;
		}
	}
	uint8_t* data = nullptr;
	if( FAILED( m_vertexBuffer.MapForWriting( data, renderContext ) ) )
	{
		return nullptr;
	}

	return data;
}

// --------------------------------------------------------------------------------------
// Description:
//   Unmaps the vertex buffer that was mapped for writing, making data available to the GPU,
//   with an early exit if buffer is invalid. Should be called after data is modified.
// --------------------------------------------------------------------------------------
void Tr2DirectInstanceData::UpdateData()
{
	CCP_STATS_ZONE( __FUNCTION__ );
	if( !m_vertexBuffer.IsValid() )
	{
		return;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	m_vertexBuffer.UnmapForWriting( renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the bounding box.
// --------------------------------------------------------------------------------------
void Tr2DirectInstanceData::SetBoundingBox( const CcpMath::AxisAlignedBox& aabb )
{
	m_aabb = aabb;
}

// --------------------------------------------------------------------------------------
// Description:
//   Destroys instance data.
// --------------------------------------------------------------------------------------
void Tr2DirectInstanceData::DestroyData()
{
	m_vertexBuffer = Tr2BufferAL();
	m_count = 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns per-instance data size.
// Return value:
//   Per-instance data size
// --------------------------------------------------------------------------------------
unsigned Tr2DirectInstanceData::GetStride() const
{
	return m_stride;
}