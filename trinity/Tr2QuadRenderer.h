// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2QuadRenderer_H
#define Tr2QuadRenderer_H

#include "Tr2DynamicRingBuffer.h"
#include "TbbStub.h"
#include "ITr2Renderable.h"

BLUE_DECLARE( Tr2Material );

// --------------------------------------------------------------------------------------
// Description:
//   This class allows batch rendering of quads. Users of the class can register several
//   effects along with specific instance vertex layout. To render quads users then call
//   AddQuads function to add instance data every frame. The class handles instance and
//   quad buffer management and rendering added data.
// See Also:
//   EveSpaceScene, EveSpriteSet
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2QuadRenderer ) :
	public IRoot, public Tr2DeviceResource
{
public:
	Tr2QuadRenderer( IRoot* parent = nullptr );

	EXPOSE_TO_BLUE();

	typedef uint64_t EffectKey;

	void RegisterEffect( EffectKey key, TriBatchType batchType, uint32_t instanceSize, uint32_t quadCount, const Tr2VertexDefinition& definition, Tr2Material* effect );
	void UnregisterEffect( EffectKey key );

	void AddQuads( EffectKey effectKey, const void* sprites, size_t count );
	void BeginRendering( Tr2RenderContext & renderContext );
	void DoneRendering( Tr2RenderContext & renderContext );

	void GetBatches( TriBatchType batchType, ITriRenderBatchAccumulator * accumulator );

	virtual void ReleaseResources( TriStorage s );

	uint32_t GetInstanceBufferSize() const;
	uint32_t GetInstanceDataSize() const;

	static Tr2QuadRenderer* Instance();

private:
	struct PerThreadData
	{
		PerThreadData() :
			addedSize( 0 )
		{
		}

		// Per-effect, per-thread accumulation buffer
		CcpMallocBuffer buffer;
		// Size of data in the buffer
		uint32_t addedSize;
	};

	struct EffectRecord
	{
		// Effect to use
		Tr2MaterialPtr effect;
		// Batch type
		TriBatchType batchType;
		// Size of instance vertex in bytes
		uint32_t instanceSize;
		// Vertex declaration
		unsigned int vertexDeclHandle;
		// Offset to instance data in the shared instance buffer
		uint32_t bufferOffset;
		// Number of instances in the buffer
		uint32_t count;
		// Number of quads to render per instance
		uint32_t quadCount;
		// Vertex definition
		Tr2VertexDefinition definition;
		// Thread local storage used when adding instances
		Tr2EnumerableThreadSpecific<PerThreadData> combinable;
	};

	virtual bool OnPrepareResources();

	uint32_t MergeBuffers();
	void UpdateInstanceBuffer( Tr2RenderContext & renderContext );
	void RecreateQuadBuffers( uint32_t quadCount );

	// Map of registered effects
	TrackableStdUnorderedMap<EffectKey, EffectRecord*> m_effects;

	// Total size of all added data this frame
	CcpAtomic<uint32_t> m_bufferSize;
	// In-memory buffer for storing all instance data
	CcpAlignedMallocBuffer m_buffer;

	// Vertex buffer with quad vertex numbers
	Tr2BufferAL m_quad;
	// Index buffer for the quad
	Tr2BufferAL m_quadIB;
	// Instance data buffer
	Tr2RingVertexBuffer m_vertexBuffer;
	// Offset into m_vertexBuffer where the last frame data is stored
	uint32_t m_vertexBufferOffset;
	// Size of the instance data added last frame
	uint32_t m_lastInstanceDataSize;
	uint32_t m_bufferAlignment;
};

TYPEDEF_BLUECLASS( Tr2QuadRenderer );

#endif