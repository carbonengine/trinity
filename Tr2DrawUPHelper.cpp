#include "StdAfx.h"

#if TRINITY_PLATFORM==TRINITY_DIRECTX11 || TRINITY_PLATFORM==TRINITY_OPENGL4

#include "Tr2DrawUPHelper.h"
#include "include/Tr2RenderContextAL.h"


using namespace Tr2RenderContextEnum;

Tr2DrawUPHelper::Tr2DrawUPHelper()
	: m_nextRingVB( 0 )
	, m_nextRingIB16( 0 )
	, m_nextRingIB32( 0 )
{}

Tr2DrawUPHelper::~Tr2DrawUPHelper()
{
	Destroy();
}

void Tr2DrawUPHelper::Destroy()
{
	for( unsigned i = 0; i != DRAW_UP_RING_SIZE; ++i )
	{
		m_vertexUP[i] = Tr2BufferAL();
		m_indexUP16[i] = Tr2BufferAL();
		m_indexUP32[i] = Tr2BufferAL();
	}

	m_nextRingVB = m_nextRingIB16 = m_nextRingIB32 = 0;
}

ALResult Tr2DrawUPHelper::FillUPVertexBuffer(	
	uint32_t vertexCount, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride, 
	Tr2BufferAL& buffer,
	Tr2RenderContextAL& renderContext )
{
	const uint32_t totalSize = vertexCount * vertexStreamZeroStride;

	if( totalSize == 0 )
	{
		return S_OK;
	}

	if( totalSize > buffer.GetDesc().count * buffer.GetDesc().stride )
	{
		CR_RETURN_HR( buffer.Create( 4, ( totalSize + 3 ) / 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE_OFTEN, nullptr, Tr2RenderContextAL::GetPrimaryRenderContext() ) );
	}

	void* mapped = nullptr;
	CR_RETURN_HR( buffer.MapForWriting( mapped, renderContext ) );
	if( !mapped )
	{
		buffer.UnmapForWriting( renderContext );
		return E_FAIL;
	}
	memcpy( mapped, vertexStreamZeroData, totalSize );
	buffer.UnmapForWriting( renderContext );

	return S_OK;
}

ALResult Tr2DrawUPHelper::DrawPrimitiveUP(		
	uint32_t primitiveCount, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride,
	Tr2RenderContextAL& renderContext )
{
	if( primitiveCount == 0 )
	{
		return S_OK;
	}

	CR_RETURN_HR( FillUPVertexBuffer(	renderContext.ComputeVertexCount( primitiveCount ), 
										vertexStreamZeroData, 
										vertexStreamZeroStride, 
										m_vertexUP[ m_nextRingVB ],
										renderContext ) );

	CR_RETURN_HR( renderContext.SetStreamSource(		
										0, 
										m_vertexUP[ m_nextRingVB ], 
										0, 
										vertexStreamZeroStride ) );

	m_nextRingVB = ( m_nextRingVB + 1 ) % DRAW_UP_RING_SIZE;

	return renderContext.DrawPrimitive( 0, primitiveCount );
}

ALResult Tr2DrawUPHelper::FillUPIndexBuffer(	
	uint32_t primitiveCount, 
	const uint16_t* indices, 
	Tr2BufferAL& buffer,
	Tr2RenderContextAL& renderContext )
{
	return FillUPIndexBuffer(	primitiveCount, 
								(const void*)indices, 
								2, 
								buffer, 
								renderContext );
}

ALResult Tr2DrawUPHelper::FillUPIndexBuffer(	
	uint32_t primitiveCount, 
	const uint32_t* indices, 
	Tr2BufferAL& buffer,
	Tr2RenderContextAL& renderContext )
{
	return FillUPIndexBuffer(	primitiveCount, 
								(const void*)indices, 
								4, 
								buffer,
								renderContext );
}

ALResult Tr2DrawUPHelper::FillUPIndexBuffer(	
	uint32_t primitiveCount, 
	const void* indices, 
	uint32_t bytesPerIndex, 
	Tr2BufferAL& buffer,
	Tr2RenderContextAL& renderContext )
{
	const uint32_t indexCount = renderContext.ComputeVertexCount( primitiveCount );
	const uint32_t totalSize = indexCount * bytesPerIndex;

	if( indexCount > buffer.GetDesc().count )
	{
		CR_RETURN_HR( buffer.Create( bytesPerIndex, indexCount, Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::WRITE_OFTEN, nullptr, Tr2RenderContextAL::GetPrimaryRenderContext() ) );
	}

	void* mapped = nullptr;
	CR_RETURN_HR( buffer.MapForWriting( mapped, renderContext ) );
	if( !mapped )
	{
		buffer.UnmapForWriting( renderContext );
		return E_FAIL;
	}
	memcpy( mapped, indices, totalSize );
	buffer.UnmapForWriting( renderContext );

	return S_OK;
}

ALResult Tr2DrawUPHelper::DrawIndexedPrimitiveUP(	
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint32_t* indexData, 
	const void* vertexStreamZeroData,
	uint32_t vertexStreamZeroStride,
	Tr2RenderContextAL& renderContext )
{
	if( primitiveCount == 0 )
	{
		return S_OK;
	}

	CR_RETURN_HR( FillUPVertexBuffer(	numVertices, 
										vertexStreamZeroData, 
										vertexStreamZeroStride, 
										m_vertexUP[ m_nextRingVB ],
										renderContext ) );

	CR_RETURN_HR( renderContext.SetStreamSource(		
										0, 
										m_vertexUP[ m_nextRingVB ], 
										0, 
										vertexStreamZeroStride ) );

	m_nextRingVB = ( m_nextRingVB + 1 ) % DRAW_UP_RING_SIZE;

	CR_RETURN_HR( FillUPIndexBuffer (	primitiveCount, 
										indexData, 
										4, 
										m_indexUP32[ m_nextRingIB32 ],
										renderContext )  );

	CR_RETURN_HR( renderContext.SetIndices( m_indexUP32[ m_nextRingIB32 ] ) );

	m_nextRingIB32 = ( m_nextRingIB32 + 1 ) % DRAW_UP_RING_SIZE;

	return renderContext.DrawIndexedPrimitive( numVertices, 0, primitiveCount );
}

ALResult Tr2DrawUPHelper::DrawIndexedPrimitiveUP(	
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint16_t* indexData, 
	const void* vertexStreamZeroData,
	uint32_t vertexStreamZeroStride,
	Tr2RenderContextAL& renderContext )
{
	if( primitiveCount == 0 )
	{
		return S_OK;
	}

	CR_RETURN_HR( FillUPVertexBuffer(	numVertices, 
										vertexStreamZeroData, 
										vertexStreamZeroStride, 
										m_vertexUP[ m_nextRingVB ],
										renderContext ) );

	CR_RETURN_HR( renderContext.SetStreamSource(
										0, 
										m_vertexUP[ m_nextRingVB ], 
										0, 
										vertexStreamZeroStride ) );

	m_nextRingVB = ( m_nextRingVB + 1 ) % DRAW_UP_RING_SIZE;
	
	CR_RETURN_HR( FillUPIndexBuffer (	primitiveCount, 
										indexData, 2, 
										m_indexUP16[ m_nextRingIB16 ],
										renderContext ) );

	CR_RETURN_HR( renderContext.SetIndices( m_indexUP16[ m_nextRingIB16 ] ) );

	m_nextRingIB16 = ( m_nextRingIB16 + 1 ) % DRAW_UP_RING_SIZE;

	return renderContext.DrawIndexedPrimitive( numVertices, 0, primitiveCount );
}

#endif // ( TRINITY_PLATFORM==TRINITY_DIRECTX11 )
