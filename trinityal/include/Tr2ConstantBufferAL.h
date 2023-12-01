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
	};
}

class Tr2ConstantBufferAL
{
public:
	Tr2ConstantBufferAL();


	ALResult Create( uint32_t size, Tr2PrimaryRenderContextAL & renderContext );
	ALResult Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2PrimaryRenderContextAL & renderContext );

	ALResult Lock( void** data, Tr2RenderContextAL & renderContext );
	ALResult Unlock( Tr2RenderContextAL & renderContext );

	bool IsValid() const;
	uint32_t GetSize() const;
	Tr2ALMemoryType GetMemoryClass() const;

	ALResult SetName( const char* name );

	bool operator==( const Tr2ConstantBufferAL& other ) const;

private:
	std::shared_ptr<TrinityALImpl::Tr2ConstantBufferAL> m_buffer;
	friend class Tr2RenderContextAL;
};
