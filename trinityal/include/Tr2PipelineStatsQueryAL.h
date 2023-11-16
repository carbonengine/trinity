#pragma once

#include "../Tr2DeviceResourceAL.h"
#include "../ALResult.h"

class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;

namespace TrinityALImpl
{
class Tr2PipelineStatsQueryAL;
class Tr2PipelineStatsDataAL;
}

typedef TrinityALImpl::Tr2PipelineStatsDataAL Tr2PipelineStatsDataAL;

class Tr2PipelineStatsQueryAL
{
public:
	Tr2PipelineStatsQueryAL();

	ALResult Create( Tr2PrimaryRenderContextAL& );

	ALResult Begin( Tr2RenderContextAL& renderContext );
	ALResult End( Tr2RenderContextAL& renderContext );

	ALResult GetStats( Tr2PipelineStatsDataAL& data, Tr2RenderContextAL& renderContext );

	bool IsValid() const;
	bool operator==( const Tr2PipelineStatsQueryAL& other ) const;
	Tr2ALMemoryType GetMemoryClass() const;

	typedef uint64_t Value;

	static size_t GetValueCount( const Tr2PipelineStatsDataAL& data );
	static const char* GetLabel( const Tr2PipelineStatsDataAL& data, size_t index );
	static const char* GetDescription( const Tr2PipelineStatsDataAL& data, size_t index );
	static Value GetValue( const Tr2PipelineStatsDataAL& data, size_t index );

	ALResult SetName( const char* name );

private:
	std::shared_ptr<TrinityALImpl::Tr2PipelineStatsQueryAL> m_query;
};

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2PipelineStatsQueryAL )
