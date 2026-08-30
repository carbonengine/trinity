// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12


#include "../include/Tr2RtShaderTableAL.h"
#include "../include/Tr2RtPipelineStateAL.h"

namespace TrinityALImpl
{
class Tr2RtShaderTableAL : public Tr2DeviceResourceAL<Tr2RtShaderTableAL>
{
public:
	Tr2RtShaderTableAL();
	~Tr2RtShaderTableAL();

	ALResult Create( const Tr2RtShaderTableDescriptionAL& desc, const ::Tr2RtPipelineStateAL& pipeline, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();
	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

	D3D12_GPU_VIRTUAL_ADDRESS GetRayGenShader( const wchar_t* name ) const;
	D3D12_GPU_VIRTUAL_ADDRESS GetMissShaders() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetHitGroupShaders() const;
	uint64_t GetEntrySize() const;
	uint64_t GetMissShaderTableSize() const;
	uint64_t GetHitGroupTableSize() const;

private:
	struct TableBuffer
	{
		CComPtr<ID3D12Resource> resource;
		uint8_t* mapped = nullptr;
		size_t capacity = 0;
		// retained hit records already written to this buffer, valid for this pipeline / description version / entry size
		uint32_t retainedWritten = 0;
		uint32_t retainedVersion = 0xffffffff;
		ID3D12StateObject* pipeline = nullptr;
		uint64_t entrySize = 0;
	};
	void ReleaseBuffers();

	Tr2RtShaderTableDescriptionAL m_desc;
	CComPtr<ID3D12Resource> m_table;
	std::vector<TableBuffer> m_buffers;
	std::vector<const wchar_t*> m_rayGenNames;
	size_t m_missCount = 0;
	size_t m_hitGroupCount = 0;
	Tr2PrimaryRenderContextAL* m_owner;
	uint64_t m_entrySize;
};
}

#endif