#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2PipelineStatsQueryAL.h"

namespace TrinityALImpl
{
class Tr2PipelineStatsDataAL
{
};


class Tr2PipelineStatsQueryAL : public Tr2DeviceResourceAL<Tr2PipelineStatsQueryAL>
{
public:
	Tr2PipelineStatsQueryAL();

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;
	void Destroy();

	ALResult Begin( Tr2RenderContextAL& renderContext );
	ALResult End( Tr2RenderContextAL& renderContext );

	ALResult GetStats( Tr2PipelineStatsDataAL& data, Tr2RenderContextAL& renderContext );

	static size_t GetValueCount( const Tr2PipelineStatsDataAL& data );
	static const char* GetLabel( const Tr2PipelineStatsDataAL& data, size_t index );
	static const char* GetDescription( const Tr2PipelineStatsDataAL& data, size_t index );
	static ::Tr2PipelineStatsQueryAL::Value GetValue( const Tr2PipelineStatsDataAL& data, size_t index );

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

private:
	Tr2PipelineStatsQueryAL( const Tr2PipelineStatsQueryAL& ) = delete;
	Tr2PipelineStatsQueryAL& operator=( const Tr2PipelineStatsQueryAL& ) = delete;
};

}

#endif