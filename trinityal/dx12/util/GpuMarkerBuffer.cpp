// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "GpuMarkerBuffer.h"

namespace TrinityALImpl
{
GpuMarkerBuffer::GpuMarkerBuffer() :
	m_gpuAddress( 0 ),
	m_cpuAddress( nullptr )
{
}

GpuMarkerBuffer::~GpuMarkerBuffer()
{
	Destroy();
}

ALResult GpuMarkerBuffer::Create( ID3D12Device* device )
{
	Destroy();

	D3D12_FEATURE_DATA_D3D12_OPTIONS3 options;
	CR_RETURN_HR( device->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS3, &options, sizeof( options ) ) );
	if( ( options.WriteBufferImmediateSupportFlags & D3D12_COMMAND_LIST_SUPPORT_FLAG_DIRECT ) == 0 )
	{
		return E_FAIL;
	}

	D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_READBACK };

	D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER };
	resourceDesc.Width = 256;
	resourceDesc.Height = resourceDesc.DepthOrArraySize = resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

	CR_RETURN_HR( device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS( &m_buffer ) ) );
	m_buffer->SetName( L"WriteImmedateBuffer" );
	if( FAILED( m_buffer->Map( 0, nullptr, (void**)&m_cpuAddress ) ) )
	{
		m_buffer = nullptr;
		m_cpuAddress = nullptr;
		return E_FAIL;
	}
	m_gpuAddress = m_buffer->GetGPUVirtualAddress();
	return S_OK;
}

void GpuMarkerBuffer::Destroy()
{
	if( m_buffer )
	{
		D3D12_RANGE emptyRange = { 0, 0 };
		m_buffer->Unmap( 0, &emptyRange );
		m_buffer = nullptr;
	}
	m_gpuAddress = 0;
	m_cpuAddress = nullptr;
}

void GpuMarkerBuffer::PutMarker( ID3D12GraphicsCommandList2* commandList, const char* marker ) const
{
	if( commandList && m_buffer )
	{
		auto length = std::min( strlen( marker ) + 1, size_t( 256 ) );
		D3D12_WRITEBUFFERIMMEDIATE_PARAMETER params[256 / 4];
		D3D12_WRITEBUFFERIMMEDIATE_MODE modes[256 / 4];
		auto addr = m_gpuAddress;
		uint32_t count = 0;
		for( size_t i = 0; i < 256 / 4 && length > 0; ++i )
		{
			params[i].Value = 0;
			params[i].Dest = addr;
			addr += 4;
			auto l = std::min( length, sizeof( params[i].Value ) );
			memcpy( &params[i].Value, marker, l );
			marker += l;
			length -= l;
			modes[i] = D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT;
			++count;
		}
		commandList->WriteBufferImmediate( count, params, modes );
	}
}
ALResult GpuMarkerBuffer::GetMarker( std::string& marker ) const
{
	if( m_buffer )
	{
		marker = std::string( static_cast<const char*>( m_cpuAddress ) );
		return S_OK;
	}

	return E_FAIL;
}
}

#endif