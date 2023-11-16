////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM==TRINITY_VULKAN

#define TRINITY_PLATFORM_SUPPORTS_BUFFER_SHADER_RESOURCES 1
#define TRINITY_PLATFORM_SUPPORTS_BUFFER_COUNTERS 0
#define TRINITY_PLATFORM_SUPPORTS_UNORDERED_ACCESS 1
#define TRINITY_PLATFORM_SUPPORTS_COMPUTE 1
#define TRINITY_PLATFORM_SUPPORTS_TEXTURE_ARRAYS 1
#define TRINITY_PLATFORM_SUPPORTS_MSAA_SAMPLE 1
#define TRINITY_PLATFORM_SUPPORTS_RENDER_PASS_HINTS 0
#define TRINITY_PLATFORM_IS_LOW_PERFORMACE 0
#define TRINITY_PLATFORM_MAX_CONSTANT_BUFFER_SIZE ( 64 * 1024 )


class Tr2CapsAL
{
public:
	bool SupportsFloat16() const
	{
		return true;
	}
	bool SupportsGpuBuffer() const
	{
		return true;
	}
	bool SupportsStandaloneSwapChain() const
	{
		return true;
	}
	bool SupportsVertexShaderTextures() const
	{
		return true;
	}
	bool SupportsVariableRefreshRate() const
	{
		return false;
	}

private:
	Tr2CapsAL() {}
	Tr2CapsAL( const Tr2CapsAL& ) {}
	Tr2CapsAL& operator=( const Tr2CapsAL& ) { return *this; }

	friend class Tr2PrimaryRenderContextAL;
};

#endif
