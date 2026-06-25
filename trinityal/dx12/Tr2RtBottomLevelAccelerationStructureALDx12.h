// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2RtBottomLevelAccelerationStructureAL.h"

namespace TrinityALImpl
{
class Tr2RtBottomLevelAccelerationStructureAL : public Tr2DeviceResourceAL<Tr2RtBottomLevelAccelerationStructureAL>
{
public:
	Tr2RtBottomLevelAccelerationStructureAL();
	~Tr2RtBottomLevelAccelerationStructureAL();

	ALResult Create( const Tr2RtGeometryAL& geometry, const Tr2RtGeometryAL& capacity, Tr2RtBlasGeometryFlags::Type geometryFlags, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext );
	ALResult Update( const Tr2RtGeometryAL& geometry, Tr2RenderContextAL& renderContext );
	ALResult CompactBlas( Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;

	void Destroy();
	Tr2ALMemoryType GetMemoryClass() const;
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

	ID3D12Resource* GetBuffer() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

private:
	CComPtr<ID3D12Resource> m_buffer;
	CComPtr<ID3D12Resource> m_scratch;
	D3D12_GPU_VIRTUAL_ADDRESS m_bufferAddress;
	D3D12_RAYTRACING_GEOMETRY_DESC m_geometryDesc;
	Tr2RtBlasGeometryFlags::Type m_geometryFlags;
	Tr2RtBuildFlags::Type m_buildFlags;
	Tr2PrimaryRenderContextAL* m_owner;
};
}

#endif
