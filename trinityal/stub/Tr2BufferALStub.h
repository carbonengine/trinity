// Copyright © 2023 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_STUB

#include "../include/Tr2BufferAL.h"

namespace TrinityALImpl
{
class Tr2BufferAL : public Tr2DeviceResourceAL<Tr2BufferAL>
{
public:
	ALResult Create(
		const Tr2BufferDescriptionAL& desc,
		const void* initialData,
		Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;
	Tr2ALMemoryType GetMemoryClass() const;
	const Tr2BufferDescriptionAL& GetDesc() const;

	ALResult MapForReading( const void*& data, Tr2RenderContextAL& renderContext );
	ALResult MapForReading( const void*& data, uint32_t offset, uint32_t size, Tr2RenderContextAL& renderContext );
	void UnmapForReading( Tr2RenderContextAL& renderContext );
	ALResult MapForWriting( void*& data, Tr2RenderContextAL& renderContext );
	void UnmapForWriting( Tr2RenderContextAL& renderContext );

	ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL& renderContext );
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

	uint32_t GetSrvIndexInHeap() const;
	uint32_t GetUavIndexInHeap() const;

private:
	CcpMallocBuffer m_buffer;
	Tr2BufferDescriptionAL m_desc;

	friend class Tr2RenderContextAL;
	friend class Tr2PrimaryRenderContextAL;
};
}

#endif