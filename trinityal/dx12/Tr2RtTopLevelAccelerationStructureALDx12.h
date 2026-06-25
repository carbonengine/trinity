// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2RtTopLevelAccelerationStructureAL.h"

namespace TrinityALImpl
{
class Tr2RtTopLevelAccelerationStructureAL : public Tr2DeviceResourceAL<Tr2RtTopLevelAccelerationStructureAL>
{
public:
	Tr2RtTopLevelAccelerationStructureAL();
	~Tr2RtTopLevelAccelerationStructureAL();

	ALResult Create( const size_t count, const Tr2RtInstanceAL* instances, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext );
	ALResult Update( const size_t count, const Tr2RtInstanceAL* instances, Tr2RenderContextAL& renderContext );

	bool IsValid() const;

	void Destroy();
	Tr2ALMemoryType GetMemoryClass() const;
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

	const ::Tr2BufferAL& GetBuffer() const;
	size_t GetCapacity() const;

private:
	struct UploadBuffer
	{
		CComPtr<ID3D12Resource> uploadBuffer;
		uint64_t frameIndex;
	};

	::Tr2BufferAL m_buffer;
	std::vector<UploadBuffer> m_uploadBuffers;
	CComPtr<ID3D12Resource> m_scratch;
	size_t m_capacity;
	Tr2RtBuildFlags::Type m_buildFlags;
	Tr2PrimaryRenderContextAL* m_owner;
};
}

#endif
