// Copyright © 2023 CCP ehf.

#pragma once

#if ( TRINITY_PLATFORM == TRINITY_METAL )

#define TRINITY_PLATFORM_SUPPORTS_BUFFER_SHADER_RESOURCES 1
#define TRINITY_PLATFORM_SUPPORTS_BUFFER_COUNTERS 0
#define TRINITY_PLATFORM_SUPPORTS_UNORDERED_ACCESS 1
#define TRINITY_PLATFORM_SUPPORTS_COMPUTE 1
#define TRINITY_PLATFORM_SUPPORTS_TEXTURE_ARRAYS 1
#define TRINITY_PLATFORM_SUPPORTS_MSAA_SAMPLE 1
#define TRINITY_PLATFORM_SUPPORTS_RENDER_PASS_HINTS 1
#define TRINITY_PLATFORM_IS_LOW_PERFORMACE 0
#define TRINITY_PLATFORM_MAX_CONSTANT_BUFFER_SIZE ( 4 * 1024 )
#define TRINITY_PLATFORM_SUPPORTS_PARALLEL_CONTEXTS 1
#define TRINITY_PLATFORM_SUPPORTS_HEAP_VIEW 1
#define TRINITY_PLATFORM_SUPPORTS_RAY_TRACING 1

class Tr2CapsAL
{
public:
	bool SupportsFloat16() const;
	bool SupportsGpuBuffer() const;
	bool SupportsStandaloneSwapChain() const;
	bool SupportsVertexShaderTextures() const;
	bool SupportsVariableRefreshRate() const;
	bool SupportsRaytracing() const;

	bool m_supportsRaytracing;

	Tr2CapsAL();
	Tr2CapsAL( const Tr2CapsAL& other );

	Tr2CapsAL& operator=( const Tr2CapsAL& other );
};

#endif
