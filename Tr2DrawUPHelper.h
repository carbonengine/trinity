#pragma once
#ifndef Tr2DrawUPHelper_h_
#define Tr2DrawUPHelper_h_

#include "ALResult.h"
#include "include/Tr2BufferAL.h"

class Tr2RenderContextAL;

namespace TrinityALImpl
{

	class Tr2DrawUPHelper
	{
	public:
		Tr2DrawUPHelper();
		~Tr2DrawUPHelper();

		ALResult DrawIndexedPrimitiveUP(
			Tr2RenderContextEnum::Topology topology,
			uint32_t numVertices,
			uint32_t primitiveCount,
			const uint32_t* indexData,
			const void* vertexStreamZeroData,
			uint32_t vertexStreamZeroStride,
			Tr2RenderContextAL& renderContext,
			Tr2PrimaryRenderContextAL& device );

		ALResult DrawIndexedPrimitiveUP(
			Tr2RenderContextEnum::Topology topology,
			uint32_t numVertices,
			uint32_t primitiveCount,
			const uint16_t* indexData,
			const void* vertexStreamZeroData,
			uint32_t vertexStreamZeroStride,
			Tr2RenderContextAL& renderContext,
			Tr2PrimaryRenderContextAL& device );

		ALResult DrawPrimitiveUP(
			Tr2RenderContextEnum::Topology topology,
			uint32_t primitiveCount,
			const void* vertexStreamZeroData,
			uint32_t VertexStreamZeroStride,
			Tr2RenderContextAL& renderContext,
			Tr2PrimaryRenderContextAL& device );

		void Destroy();

	private:
		// Vertex and index buffer to emulate the Draw...UP calls from DX9
		static const uint32_t DRAW_UP_RING_SIZE = 4;

		uint32_t m_nextRingVB, m_nextRingIB16, m_nextRingIB32;
		::Tr2BufferAL m_vertexUP[DRAW_UP_RING_SIZE];
		::Tr2BufferAL m_indexUP16[DRAW_UP_RING_SIZE];
		::Tr2BufferAL m_indexUP32[DRAW_UP_RING_SIZE];

		ALResult FillUPVertexBuffer(
			uint32_t vertexCount,
			const void* vertexStreamZeroData,
			uint32_t vertexStreamZeroStride,
			::Tr2BufferAL& buffer,
			Tr2RenderContextAL& renderContext,
			Tr2PrimaryRenderContextAL& device );

		ALResult FillUPIndexBuffer(
			uint32_t indexCount,
			const uint16_t* indices,
			::Tr2BufferAL& buffer,
			Tr2RenderContextAL& renderContext,
			Tr2PrimaryRenderContextAL& device );

		ALResult FillUPIndexBuffer(
			uint32_t indexCount,
			const uint32_t* indices,
			::Tr2BufferAL& buffer,
			Tr2RenderContextAL& renderContext,
			Tr2PrimaryRenderContextAL& device );

		ALResult FillUPIndexBuffer(
			uint32_t indexCount,
			const void* indices,
			uint32_t bytesPerIndex,
			::Tr2BufferAL& buffer,
			Tr2RenderContextAL& renderContext,
			Tr2PrimaryRenderContextAL& device );

		Tr2DrawUPHelper( const Tr2DrawUPHelper& ) /* = delete */;
		Tr2DrawUPHelper& operator=( const Tr2DrawUPHelper& ) /* = delete */;
	};

}

#endif 
