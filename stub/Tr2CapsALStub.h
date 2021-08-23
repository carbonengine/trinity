#pragma once
#ifndef Tr2CapsALStub_H
#define Tr2CapsALStub_H

#if( TRINITY_PLATFORM==TRINITY_STUB )


#define TRINITY_PLATFORM_SUPPORTS_NON_SYNCHRONIZED_LOCKS 1
#define TRINITY_PLATFORM_SUPPORTS_BUFFER_SHADER_RESOURCES 0
#define TRINITY_PLATFORM_SUPPORTS_BUFFER_COUNTERS 0
#define TRINITY_PLATFORM_SUPPORTS_UNORDERED_ACCESS 0
#define TRINITY_PLATFORM_SUPPORTS_COMPUTE 0
#define TRINITY_PLATFORM_SUPPORTS_TEXTURE_ARRAYS 1
#define TRINITY_PLATFORM_SUPPORTS_MSAA_SAMPLE 0
#define TRINITY_PLATFORM_SUPPORTS_RENDER_PASS_HINTS 0
#define TRINITY_PLATFORM_IS_LOW_PERFORMACE 1
#define TRINITY_PLATFORM_MAX_CONSTANT_BUFFER_SIZE ( 4 * 1024 )


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
