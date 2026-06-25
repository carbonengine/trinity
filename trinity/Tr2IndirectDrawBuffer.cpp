// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2IndirectDrawBuffer.h"
#include "Tr2Renderer.h"
#include "../trinityal/metal/Tr2ShaderProgramALMetal.h"


CCP_STATS_DECLARE( sceneExecuteIndirectCount, "Trinity/AL/sceneExecuteIndirectCount", true, CST_COUNTER_LOW, "Number of ExecuteIndirect calls." );
CCP_STATS_DECLARE( sceneIndirectDrawCount, "Trinity/AL/sceneIndirectDrawCount", true, CST_COUNTER_LOW, "Number of indirect draw calls." );

Tr2IndirectDrawBufferLayout::Tr2IndirectDrawBufferLayout( const Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext )
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	:
	m_shaderProgram( program )
#endif
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	std::fill( std::begin( m_materialOffsets ), std::end( m_materialOffsets ), 0xffffffff );
	std::fill( std::begin( m_perObjectDataOffsets ), std::end( m_perObjectDataOffsets ), 0xffffffff );

	Tr2IndirectBufferLayoutAL layout;
	if( SUCCEEDED( program.TrinityALImpl_GetObject()->CreateCommandSignatures( layout, renderContext ) ) )
	{
		m_drawOffset = layout.drawArgOffset;
		m_stride = layout.drawIndexedStride;
		for( uint32_t i = 0; i < layout.constantBufferCount; ++i )
		{
			auto& cb = layout.constantBuffers[i];
			if( cb.registerIndex == 0 )
			{
				m_materialOffsets[cb.stage] = i * 8;
			}
			else if( cb.stage != Tr2RenderContextEnum::PIXEL_SHADER && cb.registerIndex == Tr2Renderer::GetPerObjectVSStartRegister() )
			{
				m_perObjectDataOffsets[cb.stage] = i * 8;
			}
			else if( cb.stage == Tr2RenderContextEnum::PIXEL_SHADER && cb.registerIndex == Tr2Renderer::GetPerObjectPSStartRegister() )
			{
				m_perObjectDataOffsets[Tr2RenderContextEnum::PIXEL_SHADER] = i * 8;
			}
		}
	}
#elif TRINITY_PLATFORM == TRINITY_METAL
	auto masks = program.TrinityALImpl_GetObject()->GetResourceMasks();
	m_hasMaterialData[0] = masks[Tr2RenderContextEnum::VERTEX_SHADER].constantBufferMask & ( 1 << METAL_CONST_BUFFER_OFFSET );
	m_hasMaterialData[1] = masks[Tr2RenderContextEnum::PIXEL_SHADER].constantBufferMask & ( 1 << METAL_CONST_BUFFER_OFFSET );
	m_hasPerObjectData[0] = masks[Tr2RenderContextEnum::VERTEX_SHADER].constantBufferMask & ( 1 << ( METAL_CONST_BUFFER_OFFSET + Tr2Renderer::GetPerObjectVSStartRegister() ) );
	m_hasPerObjectData[1] = masks[Tr2RenderContextEnum::PIXEL_SHADER].constantBufferMask & ( 1 << ( METAL_CONST_BUFFER_OFFSET + Tr2Renderer::GetPerObjectPSStartRegister() ) );
#endif
}


Tr2IndirectDrawBufferWriter::Tr2IndirectDrawBufferWriter( Tr2IndirectDrawBuffer& buffer, const Tr2IndirectDrawBufferLayout& layout, uint32_t commands, Tr2RenderContextAL& renderContext ) :
	m_layout( layout ),
	m_renderContext( renderContext ),
	m_commandCount( 0 )
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	m_allocation = buffer.Allocate( layout.m_stride * commands );
	m_buffer = static_cast<uint8_t*>( m_allocation.address );
#elif TRINITY_PLATFORM == TRINITY_METAL
	m_allocation = buffer.Allocate( commands );
	m_buffer = m_allocation.address;
#endif
}

void Tr2IndirectDrawBufferWriter::SetMaterialConstants( Tr2RenderContextEnum::ShaderType stage, const Tr2ConstantBufferAL& buffer )
{
	if( !HasMaterialConstants( stage ) )
	{
		return;
	}
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	auto addr = m_renderContext.UploadConstants( buffer );
	*reinterpret_cast<uint64_t*>( m_buffer + m_layout.m_materialOffsets[stage] ) = addr;
#elif TRINITY_PLATFORM == TRINITY_METAL
	auto addr = m_renderContext.UploadConstants( buffer );
	m_buffer->material[stage] = addr;
#endif
}

void Tr2IndirectDrawBufferWriter::SetPerObjectData( Tr2RenderContextEnum::ShaderType stage, const Tr2ConstantBufferAL& buffer )
{
	if( !HasPerObjectData( stage ) )
	{
		return;
	}
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	auto addr = m_renderContext.UploadConstants( buffer );
	*reinterpret_cast<uint64_t*>( m_buffer + m_layout.m_perObjectDataOffsets[stage] ) = addr;
#elif TRINITY_PLATFORM == TRINITY_METAL
	auto addr = m_renderContext.UploadConstants( buffer );
	m_buffer->perObject[stage] = addr;
#endif
}

void Tr2IndirectDrawBufferWriter::SetPerObjectData( Tr2RenderContextEnum::ShaderType stage, const void* data, size_t size )
{
	if( !HasPerObjectData( stage ) )
	{
		return;
	}
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	auto addr = m_renderContext.UploadConstants( data, uint32_t( size ) );
	*reinterpret_cast<uint64_t*>( m_buffer + m_layout.m_perObjectDataOffsets[stage] ) = addr;
#elif TRINITY_PLATFORM == TRINITY_METAL
	auto addr = m_renderContext.UploadConstants( data, size );
	m_buffer->perObject[stage] = addr;
#endif
}

void Tr2IndirectDrawBufferWriter::DrawIndexed( uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation )
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	D3D12_DRAW_INDEXED_ARGUMENTS args = { indexCountPerInstance, instanceCount, startIndexLocation, int32_t( baseVertexLocation ), startInstanceLocation };
	*reinterpret_cast<D3D12_DRAW_INDEXED_ARGUMENTS*>( m_buffer + m_layout.m_drawOffset ) = args;
#elif TRINITY_PLATFORM == TRINITY_METAL
	m_buffer->indexCountPerInstance = indexCountPerInstance;
	m_buffer->instanceCount = instanceCount;
	m_buffer->startIndexLocation = startIndexLocation;
	m_buffer->baseVertexLocation = baseVertexLocation;
	m_buffer->startInstanceLocation = startInstanceLocation;
#endif
}

void Tr2IndirectDrawBufferWriter::Next()
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	m_buffer += m_layout.m_stride;
	++m_commandCount;
#elif TRINITY_PLATFORM == TRINITY_METAL
	m_buffer++;
	++m_commandCount;
#endif
}

bool Tr2IndirectDrawBufferWriter::HasMaterialConstants( Tr2RenderContextEnum::ShaderType stage ) const
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	return m_layout.m_materialOffsets[stage] != 0xffffffff;
#elif TRINITY_PLATFORM == TRINITY_METAL
	return m_layout.m_hasMaterialData[stage];
#else
	return false;
#endif
}

bool Tr2IndirectDrawBufferWriter::HasPerObjectData( Tr2RenderContextEnum::ShaderType stage ) const
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	return m_layout.m_perObjectDataOffsets[stage] != 0xffffffff;
#elif TRINITY_PLATFORM == TRINITY_METAL
	return m_layout.m_hasPerObjectData[stage];
#else
	return false;
#endif
}


Tr2IndirectDrawBuffer::~Tr2IndirectDrawBuffer()
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( m_uploadBuffer && renderContext.IsValid() )
	{
		renderContext.ReleaseLater( m_uploadBuffer );
		renderContext.ReleaseLater( m_defaultBuffer );
	}
#endif
}

bool Tr2IndirectDrawBuffer::IsValid() const
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	return m_uploadBuffer != nullptr;
#elif TRINITY_PLATFORM == TRINITY_METAL
	return true;
#else
	return false;
#endif
}

void Tr2IndirectDrawBuffer::Create( uint32_t size )
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	Resize( size );
#elif TRINITY_PLATFORM == TRINITY_METAL
	m_pageSize = size;
#endif
}

Tr2IndirectDrawBuffer::Allocation Tr2IndirectDrawBuffer::Allocate( uint32_t size )
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	if( m_head >= m_tail )
	{
		if( m_head + int32_t( size ) > m_size )
		{
			if( m_tail >= int32_t( size ) )
			{
				m_head = 0;
			}
			else
			{
				CCP_LOGNOTICE( "Resizing indirect draw buffer" );
				Resize( m_size * 2 );
			}
		}
	}
	if( m_head < m_tail && m_head + int32_t( size ) >= m_tail )
	{
		CCP_LOGNOTICE( "Resizing indirect draw buffer" );
		Resize( m_size * 2 );
	}

	auto result = m_cpuAddr + m_head;

	int32_t start = m_head;
	int32_t end = m_head + size;

	CComPtr<ID3D12Resource> buffer;
	bool copied;

	const bool COPY_TO_DEVICE = true;
	if( COPY_TO_DEVICE )
	{
		buffer = m_defaultBuffer;
		copied = false;
	}
	else
	{
		buffer = m_uploadBuffer;
		copied = true;
	}


	m_regions.push_back( { m_frame, start, end, copied } );

	m_head += size;


	return { buffer, m_cpuAddr, result };
#elif TRINITY_PLATFORM == TRINITY_METAL
	if( m_pageOffset + size >= m_pageSize )
	{
		++m_page;
		m_pageOffset = 0;
	}
	if( m_page >= m_pages.size() )
	{
		m_pages.push_back( std::unique_ptr<DP[]>( new DP[m_pageSize] ) );
	}
	auto buffer = m_pages[m_page].get() + m_pageOffset;
	m_pageOffset += size;
	return { buffer };
#else
	return {};
#endif
}



void Tr2IndirectDrawBuffer::CopyArguments()
{

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	USE_MAIN_THREAD_RENDER_CONTEXT();


	/*
	* Because we use a ring buffer for the arguments, we get a discontinuity in the copies
	* if we end up wrapping around. This code batches the copy operations into up to 2
	* continuous copy ranges to minimize the GPU performance cost in all cases.
	*/

	struct CopyRange
	{
		int32_t start;
		int32_t end;
	};

	CopyRange copyRanges[2];
	int copyIndex = -1;

	for( auto it = begin( m_regions ); it != end( m_regions ); ++it )
	{
		if( it->copied )
		{
			continue;
		}

		it->copied = true;

		if( copyIndex == -1 || copyRanges[copyIndex].end != it->start )
		{
			//First copy, or copy is discontinuous with previous copy, so move on to the next batch.
			copyIndex++;

			CCP_ASSERT( copyIndex < 2 ); //We shouldn't ever need more than two copies.

			//Restart the range at the start of this region
			copyRanges[copyIndex].start = it->start;
		}
		//Expand the range to cover this region.
		copyRanges[copyIndex].end = it->end;
	}

	if( copyIndex == -1 )
	{
		return; //Nothing to copy
	}

	GPU_REGION( renderContext, "Copy arguments" );

	{
		D3D12_RESOURCE_TRANSITION_BARRIER transition = {
			m_defaultBuffer,
			0,
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
			D3D12_RESOURCE_STATE_COPY_DEST
		};

		D3D12_RESOURCE_BARRIER barrier = {
			D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			D3D12_RESOURCE_BARRIER_FLAG_NONE,
			transition
		};

		renderContext.m_commandList->ResourceBarrier( 1, &barrier );
	}

	for( int i = 0; i <= copyIndex; i++ )
	{
		int start = copyRanges[i].start;
		int end = copyRanges[i].end;
		renderContext.m_commandList->CopyBufferRegion( m_defaultBuffer, start, m_uploadBuffer, start, end - start );
	}

	{
		D3D12_RESOURCE_TRANSITION_BARRIER transition = {
			m_defaultBuffer,
			0,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
		};

		D3D12_RESOURCE_BARRIER barrier = {
			D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			D3D12_RESOURCE_BARRIER_FLAG_NONE,
			transition
		};

		renderContext.m_commandList->ResourceBarrier( 1, &barrier );
	}
#endif
}

void Tr2IndirectDrawBuffer::SetFrameNumbers( uint64_t recordingFrame, uint64_t completedFrame )
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	m_frame = recordingFrame;

	for( auto it = begin( m_regions ); it != end( m_regions ); ++it )
	{
		if( it->frame <= completedFrame )
		{
			m_tail = it->end;
			CCP_ASSERT( it->copied );
		}
		else
		{
			m_regions.erase( begin( m_regions ), it );
			break;
		}
	}
#elif TRINITY_PLATFORM == TRINITY_METAL
	m_page = 0;
	m_pageOffset = 0;
#endif
}

void Tr2IndirectDrawBuffer::Submit( const Tr2IndirectDrawBufferWriter& writer )
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	if( writer.m_commandCount )
	{
		auto offset = static_cast<uint8_t*>( writer.m_allocation.address ) - static_cast<uint8_t*>( writer.m_allocation.baseAddress );
		writer.m_renderContext.m_commandList->ExecuteIndirect( writer.m_layout.m_shaderProgram.TrinityALImpl_GetObject()->m_drawIndexedInstanced, writer.m_commandCount, writer.m_allocation.buffer, offset, nullptr, 0 );
		CCP_STATS_INC( sceneExecuteIndirectCount );
		CCP_STATS_ADD( sceneIndirectDrawCount, writer.m_commandCount );
	}
#elif TRINITY_PLATFORM == TRINITY_METAL

	auto encoder = writer.m_renderContext.GetMetalWorkQueue()->GetRenderEncoder();
	if( writer.m_renderContext.GetMetalWorkQueue()->EmitRenderEncoderState() )
	{
		auto& allocator = writer.m_renderContext.GetMetalContext()->GetConstantBufferAllocator();
		auto& layout = writer.m_layout;
		auto perObjectVS = Tr2Renderer::GetPerObjectVSStartRegister();
		auto perObjectPS = Tr2Renderer::GetPerObjectPSStartRegister();
		auto materialPageVS = 0xffffffff;
		auto materialPagePS = 0xffffffff;
		auto perObjectPageVS = 0xffffffff;
		auto perObjectPagePS = 0xffffffff;

		for( uint32_t i = 0; i < writer.m_commandCount; ++i )
		{
			auto& dp = writer.m_allocation.address[i];
			if( layout.m_hasMaterialData[0] )
			{
				auto page = uint32_t( dp.material[0] >> 32 );
				auto offset = uint32_t( dp.material[0] & 0xffffffff );
				if( page != materialPageVS )
				{
					[encoder setVertexBuffer:allocator.GetPage( page ) offset:offset atIndex:METAL_CONST_BUFFER_OFFSET];
					materialPageVS = page;
				}
				else
				{
					[encoder setVertexBufferOffset:offset atIndex:METAL_CONST_BUFFER_OFFSET];
				}
			}
			if( layout.m_hasPerObjectData[0] )
			{
				auto page = uint32_t( dp.perObject[0] >> 32 );
				auto offset = uint32_t( dp.perObject[0] & 0xffffffff );
				if( page != perObjectPageVS )
				{
					[encoder setVertexBuffer:allocator.GetPage( page ) offset:offset atIndex:METAL_CONST_BUFFER_OFFSET + perObjectVS];
					perObjectPageVS = page;
				}
				else
				{
					[encoder setVertexBufferOffset:offset atIndex:METAL_CONST_BUFFER_OFFSET + perObjectVS];
				}
			}
			if( layout.m_hasMaterialData[1] )
			{
				auto page = uint32_t( dp.material[1] >> 32 );
				auto offset = uint32_t( dp.material[1] & 0xffffffff );
				if( page != materialPagePS )
				{
					[encoder setFragmentBuffer:allocator.GetPage( page ) offset:offset atIndex:METAL_CONST_BUFFER_OFFSET];
					materialPagePS = page;
				}
				else
				{
					[encoder setFragmentBufferOffset:offset atIndex:METAL_CONST_BUFFER_OFFSET];
				}
			}
			if( layout.m_hasPerObjectData[1] )
			{
				auto page = uint32_t( dp.perObject[1] >> 32 );
				auto offset = uint32_t( dp.perObject[1] & 0xffffffff );
				if( page != perObjectPagePS )
				{
					[encoder setFragmentBuffer:allocator.GetPage( page ) offset:offset atIndex:METAL_CONST_BUFFER_OFFSET + perObjectPS];
					perObjectPagePS = page;
				}
				else
				{
					[encoder setFragmentBufferOffset:offset atIndex:METAL_CONST_BUFFER_OFFSET + perObjectPS];
				}
			}
			[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
								indexCount:dp.indexCountPerInstance
								 indexType:writer.m_renderContext.m_metalIndexType
							   indexBuffer:writer.m_renderContext.m_metalIndexBuffer
						 indexBufferOffset:dp.startIndexLocation * ( writer.m_renderContext.m_metalIndexType == MTLIndexTypeUInt16 ? 2 : 4 )
							 instanceCount:dp.instanceCount
								baseVertex:dp.baseVertexLocation
							  baseInstance:dp.startInstanceLocation];
		}
		writer.m_renderContext.GetMetalWorkQueue()->MarkConstantBuffersDirty();
	}
	writer.m_renderContext.GetMetalWorkQueue()->ReleaseEncoder( false );
#endif
}

void Tr2IndirectDrawBuffer::ReleaseResources( TriStorage s )
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	if( ( s & TRISTORAGE_MANAGEDMEMORY ) != 0 )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		if( m_uploadBuffer )
		{
			renderContext.ReleaseLater( m_uploadBuffer );
			m_uploadBuffer = nullptr;
		}
		if( m_defaultBuffer )
		{
			renderContext.ReleaseLater( m_defaultBuffer );
			m_defaultBuffer = nullptr;
		}
		m_cpuAddr = nullptr;
		m_size = 0;
		m_head = 0;
		m_tail = -1;
		m_frame = 0;
		m_regions.clear();
	}
#endif
}

bool Tr2IndirectDrawBuffer::OnPrepareResources()
{
	return true;
}


#if TRINITY_PLATFORM == TRINITY_DIRECTX12

void Tr2IndirectDrawBuffer::Resize( uint32_t newSize )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( m_uploadBuffer )
	{
		//Make sure we copy all pending arguments before we release them!
		CopyArguments();

		renderContext.ReleaseLater( m_uploadBuffer );
		m_uploadBuffer = nullptr;
		renderContext.ReleaseLater( m_defaultBuffer );
		m_defaultBuffer = nullptr;
	}


	CComPtr<ID3D12Resource> uploadBuffer;
	CComPtr<ID3D12Resource> defaultBuffer;

	{
		D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
		D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, UINT64( newSize ), 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

		HRESULT result = renderContext.m_device->CreateCommittedResource( &heap, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS( &uploadBuffer ) );
		if( FAILED( result ) )
		{
			CCP_LOGERR( "Failed to create indirect draw upload buffer (hr=0x%x)", result );
			return;
		}

		D3D12_RANGE range = { 0, 0 };
		uploadBuffer->Map( 0, &range, (void**)&m_cpuAddr );
	}

	{
		D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
		D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, UINT64( newSize ), 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

		HRESULT result = renderContext.m_device->CreateCommittedResource( &heap, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, nullptr, IID_PPV_ARGS( &defaultBuffer ) );
		if( FAILED( result ) )
		{
			CCP_LOGERR( "Failed to create indirect draw buffer (hr=0x%x)", result );
			return;
		}

		if( renderContext.m_device1 )
		{
			//Don't need a super high priority on this.
			ID3D12Pageable* resource = defaultBuffer;
			D3D12_RESIDENCY_PRIORITY priority = D3D12_RESIDENCY_PRIORITY_NORMAL;

			renderContext.m_device1->SetResidencyPriority( 1, &resource, &priority );
		}
	}

	m_uploadBuffer = uploadBuffer;
	m_defaultBuffer = defaultBuffer;

	m_size = newSize;


	m_head = 0;
	m_tail = 0;
	m_regions.clear();
}

#endif
