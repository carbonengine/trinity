#pragma once
#ifndef Tr2CapsALStub_H
#define Tr2CapsALStub_H

#if( TRINITY_PLATFORM==TRINITY_STUB )


#define TR2_SUPPORTS_NON_SYNCHRONIZED_LOCKS 1
#define TR2_SUPPORTS_BUFFER_SHADER_RESOURCES 0
#define TR2_SUPPORTS_UNORDERED_ACCESS 0


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