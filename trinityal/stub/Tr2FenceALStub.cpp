#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "Tr2FenceALStub.h"
#include "Tr2RenderContextStub.h"

namespace TrinityALImpl
{

	Tr2FenceAL::Tr2FenceAL()
		:m_isValid( false ),
		m_hasFence( false )
	{
	}

	Tr2FenceAL::~Tr2FenceAL()
	{
	}

	ALResult Tr2FenceAL::Create( Tr2PrimaryRenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}
		m_isValid = true;
		return S_OK;
	}

	void Tr2FenceAL::Destroy()
	{
		m_isValid = false;
		m_hasFence = false;
	}

	bool Tr2FenceAL::IsValid() const
	{
		return m_isValid;
	}

	ALResult Tr2FenceAL::PutFence( Tr2RenderContextAL& )
	{
		if( !m_isValid )
		{
			return E_FAIL;
		}
		if( m_hasFence )
		{
			return E_INVALIDCALL;
		}
		m_hasFence = true;
		return S_OK;

	}

	ALResult Tr2FenceAL::IsReached( bool& isReached, Tr2RenderContextAL& )
	{
		if( !m_isValid )
		{
			return E_FAIL;
		}
		isReached = !m_hasFence;
		return S_OK;
	}

	ALResult Tr2FenceAL::Wait( Tr2RenderContextAL& )
	{
		if( !m_isValid )
		{
			return E_FAIL;
		}
		if( !m_hasFence )
		{
			return E_INVALIDCALL;
		}
		m_hasFence = false;
		return S_OK;
	}

	void Tr2FenceAL::Describe( Tr2DeviceResourceDescriptionAL& ) const
	{
	}

	ALResult Tr2FenceAL::SetName( const char* )
	{
		return S_OK;
	}
}

#endif