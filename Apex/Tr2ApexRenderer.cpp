#include "StdAfx.h"

#if APEX_ENABLED

#include "Tr2ApexRenderer.h"
#include "Utilities/GeometryUtils.h"
#include "TriRenderBatch.h"
#include "TriSettingsRegistrar.h"
#include "Tr2PerObjectData.h"
#include "Tr2VertexDefinitionUtilities.h"
#include "Tr2Effect.h"

Tr2ApexRenderer s_apexRenderer;
Tr2ApexRenderer& g_apexRenderer = s_apexRenderer;

using namespace Tr2RenderContextEnum;

// If 'g_apexRendererVerbose' is true, then Apex renderer logs out information about D3D objects created.
// If 'g_apexRendererVerbose' is false the Apex renderer is silent.
bool g_apexRendererVerbose = false;
TRI_REGISTER_SETTING( "apexRendererVerbose", g_apexRendererVerbose );

#define APEX_LOG( msg, ... ) if( g_apexRendererVerbose ) { CCP_LOG( msg, __VA_ARGS__  ); }

static Tr2RenderContextEnum::BufferUsage ApexHintToD3DUsage( physx::apex::NxRenderBufferHint::Enum hint )
{
	switch( hint )
	{
		case physx::apex::NxRenderBufferHint::STATIC:
			return Tr2RenderContextEnum::USAGE_CPU_READ | Tr2RenderContextEnum::USAGE_CPU_WRITE;
			
		case physx::apex::NxRenderBufferHint::DYNAMIC:
		case physx::apex::NxRenderBufferHint::STREAMING:
			return Tr2RenderContextEnum::USAGE_CPU_WRITE | Tr2RenderContextEnum::USAGE_LOCK_FREQUENTLY;			
	}

	return Tr2RenderContextEnum::USAGE_CPU_WRITE;
}

class Tr2ApexVertexBuffer : 
	public physx::apex::NxUserRenderVertexBuffer, 
	public Tr2DeviceResource
{
public:
	Tr2ApexVertexBuffer( const physx::apex::NxUserRenderVertexBufferDesc &desc ) :
		m_vertexCount( 0 ),
		m_usage( Tr2RenderContextEnum::USAGE_CPU_READ | Tr2RenderContextEnum::USAGE_CPU_WRITE )
	{
		CCP_ASSERT( desc.isValid() );

		m_vertexCount = desc.maxVerts;
		m_uvOrigin = desc.uvOrigin;

		m_vertexDef = BuildFromApex( &desc.buffersRequest[0], &m_semanticOffset[0], &m_semanticByteSize[0] );
		m_vertexSize = m_vertexDef.m_nextOffset[0];
		if( !m_vertexDef.empty() )
		{
			m_usage = ApexHintToD3DUsage( desc.hint );
			PrepareResources();
		}

		m_isDirty = false;
		m_vertexBufferMirrorSize = m_vertexCount * m_vertexSize;
		m_vertexBufferMirror = CCP_NEW( "Tr2ApexVertexBuffer/m_vertexBufferMirror" ) uint8_t[m_vertexBufferMirrorSize];

	}

	~Tr2ApexVertexBuffer()
	{
		CCP_DELETE m_vertexBufferMirror;
		m_vertexBufferMirror = NULL;
	}

	Tr2VertexBufferAL& GetVertexBuffer()
	{
		return m_vertexBuffer;
	}

	unsigned int GetVertexSize()
	{
		return m_vertexSize;
	}

	const Tr2VertexDefinition& GetVertexDescription()
	{
		return m_vertexDef;
	}

	void Commit( Tr2RenderContext& renderContext )
	{
		if( !m_isDirty || !m_vertexBuffer.IsValid() )
		{
			return;
		}

		CCP_ASSERT( m_vertexBufferMirrorSize >= (unsigned int)(m_vertexCount * m_vertexSize) );

		void* p = NULL;
		HRESULT hr = m_vertexBuffer.Lock( 0, 0, &p, Tr2RenderContextEnum::LOCK_WRITEONLY, renderContext );
		if( SUCCEEDED( hr ) )
		{
			memcpy( p, m_vertexBufferMirror, m_vertexCount * m_vertexSize );
			m_vertexBuffer.Unlock( renderContext );
		}

		m_isDirty = false;
	}

	virtual void writeBuffer(const physx::apex::NxApexRenderVertexBufferData& data, physx::PxU32 firstVertex, physx::PxU32 numVertices)
	{
		for (physx::PxU32 i = 0; i < physx::apex::NxRenderVertexSemantic::NUM_SEMANTICS; i++)
		{
			physx::apex::NxRenderVertexSemantic::Enum apexSemantic = (physx::apex::NxRenderVertexSemantic::Enum)i;
			const physx::apex::NxApexRenderSemanticData& semanticData = data.getSemanticData(apexSemantic);
			if (semanticData.data)
			{
				writeBuffer((physx::apex::NxRenderVertexSemantic::Enum)i,semanticData.data,semanticData.stride,firstVertex,numVertices);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// NxUserRenderVertexBuffer
	void writeBuffer( physx::apex::NxRenderVertexSemantic::Enum semantic, const void *srcData, NxU32 srcStride, NxU32 firstDestElement, NxU32 numElements )
	{
		m_isDirty = true;

		uint8_t* end = m_vertexBufferMirror + m_vertexBufferMirrorSize;

		uint8_t* dst = m_vertexBufferMirror + m_semanticOffset[semantic];
		uint8_t* src = (uint8_t*)srcData;
		int size = m_semanticByteSize[semantic];

		CCP_ASSERT( dst + (numElements-1)*m_vertexSize + size <= m_vertexBufferMirror + m_vertexBufferMirrorSize );

		switch( semantic )
		{
			case physx::apex::NxRenderVertexSemantic::TEXCOORD0:
			case physx::apex::NxRenderVertexSemantic::TEXCOORD1:
			case physx::apex::NxRenderVertexSemantic::TEXCOORD2:
			case physx::apex::NxRenderVertexSemantic::TEXCOORD3:
				CCP_ASSERT( size == sizeof( float ) * 2 );

				switch( m_uvOrigin )
				{
					case physx::apex::NxTextureUVOrigin::ORIGIN_TOP_LEFT:
						for( unsigned int i = 0; i < numElements; ++i )
						{
							CCP_ASSERT( dst + size <= end );

							float* fDst = (float*)dst;
							float* fSrc = (float*)src;
							fDst[0] = fSrc[0];
							fDst[1] = 1.0f - fSrc[1];

							dst += m_vertexSize;
							src += srcStride;
						}
						break;

					case physx::apex::NxTextureUVOrigin::ORIGIN_TOP_RIGHT:
						CCP_ASSERT_M( false, "NxTextureUVOrigin::ORIGIN_TOP_RIGHT: Not supported" );
					case physx::apex::NxTextureUVOrigin::ORIGIN_BOTTOM_LEFT:
						for( unsigned int i = 0; i < numElements; ++i )
						{
							CCP_ASSERT( dst + size <= end );

							memcpy( dst, src, size );
							dst += m_vertexSize;
							src += srcStride;
						}
						break;

					case physx::apex::NxTextureUVOrigin::ORIGIN_BOTTOM_RIGHT:
						CCP_ASSERT_M( false, "NxTextureUVOrigin::ORIGIN_BOTTOM_RIGHT: Not supported" );
				}
				break;

			case physx::apex::NxRenderVertexSemantic::TANGENT:
				switch( m_uvOrigin )
				{
					case physx::apex::NxTextureUVOrigin::ORIGIN_TOP_LEFT:
						for( unsigned int i = 0; i < numElements; ++i )
						{
							CCP_ASSERT( dst + size <= end );

							float* fDst = (float*)dst;
							float* fSrc = (float*)src;
							fDst[0] = -fSrc[0];
							fDst[1] = -fSrc[1];
							fDst[2] = -fSrc[2];

							dst += m_vertexSize;
							src += srcStride;
						}
						break;

					case physx::apex::NxTextureUVOrigin::ORIGIN_TOP_RIGHT:
						CCP_ASSERT_M( false, "NxTextureUVOrigin::ORIGIN_TOP_RIGHT: Not supported" );
					case physx::apex::NxTextureUVOrigin::ORIGIN_BOTTOM_LEFT:
						for( unsigned int i = 0; i < numElements; ++i )
						{
							CCP_ASSERT( dst + size <= end );

							memcpy( dst, src, size );
							dst += m_vertexSize;
							src += srcStride;
						}
						break;

					case physx::apex::NxTextureUVOrigin::ORIGIN_BOTTOM_RIGHT:
						CCP_ASSERT_M( false, "NxTextureUVOrigin::ORIGIN_BOTTOM_RIGHT: Not supported" );
				}
				break;

			default:
				for( unsigned int i = 0; i < numElements; ++i )
				{
					CCP_ASSERT( dst + size <= end );

					memcpy( dst, src, size );
					dst += m_vertexSize;
					src += srcStride;
				}
				break;
		}
	}
	void writeBuffer( physx::apex::NxAuthObjTypeID moduleId, NxU32 semantic, const void *srcData, NxU32 srcStride, NxU32 firstDestElement, NxU32 numElements )
	{
		// Not sure if this is needed
		CCP_ASSERT( false );
	}
	// NxUserRenderVertexBuffer
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s )
	{
		if( s & m_vertexBuffer.GetMemoryClass() )
		{
			m_vertexBuffer.Destroy();
		}
	}

private:
	bool OnPrepareResources()
	{
		// Buffer might exist, if we're coming back from a lost device state
		// and the buffer was created in the managed pool.
		if( !m_vertexBuffer.IsValid() )
		{
			APEX_LOG( "Creating Apex vertex buffer: %d verts, %d bytes per vertex", m_vertexCount, m_vertexSize );
			if( g_apexRendererVerbose )
			{
				DescribeVertexDecl( m_vertexDef );
			}

			USE_MAIN_THREAD_RENDER_CONTEXT();
			m_vertexBuffer.Create( m_vertexCount * m_vertexSize, m_usage, nullptr, renderContext );
		}

		return true;
	}
	// ITriDeviceResource
	//////////////////////////////////////////////////////////////////////////

private:
	Tr2VertexDefinition m_vertexDef;

	Tr2VertexBufferAL m_vertexBuffer;

	uint8_t* m_vertexBufferMirror;
	unsigned int m_vertexBufferMirrorSize;

	bool m_isDirty;

	// Number of vertices in the vertex buffer
	int m_vertexCount;

	// Size of the vertex in bytes - also the stride
	int m_vertexSize;

	// Usage flag for the vertex buffer
	Tr2RenderContextEnum::BufferUsage m_usage;

	// Offset in the vertex where a given semantic lives.
	int m_semanticOffset[physx::apex::NxRenderVertexSemantic::NUM_SEMANTICS];

	// Size of the data in the vertex for a given semantic.
	int m_semanticByteSize[physx::apex::NxRenderVertexSemantic::NUM_SEMANTICS];

	physx::apex::NxTextureUVOrigin::Enum m_uvOrigin;
};

class Tr2ApexIndexBuffer : 
	public physx::apex::NxUserRenderIndexBuffer,
	public Tr2DeviceResource
{
public:
	Tr2ApexIndexBuffer( const physx::apex::NxUserRenderIndexBufferDesc &desc )
	{
		CCP_ASSERT( desc.isValid() );
		CCP_ASSERT( desc.format != physx::apex::NxRenderDataFormat::UNSPECIFIED );

		m_indexCount = desc.maxIndices;

		switch( desc.format )
		{
			case physx::apex::NxRenderDataFormat::UBYTE1:
			case physx::apex::NxRenderDataFormat::USHORT1:
				m_indexByteSize = 2;
				m_use16Bit = true;
				break;
			case physx::apex::NxRenderDataFormat::UINT1:
				m_indexByteSize = 4;
				m_use16Bit = false;
				break;

			default:
				CCP_ASSERT( false );
				m_indexByteSize = 0;
				m_use16Bit = true;
				break;
		}
		m_usage = ApexHintToD3DUsage( desc.hint );
		
		PrepareResources();
	}

	~Tr2ApexIndexBuffer()
	{
	}

	Tr2IndexBufferAL& GetIndexBuffer()
	{
		return m_indexBuffer;
	}

	//////////////////////////////////////////////////////////////////////////
	// NxUserRenderIndexBuffer
	void writeBuffer( const void *srcData, NxU32 srcStride, NxU32 firstDestElement, NxU32 numElements )
	{
		if( !m_indexBuffer.IsValid() )
		{
			return;
		}

		USE_MAIN_THREAD_RENDER_CONTEXT();

		unsigned int offsetToLock = firstDestElement * m_indexByteSize;
		unsigned int sizeToLock = numElements * m_indexByteSize;
		void* p = NULL;
		HRESULT hr = m_indexBuffer.Lock( offsetToLock, sizeToLock, &p, Tr2RenderContextEnum::LOCK_WRITEONLY, renderContext );
		if( SUCCEEDED( hr ) )
		{
			uint8_t* dst = (uint8_t*)p;
			uint8_t* src = (uint8_t*)srcData;
			if( srcStride == m_indexByteSize )
			{
				memcpy( dst, src, m_indexByteSize * numElements );
			}
			else
			{
				for( unsigned int i = 0; i < numElements; ++i )
				{
					memcpy( dst, src, m_indexByteSize );
					dst += m_indexByteSize;
					src += srcStride;
				}
			}

			m_indexBuffer.Unlock( renderContext );
		}
	}
	// NxUserRenderIndexBuffer
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s )
	{
		if( s & m_indexBuffer.GetMemoryClass() )
		{
			m_indexBuffer.Destroy();
		}
	}

private:
	bool OnPrepareResources()
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		unsigned int size = m_indexByteSize * m_indexCount;

		// Buffer might exist, if we're coming back from a lost device state
		// and the buffer was created in the managed pool.
		if( !m_indexBuffer.IsValid() )
		{
			APEX_LOG( "Creating Apex index buffer: %d indices, %d bytes per index", m_indexCount, m_indexByteSize )
			USE_MAIN_THREAD_RENDER_CONTEXT();
			HRESULT hr = m_indexBuffer.Create( 
								m_indexCount, 
								m_usage, 
								m_use16Bit ? IB_16BIT : IB_32BIT, 
								nullptr, 
								renderContext );
		}

		return true;
	}
	// ITriDeviceResource
	//////////////////////////////////////////////////////////////////////////

private:
	Tr2IndexBufferAL m_indexBuffer;
	bool m_use16Bit;
	int m_indexByteSize;
	int m_indexCount;

	// Usage flag for the index buffer
	Tr2RenderContextEnum::BufferUsage	m_usage;
};

class Tr2ApexBoneBuffer : public physx::apex::NxUserRenderBoneBuffer
{
public:
	Tr2ApexBoneBuffer( const physx::apex::NxUserRenderBoneBufferDesc &desc )
	{
		m_maxBones = desc.maxBones;
		m_bones = CCP_NEW( "Tr2ApexBoneBuffer/m_bones" ) NxMat34[m_maxBones];
	}

	~Tr2ApexBoneBuffer()
	{
		CCP_DELETE [] m_bones;
	}

	virtual void writeBuffer(const physx::apex::NxApexRenderBoneBufferData& data, physx::PxU32 firstBone, physx::PxU32 numBones)	
	{
		const physx::apex::NxApexRenderSemanticData& semanticData = data.getSemanticData(physx::apex::NxRenderBoneSemantic::POSE);
		if ( semanticData.data )
		{
			writeBuffer(physx::apex::NxRenderBoneSemantic::POSE,semanticData.data,semanticData.stride,firstBone,numBones);
		}
		else
		{
			CCP_ASSERT(false); // encountered an unexpected semantic data type.
		}

	}

	//////////////////////////////////////////////////////////////////////////
	// NxUserRenderBoneBuffer
	void writeBuffer( physx::apex::NxRenderBoneSemantic::Enum semantic, const void *srcData, NxU32 srcStride, NxU32 firstDestElement, NxU32 numElements )
	{
		if( semantic == physx::apex::NxRenderBoneSemantic::POSE && m_bones )
		{
			CCP_ASSERT( srcStride == sizeof(NxMat34));
			for( unsigned int i = 0; i < numElements; ++i )
			{
				m_bones[firstDestElement+i] = *(const NxMat34*)srcData;
				srcData = ((uint8_t*)srcData)+srcStride;
			}
		}
	}

	NxMat34* m_bones;
	unsigned int m_maxBones;
};

class Tr2ApexInstanceBuffer : public physx::apex::NxUserRenderInstanceBuffer
{
public:
	Tr2ApexInstanceBuffer( const physx::apex::NxUserRenderInstanceBufferDesc &desc )
	{

	}

	virtual void writeBuffer(const physx::apex::NxApexRenderInstanceBufferData& data, physx::PxU32 firstInstance, physx::PxU32 numInstances)
	{
		CCP_ASSERT( false ); // no yet implemented
	}

};

class Tr2ApexSpriteBuffer : public physx::apex::NxUserRenderSpriteBuffer
{
public:
	Tr2ApexSpriteBuffer( const physx::apex::NxUserRenderSpriteBufferDesc& desc )
	{

	}

	//////////////////////////////////////////////////////////////////////////
	// NxUserRenderSpriteBuffer
	virtual void writeBuffer(const physx::apex::NxApexRenderSpriteBufferData& data, physx::PxU32 firstSprite, physx::PxU32 numSprites)
	{
		CCP_ASSERT( false ); // Not yet implemented
	}
};

class Tr2ApexMesh;
class Tr2ApexBatch : public TriRenderBatch
{
public:
	void SetMesh( Tr2ApexMesh* val )
	{
		m_mesh = val;
	}

	Tr2ApexMesh* GetMesh() const
	{
		return m_mesh;
	}

	// -------------------------------------------------------------
	// Description:
	//   Sets a flag indicating that a reversed triangle order and
	//   culling is required (instead of normal order). 
	// Arguments:
	//   reversedOrder - If true - render triangles in reversed order
	//                   If false - render triangles in normal order
	// -------------------------------------------------------------
	void SetReversedOrder( bool reversedOrder )
	{
		m_reversedOrder = reversedOrder;
	}

	//////////////////////////////////////////////////////////////////////////
	// TriRenderBatch
	void SubmitGeometry( Tr2RenderContext& renderContext );

	// For debugging in PIX
	virtual const std::string& GetBatchTypeName( void ) const 
	{ 
		return Tr2ApexBatch::s_batchTypeName; 
	}
	static const std::string s_batchTypeName;

private:
	Tr2ApexMesh* m_mesh;
	unsigned int m_firstVertex;
	unsigned int m_firstIndex;
	unsigned int m_primitiveCount;
	unsigned int m_vertexCount;
	// Render triangles in reversed order
	bool m_reversedOrder;
	Tr2RenderContextEnum::Topology m_primitiveType;
};

const std::string Tr2ApexBatch::s_batchTypeName = "Tr2ApexBatch";

class Tr2ApexMesh : 
	public physx::apex::NxUserRenderResource,
	public Tr2DeviceResource
{
public:
	Tr2ApexMesh( const physx::apex::NxUserRenderResourceDesc& desc ) :
		m_firstVertex( 0 ),
		m_vertexCount( 0 ),
		m_firstIndex( 0 ),
		m_indexCount( 0 ),
		m_vertexDecl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
		m_skinned( false )
	{
		m_vertexBufferCount = desc.numVertexBuffers;
		m_vertexBuffers = CCP_NEW( "Tr2ApexMesh/m_vertexBuffers" ) Tr2ApexVertexBuffer*[m_vertexBufferCount];

		int curDescIx = 0;

		for( unsigned int i = 0; i < m_vertexBufferCount; ++i )
		{
			m_vertexBuffers[i] = static_cast<Tr2ApexVertexBuffer*>( desc.vertexBuffers[i] );

			Tr2VertexDefinition newDesc = m_vertexBuffers[i]->GetVertexDescription();
			if( !i )
			{
				m_vertexDesc = newDesc;
			}
			else
			{
				for( auto it = begin( newDesc.m_items ); it != end( newDesc.m_items ); ++it )
				{
					it->m_stream = i;
				}
				m_vertexDesc.m_items.insert( m_vertexDesc.m_items.end(), newDesc.m_items.begin(), newDesc.m_items.end() );
			}			
		}

		m_vertexCount = desc.numVerts;

		m_indexBuffer = static_cast<Tr2ApexIndexBuffer*>( desc.indexBuffer );
		m_boneBuffer = desc.boneBuffer;
		m_numBones = desc.numBones;
		m_firstBone = desc.firstBone;

		switch( desc.primitives )
		{
			case physx::apex::NxRenderPrimitiveType::TRIANGLES:
				m_primitiveType = Tr2RenderContextEnum::TOP_TRIANGLES;
				m_primitiveCount = m_indexCount / 3;
				break;
			case physx::apex::NxRenderPrimitiveType::TRIANGLE_STRIP:
				m_primitiveType = Tr2RenderContextEnum::TOP_TRIANGLE_STRIP;
				m_primitiveCount = m_indexCount - 2;
				break;
			case physx::apex::NxRenderPrimitiveType::LINES:
				m_primitiveType = Tr2RenderContextEnum::TOP_LINES;
				m_primitiveCount = m_indexCount / 3;
				break;
			case physx::apex::NxRenderPrimitiveType::LINE_STRIP:
				m_primitiveType = Tr2RenderContextEnum::TOP_LINE_STRIP;
				m_primitiveCount = m_indexCount - 1;
				break;
			case physx::apex::NxRenderPrimitiveType::POINTS:
				m_primitiveType = Tr2RenderContextEnum::TOP_POINTS;
				m_primitiveCount = m_indexCount;
				break;
			case physx::apex::NxRenderPrimitiveType::POINT_SPRITES:
				CCP_ASSERT_M( false, "NxRenderPrimitiveType::POINT_SPRITES not supported" );
				m_primitiveType = Tr2RenderContextEnum::TOP_POINTS;
				m_primitiveCount = m_indexCount;
				break;

			default:
				CCP_ASSERT_M( false, "Unknown primitive type" );
				break;
		}

		PrepareResources();
	}

	~Tr2ApexMesh()
	{
		CCP_DELETE [] m_vertexBuffers;
	}

	// -------------------------------------------------------------
	// Description:
	//   Reverses indexes in the original mesh index buffer effectively 
	//   reversing triangle and culling order. This is required for
	//   proper backface rendering of hair. If the mesh does not 
	//   contain an index buffer the reversed index buffer is created
	//   anyway (containing [N, N-1,..., 0] indexes.
	//   The function assumes the primitive type for the mesh is a
	//   triangle list.
	// Return value:
	//   Reversed index buffer
	// -------------------------------------------------------------
	Tr2IndexBufferAL* GetReversedIndexBuffer( Tr2RenderContext& renderContext )
	{
		if( m_reversedIndexBuffer.IsValid() )
		{
			return &m_reversedIndexBuffer;
		}

		if( !m_indexBuffer )
		{
			unsigned length = m_vertexCount;
			if( length == 0 )
			{
				return nullptr;
			}
			std::vector<unsigned> invertedData( m_vertexCount );
			for( unsigned int i = 0; i < length; ++i )
			{
				invertedData[length - i - 1] = length - i - 1;
			}
			USE_MAIN_THREAD_RENDER_CONTEXT();
			if( FAILED( m_reversedIndexBuffer.Create( 
								length, 
								Tr2RenderContextEnum::USAGE_IMMUTABLE, 
								IB_32BIT, 
								&invertedData[0], 
								renderContext ) )	|| 
				!m_reversedIndexBuffer.IsValid() )
			{
				return nullptr;
			}
			return &m_reversedIndexBuffer;
		}

		Tr2IndexBufferAL& src = m_indexBuffer->GetIndexBuffer();
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			if( FAILED( m_reversedIndexBuffer.Create( 
									src.GetNumIndices(), 
									src.m_usage, 
									src.GetIBBitcount(), 
									nullptr, 
									renderContext ) ) || 
				!m_reversedIndexBuffer.IsValid() )
			{
				return nullptr;
			}
		}
		if( src.Is16Bit() )
		{
			unsigned short *originalData = nullptr;
			CR_RETURN_VAL( m_indexBuffer->GetIndexBuffer().Lock( 0, 0, originalData, LOCK_READONLY, renderContext ), nullptr );
			ON_BLOCK_EXIT( [&]{ m_indexBuffer->GetIndexBuffer().Unlock( renderContext ); } );

			unsigned short *invertedData = nullptr;
			CR_RETURN_VAL( m_reversedIndexBuffer.Lock( 0, 0, invertedData, LOCK_WRITEONLY, renderContext ), nullptr );
			ON_BLOCK_EXIT( [&]{ m_reversedIndexBuffer.Unlock( renderContext ); } );
			unsigned length = src.GetNumIndices();
			for( unsigned int i = 0; i < length; ++i )
			{
				invertedData[length - i - 1] = originalData[i];
			}
		}
		else
		{
			unsigned *originalData = nullptr;
			CR_RETURN_VAL( m_indexBuffer->GetIndexBuffer().Lock( 0, 0, originalData, LOCK_READONLY, renderContext ), nullptr );
			ON_BLOCK_EXIT( [&]{ m_indexBuffer->GetIndexBuffer().Unlock( renderContext ); } );

			unsigned *invertedData = nullptr;
			CR_RETURN_VAL( m_reversedIndexBuffer.Lock( 0, 0, invertedData, LOCK_WRITEONLY, renderContext ), nullptr );
			ON_BLOCK_EXIT( [&]{ m_reversedIndexBuffer.Unlock( renderContext ); } );

			unsigned length = src.GetNumIndices();
			for( unsigned int i = 0; i < length; ++i )
			{
				invertedData[length - i - 1] = originalData[i];
			}
		}

		return &m_reversedIndexBuffer;
	}

	// -------------------------------------------------------------
	// Description:
	//   Creates batches for cloth mesh. 
	// Arguments:
	//   accumulator - Render batch accumulator to add batches to
	//   perObjectData - Per-object data
	//   effect - Effect to use for normal order rendering
	//   reversedEffect - Effect to use for reversed order rendering
	// -------------------------------------------------------------
	void Render( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData, ITr2ShaderMaterial* effect, ITr2ShaderMaterial* reversedEffect, bool skinned )
	{
		if( !accumulator )
		{
			return;
		}

		unsigned int depth = g_apexRenderer.GetDepth();

		if( effect )
		{
			Tr2ApexBatch* batch = accumulator->Allocate<Tr2ApexBatch>();
			if( !batch )
			{
				return;
			}

			batch->SetMesh( this );
			batch->SetPerObjectData( perObjectData );
			batch->SetShaderMaterial( effect );
			batch->SetReversedOrder( false );
			batch->SetDepth( depth );
			accumulator->Commit( batch );
		}
		if( reversedEffect )
		{
			Tr2ApexBatch* batch = accumulator->Allocate<Tr2ApexBatch>();
			if( !batch )
			{
				return;
			}

			batch->SetMesh( this );
			batch->SetPerObjectData( perObjectData );
			batch->SetShaderMaterial( reversedEffect );
			batch->SetReversedOrder( true );
			batch->SetDepth( depth );
			accumulator->Commit( batch );
		}

		m_skinned = skinned;
	}

	// -------------------------------------------------------------
	// Description:
	//   Does the actual rendering of the mesh. 
	// Arguments:
	//   reverseOrder - If true the mesh is rendered with reversed
	//                  triangle and culling order
	//					If false the mesh is rendered with normal
	//                  triangle and culling order
	// -------------------------------------------------------------
	void SubmitGeometry( bool reverseOrder, Tr2RenderContext& renderContext )
	{
		for( unsigned int i = 0; i < m_vertexBufferCount; ++i )
		{
			Tr2ApexVertexBuffer* avb = m_vertexBuffers[i];
			avb->Commit( renderContext );
			unsigned int vertexSize = avb->GetVertexSize();
			renderContext.m_esm.ApplyStreamSource( i, avb->GetVertexBuffer(), m_firstVertex * vertexSize, vertexSize );
		}

		renderContext.m_esm.ApplyVertexDeclaration( m_vertexDecl );

		if( m_skinned && g_apexRenderer.m_skinnedVS && g_apexRenderer.m_skinnedVS->GetEffectRes() && g_apexRenderer.m_skinnedVS->GetEffectRes()->IsGood() )
		{
			//g_apexRenderer.m_skinnedVS->ApplyVertexShaderInputs( 0 );
			g_apexRenderer.m_skinnedVS->GetEffectRes()->ApplyShader( 0, Tr2RenderContextEnum::VERTEX_SHADER, renderContext );
		}

		if( !reverseOrder )
		{
			if( m_indexBuffer )
			{
				renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer->GetIndexBuffer() );
				renderContext.SetTopology( m_primitiveType );
				renderContext.DrawIndexedPrimitive( m_vertexCount, m_firstIndex, m_primitiveCount );
			}
			else
			{
				renderContext.SetTopology( m_primitiveType );
				renderContext.DrawPrimitive( 0, m_primitiveCount );
			}
		}
		else if( auto reverse = GetReversedIndexBuffer( renderContext ) )
		{
			renderContext.m_esm.ApplyIndexBuffer( *reverse );
			renderContext.SetTopology( m_primitiveType );
			renderContext.DrawIndexedPrimitive( m_vertexCount, m_firstIndex, m_primitiveCount );			
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// NxUserRenderResource
	void setVertexBufferRange(  NxU32 firstVertex,   NxU32 numVerts)
	{
		m_firstVertex = firstVertex;
		m_vertexCount = numVerts;

		switch( m_primitiveType )
		{
		case TOP_TRIANGLES:
			m_primitiveCount = m_vertexCount / 3;
			break;
		case TOP_TRIANGLE_STRIP:
			m_primitiveCount = m_vertexCount - 2;
			break;
		case TOP_LINES:
			m_primitiveCount = m_vertexCount / 3;
			break;
		case Tr2RenderContextEnum::TOP_LINE_STRIP:
			m_primitiveCount = m_vertexCount - 1;
			break;
		case TOP_POINTS:
			m_primitiveCount = m_vertexCount;
			break;
		default:
			CCP_ASSERT_M( false, "Unknown primitive type" );
			break;
		}
	}
	void setIndexBufferRange(   NxU32 firstIndex,    NxU32 numIndices)
	{
		m_firstIndex = firstIndex;
		m_indexCount = numIndices;

		switch( m_primitiveType )
		{
		case TOP_TRIANGLES:
			m_primitiveCount = m_indexCount / 3;
			break;
		case TOP_TRIANGLE_STRIP:
			m_primitiveCount = m_indexCount - 2;
			break;
		case TOP_LINES:
			m_primitiveCount = m_indexCount / 3;
			break;
		case TOP_LINE_STRIP:
			m_primitiveCount = m_indexCount - 1;
			break;
		case TOP_POINTS:
			m_primitiveCount = m_indexCount;
			break;
		default:
			CCP_ASSERT_M( false, "Unknown primitive type" );
			break;
		}
	}

	void setBoneBufferRange(    NxU32 firstBone,     NxU32 numBones)
	{

	}
	void setInstanceBufferRange(NxU32 firstInstance, NxU32 numInstances)
	{

	}
	void setSpriteBufferRange(	NxU32 firstSprite,   NxU32 numSprites)
	{

	}
	void setMaterial( void *material )
	{

	}
	NxU32 getNbVertexBuffers() const
	{
		return m_vertexBufferCount;
	}
	physx::apex::NxUserRenderVertexBuffer* getVertexBuffer( NxU32 index ) const
	{
		return m_vertexBuffers[index];
	}
	physx::apex::NxUserRenderIndexBuffer* getIndexBuffer() const
	{
		return m_indexBuffer;
	}
	physx::apex::NxUserRenderBoneBuffer* getBoneBuffer()	const
	{
		return m_boneBuffer;
	}
	physx::apex::NxUserRenderInstanceBuffer* getInstanceBuffer()	const
	{
		return NULL;
	}
	physx::apex::NxUserRenderSpriteBuffer* getSpriteBuffer() const
	{
		return NULL;
	}
	// NxUserRenderResource
	//////////////////////////////////////////////////////////////////////////

	unsigned GetNumBones() const { return m_numBones; }
	unsigned GetFirstBone() const { return m_firstBone; }

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s )
	{
		if( s & TRISTORAGE_VIDEOMEMORY )
		{
			m_vertexDecl = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
			m_reversedIndexBuffer.Destroy();
		}
	}

private:
	bool OnPrepareResources()
	{
		m_vertexDecl = Tr2EffectStateManager::GetVertexDeclarationHandle( m_vertexDesc );

		APEX_LOG( "Creating vertex declaration for Apex mesh");
		if( g_apexRendererVerbose )
		{
			DescribeVertexDecl( m_vertexDecl );
		}

		return true;
	}
	// ITriDeviceResource
	//////////////////////////////////////////////////////////////////////////

private:
	unsigned int m_vertexBufferCount;
	Tr2ApexVertexBuffer** m_vertexBuffers;
	Tr2ApexIndexBuffer* m_indexBuffer;
	// An index buffer containing reversed order indexes
	Tr2IndexBufferAL	m_reversedIndexBuffer;
	physx::apex::NxUserRenderBoneBuffer* m_boneBuffer;
	unsigned m_numBones;
	unsigned m_firstBone;
	
	unsigned int m_firstVertex;
	unsigned int m_vertexCount;
	unsigned int m_firstIndex;
	unsigned int m_indexCount;
	bool m_skinned;

	Tr2VertexDefinition m_vertexDesc;
	unsigned int m_vertexDecl;

	Tr2RenderContextEnum::Topology m_primitiveType;
	unsigned int m_primitiveCount;
};

void Tr2ApexBatch::SubmitGeometry( Tr2RenderContext& renderContext )
{
	if( m_mesh )
	{
		m_mesh->SubmitGeometry( m_reversedOrder, renderContext );
	}
}

class Tr2ApexUserRenderResourceManager : public physx::apex::NxUserRenderResourceManager
{
	physx::apex::NxUserRenderVertexBuffer* createVertexBuffer( const physx::apex::NxUserRenderVertexBufferDesc &desc )
	{
		return CCP_NEW( "Tr2ApexVertexBuffer" ) Tr2ApexVertexBuffer( desc );
	}

	void releaseVertexBuffer( physx::apex::NxUserRenderVertexBuffer &buffer )
	{
		CCP_DELETE( &buffer );
	}

	physx::apex::NxUserRenderIndexBuffer* createIndexBuffer( const physx::apex::NxUserRenderIndexBufferDesc &desc )
	{
		return CCP_NEW( "Tr2ApexIndexBuffer" ) Tr2ApexIndexBuffer( desc );
	}

	void releaseIndexBuffer( physx::apex::NxUserRenderIndexBuffer &buffer )
	{
		CCP_DELETE( &buffer );
	}

	physx::apex::NxUserRenderBoneBuffer* createBoneBuffer( const physx::apex::NxUserRenderBoneBufferDesc &desc )
	{
		return CCP_NEW( "Tr2ApexBoneBuffer" ) Tr2ApexBoneBuffer( desc );
	}

	void releaseBoneBuffer( physx::apex::NxUserRenderBoneBuffer &buffer )
	{
		CCP_DELETE( &buffer );
	}

	physx::apex::NxUserRenderInstanceBuffer* createInstanceBuffer( const physx::apex::NxUserRenderInstanceBufferDesc &desc )
	{
		return CCP_NEW( "Tr2ApexInstanceBuffer" ) Tr2ApexInstanceBuffer( desc );
	}

	void releaseInstanceBuffer( physx::apex::NxUserRenderInstanceBuffer &buffer )
	{
		CCP_DELETE( &buffer );
	}

	physx::apex::NxUserRenderSpriteBuffer* createSpriteBuffer( const physx::apex::NxUserRenderSpriteBufferDesc &desc )
	{
		return CCP_NEW( "Tr2ApexSpriteBuffer" ) Tr2ApexSpriteBuffer( desc );
	}

	void releaseSpriteBuffer( physx::apex::NxUserRenderSpriteBuffer &buffer )
	{
		CCP_DELETE( &buffer );
	}

	physx::apex::NxUserRenderResource* createResource( const physx::apex::NxUserRenderResourceDesc &desc )
	{
		return CCP_NEW( "Tr2ApexMesh" ) Tr2ApexMesh( desc );
	}

	void releaseResource( physx::apex::NxUserRenderResource &resource )
	{
		CCP_DELETE( &resource );
	}

	NxU32 getMaxBonesForMaterial(void *material)
	{
		return 60;
	}
};


Tr2ApexRenderer::Tr2ApexRenderer() :
	m_accumulator( NULL ),
	m_perObjectData( NULL ),
	m_depth( 0xFFFFFFFF )
{
}

void Tr2ApexRenderer::renderResource( const physx::apex::NxApexRenderContext &context )
{
	if( !m_accumulator )
	{
		return;
	}

	if( !m_skinnedVS )
	{
		m_skinnedVS.CreateInstance();
		m_skinnedVS->SetEffectPathName( "res:/graphics/effect/managed/interior/avatar/ApexLodSkinningVSOnly.fx" );
	}

	Tr2ApexMesh* mesh = static_cast<Tr2ApexMesh*>( context.renderResource );

	if( !mesh || !m_perObjectData )
	{
		return;
	}

	if( const Tr2ApexBoneBuffer* const bones = static_cast<Tr2ApexBoneBuffer*>( context.renderResource->getBoneBuffer() ) )
	{
		if( Tr2PerAreaDataSkinned* areaData = m_accumulator->Allocate<Tr2PerAreaDataSkinned>() )
		{
			areaData->SetPerObjectData( *const_cast<Tr2PerObjectData*>(m_perObjectData) );
			areaData->SetUserData( const_cast<Tr2PerObjectData*>(m_perObjectData)->GetUserData() );

			areaData->SetJointCount( mesh->GetNumBones() );
			for( unsigned i = 0; i != mesh->GetNumBones(); ++i )
			{
				float shuffle[12];
				float* m43 = (float*)&bones->m_bones[i];

				shuffle[0] = m43[0];	shuffle[1] = m43[1];	shuffle[2] = m43[2];	shuffle[3] = m43[9];
				shuffle[4] = m43[3];	shuffle[5] = m43[4];	shuffle[6] = m43[5];	shuffle[7] = m43[10];
				shuffle[8] = m43[6];	shuffle[9] = m43[7];	shuffle[10]= m43[8];	shuffle[11]= m43[11];

				areaData->SetJointTransform( i, shuffle );
			}

			//mesh->Render( m_accumulator, areaData, m_effectApexLod, m_reversedEffectApexLod, false );
			mesh->Render( m_accumulator, areaData, m_effect, m_reversedEffect, true );
			return;
		}
	}

	mesh->Render( m_accumulator, m_perObjectData, m_effect, m_reversedEffect, false );
}

ITriRenderBatchAccumulator* Tr2ApexRenderer::GetAccumulator() const
{
	return m_accumulator;
}

void Tr2ApexRenderer::SetAccumulator( ITriRenderBatchAccumulator* val )
{
	m_accumulator = val;
}

const Tr2PerObjectData* Tr2ApexRenderer::GetPerObjectData() const
{
	return m_perObjectData;
}

void Tr2ApexRenderer::SetPerObjectData( const Tr2PerObjectData* val )
{
	m_perObjectData = val;
}

void Tr2ApexRenderer::SetEffect( ITr2ShaderMaterial* effect )
{
	m_effect = effect;
}

// -------------------------------------------------------------
// Description:
//   Sets an effect to use with reversed triangle order rendering.
// Arguments:
//   effect - An effect to use with reversed triangle order rendering
// -------------------------------------------------------------
void Tr2ApexRenderer::SetReversedEffect( ITr2ShaderMaterial* effect )
{
	m_reversedEffect = effect;
}

void Tr2ApexRenderer::SetEffectApexLod( ITr2ShaderMaterial* effect )
{
	m_effectApexLod = effect;
}

void Tr2ApexRenderer::SetReversedEffectApexLod( ITr2ShaderMaterial* effect )
{
	m_reversedEffectApexLod = effect;
}

// -------------------------------------------------------------
// Description:
//   Sets the depth value used for batch sorting.
// Arguments:
//   depth - The depth sorting value
// -------------------------------------------------------------
void Tr2ApexRenderer::SetDepth( unsigned int depth )
{
	m_depth = depth;
}

// -------------------------------------------------------------
// Description:
//   Returns the renderer's depth sorting value
// Return value:
//   The depth sorting value
// -------------------------------------------------------------
unsigned int Tr2ApexRenderer::GetDepth() const 
{ 
	return m_depth; 
}

Tr2ApexUserRenderResourceManager s_apexRenderResourceManager;
physx::apex::NxUserRenderResourceManager* g_apexRenderResourceManager = &s_apexRenderResourceManager;

#endif // APEX_ENABLED
