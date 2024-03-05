#pragma once

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2RenderContext )


class Tr2GpuRegion
{
public:
	Tr2GpuRegion( Tr2RenderContextAL& renderContext, const char* name )
		:m_renderContext( renderContext )
	{
		m_renderContext.PushGpuMarker( name );
	}
	~Tr2GpuRegion()
	{
		m_renderContext.PopGpuMarker();
	}
private:
	Tr2RenderContextAL& m_renderContext;
};


#define GPU_REGION_AL( renderContext, name ) Tr2GpuRegion _gpuRegion##__COUNTER__( renderContext, name )
