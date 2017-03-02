#pragma once
#ifndef Tr2CapsALDx9_H
#define Tr2CapsALDx9_H

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

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