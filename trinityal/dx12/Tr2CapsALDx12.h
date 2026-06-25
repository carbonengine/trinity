// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#define TRINITY_PLATFORM_SUPPORTS_BUFFER_SHADER_RESOURCES 1
#define TRINITY_PLATFORM_SUPPORTS_BUFFER_COUNTERS 1
#define TRINITY_PLATFORM_SUPPORTS_UNORDERED_ACCESS 1
#define TRINITY_PLATFORM_SUPPORTS_COMPUTE 1
#define TRINITY_PLATFORM_SUPPORTS_TEXTURE_ARRAYS 1
#define TRINITY_PLATFORM_SUPPORTS_MSAA_SAMPLE 1
#define TRINITY_PLATFORM_SUPPORTS_RENDER_PASS_HINTS 0
#define TRINITY_PLATFORM_IS_LOW_PERFORMACE 0
#define TRINITY_PLATFORM_MAX_CONSTANT_BUFFER_SIZE ( 64 * 1024 )
#define TRINITY_PLATFORM_SUPPORTS_HEAP_VIEW 1
#define TRINITY_PLATFORM_SUPPORTS_SHADER_PROGRAM_SAMPLERS 1
#define TRINITY_PLATFORM_SUPPORTS_RAY_TRACING 1


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
		// Rather than create the 1.5 factory interface directly, we create the 1.4
		// interface and query for the 1.5 interface. This will enable the graphics
		// debugging tools which might not support the 1.5 factory interface.
		CComPtr<IDXGIFactory4> factory4;
		UINT createFactoryFlags = 0;

#if ( TRINITYDEV == 1 )
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		if( SUCCEEDED( CreateDXGIFactory2( createFactoryFlags, IID_PPV_ARGS( &factory4 ) ) ) )
		{
			BOOL allowTearing = FALSE;

			CComPtr<IDXGIFactory5> factory5;
			if( SUCCEEDED( factory4.QueryInterface( &factory5 ) ) )
			{
				if( SUCCEEDED( factory5->CheckFeatureSupport( DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof( allowTearing ) ) ) )
				{
					return allowTearing;
				}
			}
		}
		return false;
	}

	bool SupportsRaytracing() const
	{
		return m_supportsDxr;
	}

private:
	Tr2CapsAL() :
		m_supportsDxr( false )
	{
	}

	Tr2CapsAL( const Tr2CapsAL& other ) :
		m_supportsDxr( other.m_supportsDxr )
	{
	}

	Tr2CapsAL& operator=( const Tr2CapsAL& other )
	{
		m_supportsDxr = other.m_supportsDxr;
		return *this;
	}

	bool m_supportsDxr;

	friend class Tr2PrimaryRenderContextAL;
};

#endif
