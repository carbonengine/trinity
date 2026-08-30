// Copyright © 2023 CCP ehf.

#pragma once

#include "../Tr2DeviceResourceAL.h"
#include "../ALResult.h"

class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;

namespace TrinityALImpl
{
class Tr2ConstantBufferAL;
}

namespace Tr2ConstantUsageAL
{
enum Type
{
	IMMUTABLE,
	ONE_SHOT,
	REUSABLE,
	PERSISTENT, // rarely changes: lives in a GPU-resident slot, only re-uploaded when written
};
}

class Tr2ConstantBufferAL
{
public:
	Tr2ConstantBufferAL();


	ALResult Create( uint32_t size, Tr2PrimaryRenderContextAL& renderContext );
	ALResult Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2PrimaryRenderContextAL& renderContext );

	ALResult Lock( void** data, Tr2RenderContextAL& renderContext );
	ALResult Unlock( Tr2RenderContextAL& renderContext );

	bool IsValid() const;
	uint32_t GetSize() const;
	Tr2ALMemoryType GetMemoryClass() const;
	// GPU address of a PERSISTENT buffer's slot while its contents are resident and unchanged, 0 otherwise
	uint64_t GetStableAddress() const;
	static bool SupportsPersistent();

	ALResult SetName( const char* name );

	bool operator==( const Tr2ConstantBufferAL& other ) const;

	TrinityALImpl::Tr2ConstantBufferAL* TrinityALImpl_GetObject() const;

private:
	std::shared_ptr<TrinityALImpl::Tr2ConstantBufferAL> m_buffer;
	friend class Tr2RenderContextAL;
};
