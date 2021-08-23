#pragma once
#ifndef Tr2CapsALDx9_H
#define Tr2CapsALDx9_H


#include "../ALResult.h"


#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )


#define TRINITY_PLATFORM_SUPPORTS_NON_SYNCHRONIZED_LOCKS 1
#define TRINITY_PLATFORM_SUPPORTS_BUFFER_SHADER_RESOURCES 0
#define TRINITY_PLATFORM_SUPPORTS_BUFFER_COUNTERS 0
#define TRINITY_PLATFORM_SUPPORTS_UNORDERED_ACCESS 0
#define TRINITY_PLATFORM_SUPPORTS_COMPUTE 0
#define TRINITY_PLATFORM_SUPPORTS_TEXTURE_ARRAYS 0
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
private:
	Tr2CapsAL();
	Tr2CapsAL( const Tr2CapsAL& );
	Tr2CapsAL& operator=( const Tr2CapsAL& );

	ALResult QueryCaps( IDirect3DDevice9* device );

	D3DCAPS9 m_caps;

	friend class Tr2RenderContextAL;
};

#endif

#endif
