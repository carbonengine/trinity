// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if ( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2GpuTimerALMetal.h"
#include "Tr2PrimaryRenderContextMetal.h"
#include <mach/mach_time.h>


extern bool g_enableMetalCounters;

namespace TrinityALImpl
{
Tr2GpuTimerAL::Tr2GpuTimerAL() : m_lastTime( -1.f ), m_state( READY ), m_buffer( nil )
{
}

Tr2GpuTimerAL::~Tr2GpuTimerAL()
{
	Destroy();
}

ALResult Tr2GpuTimerAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	// GPU timers on metal are not stable enough (cause crashes on some machines).
	// Disabling them for now.
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
		descriptor.label = @"GPU Timer";
		descriptor.sampleCount = 2;
		descriptor.storageMode = MTLStorageModeShared;
		for( id<MTLCounterSet> counterSet in mtlDevice.counterSets )
		{
			if( [counterSet.name isEqualToString:MTLCommonCounterSetTimestamp] )
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

void Tr2GpuTimerAL::Destroy()
{
#if !__has_feature( objc_arc )
	if( m_buffer )
	{
		[m_buffer release];
	}
#endif
	m_buffer = nil;
	m_state = READY;
	m_lastTime = -1.f;
}

bool Tr2GpuTimerAL::IsValid() const
{
	return m_buffer != nil;
}

bool Tr2GpuTimerAL::Begin( Tr2RenderContextAL& renderContext )
{
	if( @available( macOS 10.15, * ) )
	{
		if( !IsValid() || !renderContext.IsValid() || m_state != READY )
		{
			return false;
		}
		renderContext.GetMetalWorkQueue()->SampleCounter( m_buffer, 0, MetalWorkQueue::COUNTER_TIMER );
		m_state = BEGIN_ISSUED;
		return true;
	}
	else
	{
		return false;
	}
}

void Tr2GpuTimerAL::End( Tr2RenderContextAL& renderContext )
{
	if( @available( macOS 10.15, * ) )
	{
		if( !IsValid() || !renderContext.IsValid() || m_state != BEGIN_ISSUED )
		{
			return;
		}
		renderContext.GetMetalWorkQueue()->SampleCounter( m_buffer, 1, MetalWorkQueue::COUNTER_TIMER );
		m_state = END_ISSUED;
	}
}

float Tr2GpuTimerAL::GetTime( Tr2RenderContextAL& renderContext )
{
	if( @available( macOS 10.15, * ) )
	{
		if( !IsValid() || m_state != END_ISSUED )
		{
			return m_lastTime;
		}
		auto data = [m_buffer resolveCounterRange:NSMakeRange( 0, 2 )];
		if( !data )
		{
			return m_lastTime;
		}
		auto timestamps = static_cast<const MTLCounterResultTimestamp*>( data.bytes );

		if( !timestamps || !timestamps[0].timestamp || !timestamps[1].timestamp )
		{
			return m_lastTime;
		}
		m_state = READY;
		if( timestamps[0].timestamp == MTLCounterErrorValue || timestamps[1].timestamp == MTLCounterErrorValue ||
			timestamps[0].timestamp > timestamps[1].timestamp )
		{
			return m_lastTime;
		}
		auto elapsed = timestamps[1].timestamp - timestamps[0].timestamp;
		m_lastTime =
			float( double( elapsed ) * renderContext.GetMetalContext()->GetGpuTimerRate() / double( NSEC_PER_SEC ) );
		return m_lastTime;
	}
	else
	{
		return -1;
	}
}

void Tr2GpuTimerAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2GpuTimerAL";
	description["name"] = m_name;
}

ALResult Tr2GpuTimerAL::SetName( const char* name )
{
	m_name = name;
	return S_OK;
}
}

#endif
