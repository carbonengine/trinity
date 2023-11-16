////////////////////////////////////////////////////////////
//
//    Created:   August 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../../ALResult.h"

namespace TrinityALImpl
{

	class GpuMarkerBuffer
	{
	public:
		GpuMarkerBuffer();
		~GpuMarkerBuffer();

		ALResult Create( ID3D12Device* device );
		void Destroy();

		void PutMarker( ID3D12GraphicsCommandList2* commandList, const char* marker ) const;
		ALResult GetMarker( std::string& marker ) const;
	private:
		CComPtr<ID3D12Resource> m_buffer;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress;
		const void* m_cpuAddress;
	};
}

#endif