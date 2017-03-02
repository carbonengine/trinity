#pragma once
#ifndef Tr2CapsALStub_H
#define Tr2CapsALStub_H

#if( TRINITY_PLATFORM==TRINITY_STUB )

class Tr2CapsAL
{
public:
	bool SupportsFloat16() const;
	bool SupportsGpuBuffer() const;
	bool SupportsStandaloneSwapChain() const;
	uint32_t GetShaderVersion() const;
	bool SupportsVertexShaderTextures() const;
	bool SupportsNoOverwriteLock() const;

};

#endif

#endif