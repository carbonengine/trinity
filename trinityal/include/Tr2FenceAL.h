// Copyright © 2023 CCP ehf.

#pragma once

#include "../Tr2DeviceResourceAL.h"
#include "../ALResult.h"

class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;

namespace TrinityALImpl
{
class Tr2FenceAL;
}

class Tr2FenceAL
{
public:
	Tr2FenceAL();

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext );

	ALResult PutFence( Tr2RenderContextAL& renderContext );
	ALResult IsReached( bool& isReached, Tr2RenderContextAL& renderContext );
	ALResult Wait( Tr2RenderContextAL& renderContext );

	bool IsValid() const;
	bool operator==( const Tr2FenceAL& other ) const;
	Tr2ALMemoryType GetMemoryClass() const;

	ALResult SetName( const char* name );

private:
	std::shared_ptr<TrinityALImpl::Tr2FenceAL> m_fence;
};
