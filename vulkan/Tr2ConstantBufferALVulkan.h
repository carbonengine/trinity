////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once


#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../Tr2TrackedALObject.h"
#include "../Tr2RenderContextEnum.h"
#include "../ALResult.h"

#ifdef TRINITY_AL_GUARD_LOCKS
#include "../Tr2LockGuard.h"
#endif


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
	Tr2ConstantBufferAL()
	{

	}
	~Tr2ConstantBufferAL()
	{

	}

	Tr2ConstantBufferAL( Tr2ConstantBufferAL&& )
	{

	}
	Tr2ConstantBufferAL& operator=( Tr2ConstantBufferAL&& )
	{

	}

	ALResult Create( uint32_t size, Tr2PrimaryRenderContextAL & renderContext )
	{
		return Create( size, Tr2RenderContextEnum::USAGE_CPU_WRITE, nullptr, renderContext );
	}

	ALResult Create( uint32_t size, Tr2RenderContextEnum::BufferUsage usage, const void* initialData, Tr2PrimaryRenderContextAL & renderContext )
	{
		return E_NOTIMPL;
	}
	ALResult Lock( void** data, Tr2RenderContextAL & renderContext )
	{
		return E_NOTIMPL;
	}
	ALResult Unlock( Tr2RenderContextAL & renderContext )
	{
		return E_NOTIMPL;
	}
	bool IsValid() const 
	{ 
		return false;
	}
	void Destroy()
	{

	}

	Tr2RenderContextEnum::BufferUsage GetUsage() const
	{
		return 0;
	}

	uint32_t GetSize() const { return 0; }

	void* GetBufferMirror( uint32_t minimumSize, Tr2RenderContextAL & renderContext )
	{
		return nullptr;
	}
	void* GetBufferMirror( Tr2RenderContextAL & renderContext ) 
	{ 
		return GetBufferMirror( 0, renderContext ); 
	}
	ALResult UpdateFromMirror( Tr2RenderContextAL & renderContext )
	{
		return E_NOTIMPL;
	}

	bool operator==( const Tr2ConstantBufferAL& other ) const 
	{ 
		return this == &other; 
	}

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

private:
	Tr2ConstantBufferAL( const Tr2ConstantBufferAL& ) /* = delete */;
	Tr2ConstantBufferAL& operator=( const Tr2ConstantBufferAL& ) /* = delete */;
};

#endif
