// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if ( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2OcclusionQueryALMetal.h"
#include "Tr2RenderContextMetal.h"

namespace TrinityALImpl
{
Tr2OcclusionQueryAL::Tr2OcclusionQueryAL() : m_isValid( false ), m_isRunning( false )
{
}

Tr2OcclusionQueryAL::~Tr2OcclusionQueryAL()
{
	Destroy();
}

ALResult Tr2OcclusionQueryAL::Create( Tr2RenderContextAL& renderContext )
{
	Destroy();

	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}

	m_currentQueryNumber = 0;
	renderContext.GetMetalWorkQueue()->CreateVisibilityQueryBuffer( 1000 );

	m_isValid = true;
	return S_OK;
}

bool Tr2OcclusionQueryAL::IsValid() const
{
	return m_isValid;
}

void Tr2OcclusionQueryAL::Destroy()
{
	m_isValid = false;
}

ALResult Tr2OcclusionQueryAL::Begin( Tr2RenderContextAL& renderContext )
{
	if( !m_isValid || m_isRunning )
	{
		return E_INVALIDCALL;
	}

	m_currentQueryNumber = renderContext.GetMetalWorkQueue()->StartVisibilityQuery();
	m_isRunning = true;
	return S_OK;
}

ALResult Tr2OcclusionQueryAL::End( Tr2RenderContextAL& renderContext )
{
	if( !m_isValid )
	{
		return E_INVALIDCALL;
	}
	if( !m_isRunning )
	{
		return E_INVALIDCALL;
	}

	renderContext.GetMetalWorkQueue()->EndVisibilityQuery( m_currentQueryNumber );
	m_isRunning = false;
	return S_OK;
}

ALResult Tr2OcclusionQueryAL::GetPixelCount( Tr2RenderContextAL& renderContext,
											 uint32_t& count,
											 ::Tr2OcclusionQueryAL::WaitMode waitMode )
{
	if( !m_isValid )
	{
		return E_INVALIDCALL;
	}

	bool waitForOutstandingWork = waitMode == ::Tr2OcclusionQueryAL::WAIT ? true : false;
	uint64_t pixelCount;
	bool pixelCountReady;
	pixelCountReady = renderContext.GetMetalWorkQueue()->GetVisibilityQueryPixelCount(
		m_currentQueryNumber, &pixelCount, waitForOutstandingWork );
	if( pixelCountReady )
	{
		count = (uint32_t)pixelCount;
		return S_OK;
	}

	return S_FALSE;
}

void Tr2OcclusionQueryAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2OcclusionQueryAL";
	description["name"] = m_name;
}

ALResult Tr2OcclusionQueryAL::SetName( const char* name )
{
	m_name = name;
	return S_OK;
}
}

#endif
