#pragma once
#ifndef Tr2DrawUPHelper_h_
#define Tr2DrawUPHelper_h_

#include "ALResult.h"
#include "include/Tr2BufferAL.h"

class Tr2RenderContextAL;

class Tr2DrawUPHelper
{
public:
	Tr2DrawUPHelper();
	~Tr2DrawUPHelper();

	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint32_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride,
		Tr2RenderContextAL& renderContext );

	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint16_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride,
		Tr2RenderContextAL& renderContext );

	ALResult DrawPrimitiveUP(
		uint32_t primitiveCount, 
		const void* vertexStreamZeroData, 
		uint32_t VertexStreamZeroStride,
		Tr2RenderContextAL& renderContext );

	void Destroy();
	
private:		
	// Vertex and index buffer to emulate the Draw...UP calls from DX9
	enum { DRAW_UP_RING_SIZE = 4 };
	uint32_t m_nextRingVB, m_nextRingIB16, m_nextRingIB32;
	Tr2BufferAL m_vertexUP[DRAW_UP_RING_SIZE];
	Tr2BufferAL m_indexUP16[DRAW_UP_RING_SIZE];
	Tr2BufferAL m_indexUP32[DRAW_UP_RING_SIZE];

	ALResult FillUPVertexBuffer(
		uint32_t vertexCount, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride, 
		Tr2BufferAL& buffer,
		Tr2RenderContextAL& renderContext );

	ALResult FillUPIndexBuffer(		
		uint32_t primitiveCount, 
		const uint16_t* indices, 
		Tr2BufferAL& buffer,
		Tr2RenderContextAL& renderContext );

	ALResult FillUPIndexBuffer(		
		uint32_t primitiveCount, 
		const uint32_t* indices, 
		Tr2BufferAL& buffer,
		Tr2RenderContextAL& renderContext );

	ALResult FillUPIndexBuffer(		
		uint32_t primitiveCount, 
		const void* indices, 
		uint32_t bytesPerIndex, 
		Tr2BufferAL& buffer,
		Tr2RenderContextAL& renderContext );

	Tr2DrawUPHelper( const Tr2DrawUPHelper& ) /* = delete */;
	Tr2DrawUPHelper& operator=( const Tr2DrawUPHelper& ) /* = delete */;
};

#endif //Tr2DrawUPHelper_h_
