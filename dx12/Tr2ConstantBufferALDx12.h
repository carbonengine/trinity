////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../Tr2TrackedALObject.h"
#include "../Tr2RenderContextEnum.h"
#include "../ALResult.h"

#include "Tr2ResourceHelper.h"


class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;


// -------------------------------------------------------------
// Description:
//   A low level wrapper around the calls needed to set up a constant
//   buffer for a given platform. Any higher level logic should be
//   handled one level up, this is only to avoid ifdef soup when
//   creating and locking buffers.
//	 32bit: no support for 64bit-sized buffers.
// -------------------------------------------------------------
class Tr2ConstantBufferAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_CONSTANT_BUFFER>
{
public:
	Tr2ConstantBufferAL();
	~Tr2ConstantBufferAL();

	ALResult Create( uint32_t size, Tr2PrimaryRenderContextAL & renderContext );
	ALResult Create( uint32_t size, Tr2RenderContextEnum::BufferUsage usage, const void* initialData, Tr2PrimaryRenderContextAL & renderContext );
	void Destroy();

	ALResult Lock( void** data, Tr2RenderContextAL & renderContext );
	ALResult Unlock( Tr2RenderContextAL & renderContext );

	bool IsValid() const;

	uint32_t GetSize() const;
	Tr2ALMemoryType GetMemoryClass() const;

	void* GetBufferMirror( uint32_t minimumSize, Tr2RenderContextAL & renderContext );
	void* GetBufferMirror( Tr2RenderContextAL & renderContext );
	ALResult UpdateFromMirror( Tr2RenderContextAL & renderContext );

	bool operator==( const Tr2ConstantBufferAL& other ) const;
private:
	Tr2ConstantBufferAL( const Tr2ConstantBufferAL& ) /* = delete */;
	Tr2ConstantBufferAL& operator=( const Tr2ConstantBufferAL& ) /* = delete */;

	TrinityALImpl::Tr2ResourceHelper m_buffer;

	CcpMallocBuffer m_mirror;
	Tr2PrimaryRenderContextAL* m_owner;
	uint32_t m_size;

	friend class Tr2RenderContextAL;
};

#endif
