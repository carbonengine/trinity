#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "Tr2GpuTimerALGL4.h"


Tr2GpuTimerAL::Tr2GpuTimerAL()
{
}

ALResult Tr2GpuTimerAL::Create( Tr2PrimaryRenderContextAL& )
{
	return E_FAIL;
}

void Tr2GpuTimerAL::Destroy()
{
}

bool Tr2GpuTimerAL::IsValid() const
{
	return false;
}

bool Tr2GpuTimerAL::Begin( Tr2RenderContextAL& )
{
	return false;
}

void Tr2GpuTimerAL::End( Tr2RenderContextAL& )
{
}

float Tr2GpuTimerAL::GetTime( Tr2RenderContextAL& )
{
	return -1.f;
}

#endif