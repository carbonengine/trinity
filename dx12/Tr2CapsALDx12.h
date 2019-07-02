////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#define TRINITY_PLATFORM_SUPPORTS_NON_SYNCHRONIZED_LOCKS 1
#define TRINITY_PLATFORM_SUPPORTS_BUFFER_SHADER_RESOURCES 1
#define TRINITY_PLATFORM_SUPPORTS_UNORDERED_ACCESS 1
#define TRINITY_PLATFORM_SUPPORTS_COMPUTE 1
#define TRINITY_PLATFORM_SUPPORTS_TEXTURE_ARRAYS 1
#define TRINITY_PLATFORM_SUPPORTS_MSAA_SAMPLE 1
#define TRINITY_PLATFORM_IS_LOW_PERFORMACE 0


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
	uint32_t GetShaderVersion() const
	{
		return 5;
	}
	bool SupportsVertexShaderTextures() const
	{
		return true;
	}
	bool SupportsNoOverwriteLock() const
	{
		return true;
	}
private:
	Tr2CapsAL() {}
	Tr2CapsAL( const Tr2CapsAL& ) {}
	Tr2CapsAL& operator=( const Tr2CapsAL& ) { return *this; }

	friend class Tr2PrimaryRenderContextAL;
};

#endif
