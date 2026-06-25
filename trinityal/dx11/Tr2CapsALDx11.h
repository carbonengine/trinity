// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2CapsALDx11_H
#define Tr2CapsALDx11_H

#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

#define TRINITY_PLATFORM_SUPPORTS_BUFFER_SHADER_RESOURCES 1
#define TRINITY_PLATFORM_SUPPORTS_BUFFER_COUNTERS 1
#define TRINITY_PLATFORM_SUPPORTS_UNORDERED_ACCESS 1
#define TRINITY_PLATFORM_SUPPORTS_COMPUTE 1
#define TRINITY_PLATFORM_SUPPORTS_TEXTURE_ARRAYS 1
#define TRINITY_PLATFORM_SUPPORTS_MSAA_SAMPLE 1
#define TRINITY_PLATFORM_SUPPORTS_RENDER_PASS_HINTS 0
#define TRINITY_PLATFORM_IS_LOW_PERFORMACE 0
#define TRINITY_PLATFORM_MAX_CONSTANT_BUFFER_SIZE ( 64 * 1024 )
#define TRINITY_PLATFORM_SUPPORTS_RAY_TRACING 0


class Tr2CapsAL
{
public:
	bool SupportsFloat16() const;
	bool SupportsGpuBuffer() const;
	bool SupportsStandaloneSwapChain() const;
	bool SupportsVertexShaderTextures() const;
	bool SupportsVariableRefreshRate() const;
	bool SupportsRaytracing() const;

private:
	Tr2CapsAL();
	Tr2CapsAL( const Tr2CapsAL& );
	Tr2CapsAL& operator=( const Tr2CapsAL& );

	friend class Tr2PrimaryRenderContextAL;
};

#endif

#endif
