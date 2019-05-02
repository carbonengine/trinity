////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#define TR2_SUPPORTS_NON_SYNCHRONIZED_LOCKS 1
#define TR2_SUPPORTS_BUFFER_SHADER_RESOURCES 1
#define TR2_SUPPORTS_UNORDERED_ACCESS 1
#define TR2_SUPPORTS_COMPUTE 1
#define TR2_SUPPORTS_TEXTURE_ARRAYS 1

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
