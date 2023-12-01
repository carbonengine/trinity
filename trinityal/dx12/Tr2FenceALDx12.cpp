#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2FenceALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"

namespace TrinityALImpl
{

	Tr2FenceAL::Tr2FenceAL()
		:m_frameIndex( 0xffffffffffffffff ),
		m_owner( nullptr )
	{
	}

	ALResult Tr2FenceAL::Create( Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}
		m_owner = &renderContext;
		return S_OK;
	}

	void Tr2FenceAL::Destroy()
	{
		m_frameIndex = 0xffffffffffffffff;
		m_owner = nullptr;
	}

	bool Tr2FenceAL::IsValid() const
	{
		return m_owner != nullptr;
	}

	ALResult Tr2FenceAL::PutFence( Tr2RenderContextAL& )
	{
		if( !m_owner )
		{
			return E_INVALIDCALL;
		}
		m_frameIndex = m_owner->GetCurrentFrameIndexDx12();
		return S_OK;
	}

	ALResult Tr2FenceAL::IsReached( bool& isReached, Tr2RenderContextAL& )
	{
		if( m_frameIndex == 0xffffffffffffffff )
		{
			return E_INVALIDCALL;
		}
		isReached = m_owner->GetCompletedFrameIndexDx12() >= m_frameIndex;
		return S_OK;
	}

	ALResult Tr2FenceAL::Wait( Tr2RenderContextAL& renderContext )
	{
		if( !m_owner )
		{
			return E_INVALIDCALL;
		}
		CR_RETURN_HR( renderContext.FlushAndSyncDx12() );
		return S_OK;
	}

	Tr2ALMemoryType Tr2FenceAL::GetMemoryClass() const
	{
		return AL_MEMORY_VIDEO;
	}

	void Tr2FenceAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2FenceAL";
		description["name"] = m_name;
	}

	ALResult Tr2FenceAL::SetName( const char* name )
	{
		m_name = name;
		return S_OK;
	}
}

#endif