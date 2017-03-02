#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )
#include "Tr2CapsALGL4.h"

Tr2CapsAL::Tr2CapsAL()
{
}

bool Tr2CapsAL::SupportsFloat16() const
{
    return true;
}

bool Tr2CapsAL::SupportsGpuBuffer() const
{
	return true;
}

bool Tr2CapsAL::SupportsStandaloneSwapChain() const
{
	return true;
}

uint32_t Tr2CapsAL::GetShaderVersion() const
{
	return 2;
}

bool Tr2CapsAL::SupportsVertexShaderTextures() const
{
	return true;
}

bool Tr2CapsAL::SupportsNoOverwriteLock() const
{
    return true;
}

#endif
