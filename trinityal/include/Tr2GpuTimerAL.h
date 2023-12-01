#pragma once

#include "../ALResult.h"
#include "../Tr2DeviceResourceAL.h"

namespace TrinityALImpl
{
	class Tr2GpuTimerAL;
}

class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;

class Tr2GpuTimerAL
{
public:
	Tr2GpuTimerAL();

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext );

	bool Begin( Tr2RenderContextAL& renderContext ) const;

	void End( Tr2RenderContextAL& renderContext ) const;

	float GetTime( Tr2RenderContextAL& renderContext ) const;

	bool IsValid() const;

	bool operator==( const Tr2GpuTimerAL& other ) const;

	Tr2ALMemoryType GetMemoryClass() const;

	ALResult SetName( const char* name );

private:
	std::shared_ptr<TrinityALImpl::Tr2GpuTimerAL> m_gpu_timer;
};
