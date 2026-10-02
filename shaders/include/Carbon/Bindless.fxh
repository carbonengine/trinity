// Copyright © 2026 CCP ehf.

#ifndef CARBON_BINDLESS_FXH
#define CARBON_BINDLESS_FXH

#include "System.fxh"

// Support for bindless rendering with Trinity. Difines types and functions to work with
// bindless resources and heap views in a platform-independent manner.

// BINDLESS_RENDERING define controls whether bindless rendering is enabled.

// Don't even try to use bindless rendering on platforms that don't support it (DirectX 11)
#if !PLATFORM_SUPPORTS_SUPPORTS_HEAP_VIEW
#define BINDLESS_RENDERING 0
#endif

#ifndef BINDLESS_RENDERING
#define BINDLESS_RENDERING 1
#endif

#if BINDLESS_RENDERING

Texture2D<float4> HeapView_Texture2D[]
<
 bool IsHeapView = true;
>;

Texture3D<float4> HeapView_Texture3D[]
<
 bool IsHeapView = true;
>;

TextureCube<float4> HeapView_TextureCube[]
<
 bool IsHeapView = true;
>;

SamplerState HeapView_Sampler[]
<
 bool IsHeapView = true;
>;

#endif

// Convenient shortcut to declaring bindless/class texture variables
#if BINDLESS_RENDERING
// CCP Shader compiler defines custom types BindlessHandleTexture... that map to uint on DX12 and Metal
#define BindlessTexture2D BindlessHandleTexture2D
#define BindlessTexture3D BindlessHandleTexture3D
#define BindlessTextureCube BindlessHandleTextureCube
#define BindlessSampler BindlessHandleSampler
#else
#define BindlessTexture2D Texture2D
#define BindlessTexture3D Texture3D
#define BindlessTextureCube TextureCube
#define BindlessSampler SamplerState
#endif

Texture2D Deref2D( BindlessTexture2D tex )
{
#if BINDLESS_RENDERING
	return HeapView_Texture2D[tex];
#else
	return tex;
#endif
}

Texture3D Deref3D( BindlessTexture3D tex )
{
#if BINDLESS_RENDERING
	return HeapView_Texture3D[tex];
#else
	return tex;
#endif
}

TextureCube DerefCube( BindlessTextureCube tex )
{
#if BINDLESS_RENDERING
	return HeapView_TextureCube[tex];
#else
	return tex;
#endif
}

SamplerState DerefSampler( BindlessSampler sampl )
{
#if BINDLESS_RENDERING
	return HeapView_Sampler[sampl];
#else
	return sampl;
#endif
}


#endif