#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "Tr2FenceALDx9.h"
#include "Tr2RenderContextDx9.h"

namespace TrinityALImpl
{

	Tr2FenceAL::Tr2FenceAL()
	{
	}

	ALResult Tr2FenceAL::Create( Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();
		if( !renderContext.IsValid() )
		{
			return E_FAIL;
		}
		return renderContext.m_d3dDevice9->CreateQuery( D3DQUERYTYPE_EVENT, &m_query );
	}

	void Tr2FenceAL::Destroy()
	{
		m_query = nullptr;
	}

	bool Tr2FenceAL::IsValid() const
	{
		return m_query != nullptr;
	}

	ALResult Tr2FenceAL::PutFence( Tr2RenderContextAL& )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		return m_query->Issue( D3DISSUE_END );
	}

	ALResult Tr2FenceAL::IsReached( bool& isReached, Tr2RenderContextAL& )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		HRESULT hr = m_query->GetData( nullptr, 0, 0 );
		CR_RETURN_HR( hr );
		isReached = hr == S_OK;
		return S_OK;
	}

	ALResult Tr2FenceAL::Wait( Tr2RenderContextAL& )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		while( true )
		{
			HRESULT hr = m_query->GetData( nullptr, 0, D3DGETDATA_FLUSH );
			CR_RETURN_HR( hr );
			if( hr == S_OK )
			{
				break;
			}
		}
		return S_OK;
	}

	void Tr2FenceAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2FenceAL";
	}
}

#endif