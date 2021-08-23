#include "StdAfx.h"
#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2FenceALMetal.h"
#include "Tr2RenderContextMetal.h"

namespace TrinityALImpl
{
	Tr2FenceAL::Tr2FenceAL()
		: m_frame( 0 ),
		m_isValid( false ),
		m_hasBeenPut( false )
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
		m_hasBeenPut = false;
		return S_OK;
	}

	void Tr2FenceAL::Destroy()
	{
		m_isValid = false;
	}

	bool Tr2FenceAL::IsValid() const
	{
		return m_isValid;
	}

	ALResult Tr2FenceAL::PutFence( Tr2RenderContextAL& renderContext )
	{
		if( !m_isValid || !renderContext.IsValid() )
		{
			return E_FAIL;
		}

		MetalContext *metalContext = renderContext.GetMetalContext();
		m_frame = metalContext->GetRecordingFrameNumber();
		m_hasBeenPut = true;
		return S_OK;

	}

	ALResult Tr2FenceAL::IsReached( bool& isReached, Tr2RenderContextAL& renderContext)
	{
		if( !m_isValid || !m_hasBeenPut || !renderContext.IsValid() )
		{
			return E_FAIL;
		}

		MetalContext *metalContext = renderContext.GetMetalContext();
		isReached = metalContext->GetRenderedFrameNumber() >= m_frame;

		return S_OK;
	}

	ALResult Tr2FenceAL::Wait( Tr2RenderContextAL& )
	{
		return E_FAIL;
	}

	void Tr2FenceAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2FenceAL";
	}
}

#endif
