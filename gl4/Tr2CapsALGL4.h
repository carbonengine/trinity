#pragma once
#ifndef Tr2CapsALGLES2_H
#define Tr2CapsALGLES2_H

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#define TR2_SUPPORTS_NON_SYNCHRONIZED_LOCKS 1
#define TR2_SUPPORTS_BUFFER_SHADER_RESOURCES 1
#define TR2_SUPPORTS_UNORDERED_ACCESS 1

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

	friend class Tr2RenderContextAL;
};

#endif

#endif