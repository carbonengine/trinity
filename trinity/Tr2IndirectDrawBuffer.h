// Copyright © 2023 CCP ehf.

#pragma once

#include "Tr2DeviceResource.h"


struct Tr2IndirectDrawBufferLayout
{
	Tr2IndirectDrawBufferLayout() = default;
	Tr2IndirectDrawBufferLayout( const Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext );

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	Tr2ShaderProgramAL m_shaderProgram;
	uint32_t m_materialOffsets[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	uint32_t m_perObjectDataOffsets[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	uint32_t m_drawOffset = 0;
	uint32_t m_stride = 0;
#elif TRINITY_PLATFORM == TRINITY_METAL
	bool m_hasMaterialData[2];
	bool m_hasPerObjectData[2];
#endif
};

class Tr2IndirectDrawBuffer;
class Tr2IndirectDrawBufferWriter;


class Tr2IndirectDrawBuffer : public Tr2DeviceResource
{
public:
	Tr2IndirectDrawBuffer() = default;
	Tr2IndirectDrawBuffer( const Tr2IndirectDrawBuffer& ) = delete;
	Tr2IndirectDrawBuffer& operator=( const Tr2IndirectDrawBuffer& ) = delete;
	~Tr2IndirectDrawBuffer();

	bool IsValid() const;
	void Create( uint32_t size );

#if TRINITY_PLATFORM == TRINITY_METAL
	struct DP
	{
		uint64_t material[2];
		uint64_t perObject[2];
		uint32_t indexCountPerInstance;
		uint32_t instanceCount;
		uint32_t startIndexLocation;
		uint32_t baseVertexLocation;
		uint32_t startInstanceLocation;
	};
#endif

	struct Allocation
	{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
		CComPtr<ID3D12Resource> buffer;
		void* baseAddress;
		void* address;
#elif TRINITY_PLATFORM == TRINITY_METAL
		DP* address;
#endif
	};
	Allocation Allocate( uint32_t size );
	void CopyArguments();
	void SetFrameNumbers( uint64_t recordingFrame, uint64_t completedFrame );
	void Submit( const Tr2IndirectDrawBufferWriter& writer );

	void ReleaseResources( TriStorage s ) override;

protected:
	bool OnPrepareResources() override;

private:
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	void Resize( uint32_t newSize );

	CComPtr<ID3D12Resource> m_uploadBuffer;
	CComPtr<ID3D12Resource> m_defaultBuffer;
	uint8_t* m_cpuAddr = nullptr;
	int32_t m_size = 0;
	int32_t m_head = 0;
	int32_t m_tail = -1;
	uint64_t m_frame = 0;

	struct Region
	{
		uint64_t frame;
		int32_t start;
		int32_t end;
		bool copied;
	};

	std::vector<Region> m_regions;
#elif TRINITY_PLATFORM == TRINITY_METAL
	std::vector<std::unique_ptr<DP[]>> m_pages;
	uint32_t m_page = 0;
	uint32_t m_pageOffset = 0;
	uint32_t m_pageSize = 1024;
#endif
};

class Tr2IndirectDrawBufferWriter
{
public:
	Tr2IndirectDrawBufferWriter( Tr2IndirectDrawBuffer& buffer, const Tr2IndirectDrawBufferLayout& layout, uint32_t commands, Tr2RenderContextAL& renderContext );

	void SetMaterialConstants( Tr2RenderContextEnum::ShaderType stage, const Tr2ConstantBufferAL& buffer );
	void SetPerObjectData( Tr2RenderContextEnum::ShaderType stage, const Tr2ConstantBufferAL& buffer );
	void SetPerObjectData( Tr2RenderContextEnum::ShaderType stage, const void* data, size_t size );

	void DrawIndexed( uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation );

	void Next();

	bool HasMaterialConstants( Tr2RenderContextEnum::ShaderType stage ) const;
	bool HasPerObjectData( Tr2RenderContextEnum::ShaderType stage ) const;

private:
	const Tr2IndirectDrawBufferLayout& m_layout;
	Tr2RenderContextAL& m_renderContext;
	uint32_t m_commandCount;
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	Tr2IndirectDrawBuffer::Allocation m_allocation;
	uint8_t* m_buffer;
#elif TRINITY_PLATFORM == TRINITY_METAL
	Tr2IndirectDrawBuffer::Allocation m_allocation;
	Tr2IndirectDrawBuffer::DP* m_buffer;
#endif
	friend class Tr2IndirectDrawBuffer;
};
