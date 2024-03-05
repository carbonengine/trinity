#pragma once

#include "../Tr2DeviceResourceAL.h"
#include "../ALResult.h"

class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;

namespace TrinityALImpl
{
	class Tr2OcclusionQueryAL;
}


class Tr2OcclusionQueryAL
{
public:
	enum WaitMode
	{
		WAIT,
		DO_NOT_WAIT,
	};

	Tr2OcclusionQueryAL();

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext );

	ALResult Begin( Tr2RenderContextAL& renderContext );
	ALResult End( Tr2RenderContextAL& renderContext );
	ALResult GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, WaitMode waitMode = DO_NOT_WAIT );

	bool IsValid() const;
	bool operator==( const Tr2OcclusionQueryAL& other ) const;
	Tr2ALMemoryType GetMemoryClass() const;

	ALResult SetName( const char* name );

private:
	std::shared_ptr<TrinityALImpl::Tr2OcclusionQueryAL> m_query;
};
