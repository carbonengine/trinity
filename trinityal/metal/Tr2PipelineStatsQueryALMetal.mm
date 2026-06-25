// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL
#include "Tr2PipelineStatsQueryALMetal.h"
#include "Tr2RenderContextMetal.h"


extern bool g_enableMetalCounters;


namespace
{


const uint32_t MAX_SAMPLES_PER_QUERY = 128;

static_assert( 2 * 64 >= MAX_SAMPLES_PER_QUERY, "too many samples" );

struct Field
{
	const char* label;
	ptrdiff_t offset;
	const char* description;
};

static const Field s_fields[] = {
	{ "tessellationInputPatches",
	  offsetof( Tr2PipelineStatsDataAL::Stats, tessellationInputPatches ),
	  "The number of tessellation patches passed into the tessellation stage" },
	{ "vertexInvocations",
	  offsetof( Tr2PipelineStatsDataAL::Stats, vertexInvocations ),
	  "The number of times vertex shaders were called" },
	{ "postTessellationVertexInvocations",
	  offsetof( Tr2PipelineStatsDataAL::Stats, postTessellationVertexInvocations ),
	  "The number of vertices created by the tessellation stage" },
	{ "clipperInvocations",
	  offsetof( Tr2PipelineStatsDataAL::Stats, clipperInvocations ),
	  "The number of primitives sent to the clip stage" },
	{ "clipperPrimitivesOut",
	  offsetof( Tr2PipelineStatsDataAL::Stats, clipperPrimitivesOut ),
	  "The number of primitives emitted by the clip stage" },
	{ "fragmentInvocations",
	  offsetof( Tr2PipelineStatsDataAL::Stats, fragmentInvocations ),
	  "The number of times fragment shaders were called" },
	{ "fragmentsPassed",
	  offsetof( Tr2PipelineStatsDataAL::Stats, fragmentsPassed ),
	  "The number of fragments passed to the visibility and blend stages" },
};

}

namespace TrinityALImpl
{

Tr2PipelineStatsQueryAL::Tr2PipelineStatsQueryAL() : m_buffer( nil ), m_nextIndex( 0 ), m_state( READY )
{
	std::fill( std::begin( m_zeroSamples ), std::end( m_zeroSamples ), 0u );
}

Tr2PipelineStatsQueryAL::~Tr2PipelineStatsQueryAL()
{
	Destroy();
}

ALResult Tr2PipelineStatsQueryAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	if( !g_enableMetalCounters )
	{
		return E_FAIL;
	}

	Destroy();
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}

	if( @available( macOS 10.15, * ) )
	{
		MetalContext* metalContext = renderContext.GetMetalContext();
		id<MTLDevice> mtlDevice = metalContext->GetDevice();

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 110000
		if( @available( macOS 11.0, * ) )
		{
			if( ![mtlDevice supportsCounterSampling:MTLCounterSamplingPointAtDispatchBoundary] )
			{
				return E_FAIL;
			}
		}
#endif

		MTLCounterSampleBufferDescriptor* descriptor = [MTLCounterSampleBufferDescriptor new];
		descriptor.label = @"Pipeline stats query";
		descriptor.sampleCount = MAX_SAMPLES_PER_QUERY;
		descriptor.storageMode = MTLStorageModeShared;
		for( id<MTLCounterSet> counterSet in mtlDevice.counterSets )
		{
			if( [counterSet.name isEqualToString:MTLCommonCounterSetStatistic] )
			{
				descriptor.counterSet = counterSet;
				break;
			}
		}
		if( !descriptor.counterSet )
		{
			return E_FAIL;
		}

		m_buffer = [mtlDevice newCounterSampleBufferWithDescriptor:descriptor error:nil];
	}
	else
	{
		return E_FAIL;
	}
	return S_OK;
}

bool Tr2PipelineStatsQueryAL::IsValid() const
{
	return m_buffer != nil;
}

void Tr2PipelineStatsQueryAL::Destroy()
{
#if !__has_feature( objc_arc )
	if( m_buffer )
	{
		[m_buffer release];
	}
#endif
	m_buffer = nil;
	m_nextIndex = 0;
	m_state = READY;
	std::fill( std::begin( m_zeroSamples ), std::end( m_zeroSamples ), 0u );
}

ALResult Tr2PipelineStatsQueryAL::Begin( Tr2RenderContextAL& renderContext )
{
	if( @available( macOS 10.15, * ) )
	{
		if( !IsValid() || !renderContext.IsValid() || m_state != READY )
		{
			return E_INVALIDARG;
		}
		if( !renderContext.GetMetalWorkQueue()->SampleCounter( m_buffer, 0, MetalWorkQueue::COUNTER_PIPELINE_STATS ) )
		{
			m_zeroSamples[0] = 1;
		}
		renderContext.GetMetalWorkQueue()->PipelineQueryStarted( this );
		m_state = BEGIN_ISSUED;
		m_nextIndex = 1;
		return S_OK;
	}
	else
	{
		return E_INVALIDARG;
	}
}

ALResult Tr2PipelineStatsQueryAL::End( Tr2RenderContextAL& renderContext )
{
	if( @available( macOS 10.15, * ) )
	{
		if( !IsValid() || !renderContext.IsValid() || m_state != BEGIN_ISSUED )
		{
			return E_INVALIDARG;
		}
		if( m_nextIndex < MAX_SAMPLES_PER_QUERY )
		{
			if( !renderContext.GetMetalWorkQueue()->SampleCounter(
					m_buffer, m_nextIndex, MetalWorkQueue::COUNTER_PIPELINE_STATS ) )
			{
				m_zeroSamples[m_nextIndex / 64] |= 1 << ( m_nextIndex % 64 );
			}
			++m_nextIndex;
			m_state = END_ISSUED;
			renderContext.GetMetalWorkQueue()->PipelineQueryEnded( this );
			return S_OK;
		}
		else
		{
			renderContext.GetMetalWorkQueue()->PipelineQueryEnded( this );
			return E_FAIL;
		}
	}
	else
	{
		return E_INVALIDARG;
	}
}

void Tr2PipelineStatsQueryAL::EncoderStarted( MetalWorkQueue* queue )
{
	if( @available( macOS 10.15, * ) )
	{
		if( m_buffer && m_nextIndex < MAX_SAMPLES_PER_QUERY )
		{
			if( !queue->SampleCounter( m_buffer, m_nextIndex, MetalWorkQueue::COUNTER_PIPELINE_STATS ) )
			{
				m_zeroSamples[m_nextIndex / 64] |= 1 << ( m_nextIndex % 64 );
			}
			++m_nextIndex;
		}
	}
}

void Tr2PipelineStatsQueryAL::EncoderEnding( MetalWorkQueue* queue )
{
	if( @available( macOS 10.15, * ) )
	{
		if( m_buffer && m_nextIndex < MAX_SAMPLES_PER_QUERY )
		{
			if( !queue->SampleCounter( m_buffer, m_nextIndex, MetalWorkQueue::COUNTER_PIPELINE_STATS ) )
			{
				m_zeroSamples[m_nextIndex / 64] |= 1 << ( m_nextIndex % 64 );
			}
			++m_nextIndex;
		}
	}
}

ALResult Tr2PipelineStatsQueryAL::GetStats( Tr2PipelineStatsDataAL& results, Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDARG;
	}
	if( @available( macOS 10.15, * ) )
	{
		if( !IsValid() || m_state != END_ISSUED )
		{
			return S_FALSE;
		}
		auto data = [m_buffer resolveCounterRange:NSMakeRange( 0, m_nextIndex )];
		if( !data )
		{
			return S_FALSE;
		}
		results.data = {};
		auto samples = static_cast<const Tr2PipelineStatsDataAL::Stats*>( data.bytes );
		Tr2PipelineStatsDataAL::Stats zeroes = {};
		for( NSUInteger i = 0; i + 1 < m_nextIndex; i += 2 )
		{
			auto& d0 = ( m_zeroSamples[i / 64] & ( 1 << ( i % 64 ) ) ) != 0 ? zeroes : samples[i];
			auto& d1 = ( m_zeroSamples[( i + 1 ) / 64] & ( 1 << ( ( i + 1 ) % 64 ) ) ) != 0 ? zeroes : samples[i + 1];
#define DIFF_DATA( x ) results.data.x += d0.x <= d1.x ? d1.x - d0.x : 0;
			DIFF_DATA( tessellationInputPatches );
			DIFF_DATA( vertexInvocations );
			DIFF_DATA( postTessellationVertexInvocations );
			DIFF_DATA( clipperInvocations );
			DIFF_DATA( clipperPrimitivesOut );
			DIFF_DATA( fragmentInvocations );
			DIFF_DATA( fragmentsPassed );
			DIFF_DATA( computeKernelInvocations );
#undef DIFF_DATA
		}
		m_state = READY;
		m_nextIndex = 0;
		return S_OK;
	}
	else
	{
		return -1;
	}
	return S_OK;
}

size_t Tr2PipelineStatsQueryAL::GetValueCount( const Tr2PipelineStatsDataAL& data )
{
	return sizeof( s_fields ) / sizeof( s_fields[0] );
}

const char* Tr2PipelineStatsQueryAL::GetLabel( const Tr2PipelineStatsDataAL& data, size_t index )
{
	return s_fields[index].label;
}

const char* Tr2PipelineStatsQueryAL::GetDescription( const Tr2PipelineStatsDataAL& data, size_t index )
{
	return s_fields[index].description;
}

::Tr2PipelineStatsQueryAL::Value Tr2PipelineStatsQueryAL::GetValue( const Tr2PipelineStatsDataAL& data, size_t index )
{
	return *reinterpret_cast<const ::Tr2PipelineStatsQueryAL::Value*>( reinterpret_cast<const uint8_t*>( &data.data ) +
																	   s_fields[index].offset );
}

void Tr2PipelineStatsQueryAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2PipelineStatsQueryAL";
	description["name"] = m_name;
}

ALResult Tr2PipelineStatsQueryAL::SetName( const char* name )
{
	m_name = name;
	return S_OK;
}

}

#endif
