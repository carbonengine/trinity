#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include "../include/Tr2PipelineStatsQueryAL.h"

namespace TrinityALImpl
{

class MetalWorkQueue;

class Tr2PipelineStatsDataAL
{
public:
    // This should be MTLCounterResultStatistic, but we are still compiling for 10.14...
    struct Stats
    {
        uint64_t tessellationInputPatches;
        uint64_t vertexInvocations;
        uint64_t postTessellationVertexInvocations;
        uint64_t clipperInvocations;
        uint64_t clipperPrimitivesOut;
        uint64_t fragmentInvocations;
        uint64_t fragmentsPassed;
        uint64_t computeKernelInvocations;
    } data = {};
};


class Tr2PipelineStatsQueryAL : public Tr2DeviceResourceAL<Tr2PipelineStatsQueryAL>
{
public:
	Tr2PipelineStatsQueryAL();
    ~Tr2PipelineStatsQueryAL();

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

    void EncoderStarted( MetalWorkQueue* queue );
    void EncoderEnding( MetalWorkQueue* queue );
private:
	Tr2PipelineStatsQueryAL( const Tr2PipelineStatsQueryAL& ) = delete;
	Tr2PipelineStatsQueryAL& operator=( const Tr2PipelineStatsQueryAL& ) = delete;

    id m_buffer;
    uint32_t m_nextIndex;
    uint64_t m_zeroSamples[2];
	std::string m_name;
	enum
    {
        READY,
        BEGIN_ISSUED,
        END_ISSUED,
    } m_state;
};

}

#endif
