// Copyright © 2026 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_STUB

#include "Tr2AsyncCreatorStub.h"
#include "Tr2BufferALStub.h"
#include "Tr2TextureALStub.h"
#include "Tr2PrimaryRenderContextStub.h"

namespace TrinityALImpl
{

Tr2AsyncCreatorAL::Tr2AsyncCreatorAL( Tr2PrimaryRenderContextAL& renderContext, std::shared_ptr<Tr2AsyncDispatcherAL> dispatcher ) :
	m_renderContext( renderContext ),
	m_dispatcher( std::move( dispatcher ) )
{
}

ALResult Tr2AsyncCreatorAL::Initialize()
{
	return S_OK;
}

void Tr2AsyncCreatorAL::Shutdown()
{
	m_guard.Shutdown();
}

void Tr2AsyncCreatorAL::ProcessCompleted()
{
}

template <typename Public, typename Impl>
Tr2FutureAL<Public> Tr2AsyncCreatorAL::Create( std::function<HRESULT( Impl& )> create, std::function<Public( std::shared_ptr<Impl> )> wrap )
{
	auto state = std::make_shared<Tr2FutureStateAL<Public>>( m_dispatcher );
	bool done = m_guard.Use( [&]() {
		auto object = std::make_shared<Impl>();
		HRESULT hr = create( *object );
		if( SUCCEEDED( hr ) )
		{
			state->Resolve( wrap( object ) );
		}
		else
		{
			state->Reject( hr );
		}
	} );
	if( !done )
	{
		state->Reject( E_ABORT );
	}
	return Tr2FutureAL<Public>( state );
}

Tr2FutureAL<::Tr2BufferAL> Tr2AsyncCreatorAL::CreateBuffer(
	const Tr2BufferDescriptionAL& desc,
	const void* initialData,
	WrapBufferFunction wrap )
{
	auto& renderContext = m_renderContext;
	return Create<::Tr2BufferAL, Tr2BufferAL>(
		[&]( Tr2BufferAL& buffer ) -> HRESULT {
			return buffer.Create( desc, initialData, renderContext ).GetResult();
		},
		std::move( wrap ) );
}

Tr2FutureAL<::Tr2TextureAL> Tr2AsyncCreatorAL::CreateTexture(
	const Tr2BitmapDimensions& desc,
	const Tr2MsaaDesc& msaa,
	Tr2GpuUsage::Type gpuUsage,
	Tr2CpuUsage::Type cpuUsage,
	Tr2SubresourceData* initialData,
	WrapTextureFunction wrap )
{
	auto& renderContext = m_renderContext;
	return Create<::Tr2TextureAL, Tr2TextureAL>(
		[&]( Tr2TextureAL& texture ) -> HRESULT {
			return texture.Create( desc, msaa, gpuUsage, cpuUsage, initialData, renderContext ).GetResult();
		},
		std::move( wrap ) );
}

}

#endif
