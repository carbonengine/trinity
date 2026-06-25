// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2ProceduralResources.h"
#include "Tr2Renderer.h"

namespace
{

template <typename T>
class Tr2ProceduralResourceRegistry : public Tr2DeviceResource
{
public:
	const typename T::ResourceType* GetResource( const BlueSharedString& name, typename T::FactoryFunction factory )
	{
		auto found = m_resources.find( name );
		if( found != m_resources.end() )
		{
			return &found->second->resource;
		}
		auto resource = new Resource;
		resource->factory = factory;
		if( Tr2Renderer::IsResourceCreationAllowed() )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			resource->factory( resource->resource, renderContext );
		}
		m_resources[name] = resource;
		return &resource->resource;
	}

protected:
	void ReleaseResources( TriStorage ) override
	{
	}

	bool OnPrepareResources() override
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		for( auto it = begin( m_resources ); it != end( m_resources ); ++it )
		{
			if( !it->second->resource.IsValid() )
			{
				it->second->factory( it->second->resource, renderContext );
			}
		}
		return true;
	}

private:
	struct Resource
	{
		typename T::ResourceType resource;
		typename T::FactoryFunction factory;
	};
	std::map<BlueSharedString, Resource*> m_resources;
};

Tr2ProceduralResourceRegistry<Tr2ProceduralBuffer>& GetBuffers()
{
	static Tr2ProceduralResourceRegistry<Tr2ProceduralBuffer> instance;
	return instance;
}
}

Tr2ProceduralBuffer::Tr2ProceduralBuffer( const BlueSharedString& name, FactoryFunction factory ) :
	m_resource( GetBuffers().GetResource( name, factory ) )
{
}

const Tr2SuballocatedBuffer::Allocation& Tr2ProceduralBuffer::GetSharedResource() const
{
	return *m_resource;
}
