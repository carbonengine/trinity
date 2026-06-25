// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if ( TRINITY_PLATFORM == TRINITY_STUB )

#include "Tr2OcclusionQueryALStub.h"
#include "Tr2RenderContextStub.h"

namespace TrinityALImpl
{

Tr2OcclusionQueryAL::Tr2OcclusionQueryAL() :
	m_isValid( false ),
	m_isRunning( false )
{
}

Tr2OcclusionQueryAL::~Tr2OcclusionQueryAL()
{
}

ALResult Tr2OcclusionQueryAL::Create( Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
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

ALResult Tr2OcclusionQueryAL::Begin( Tr2RenderContextAL& /*renderContext*/ )
{
	if( !m_isValid )
	{
		return E_INVALIDCALL;
	}
	m_isRunning = true;
	return S_OK;
}

ALResult Tr2OcclusionQueryAL::End( Tr2RenderContextAL& /*renderContext*/ )
{
	if( !m_isValid )
	{
		return E_INVALIDCALL;
	}
	if( !m_isRunning )
	{
		return E_INVALIDCALL;
	}
	m_isRunning = false;
	return S_OK;
}

ALResult Tr2OcclusionQueryAL::GetPixelCount( Tr2RenderContextAL& /*renderContext*/, uint32_t& count, ::Tr2OcclusionQueryAL::WaitMode )
{
	if( !m_isValid )
	{
		return E_INVALIDCALL;
	}
	count = 0;
	return S_OK;
}

void Tr2OcclusionQueryAL::Describe( Tr2DeviceResourceDescriptionAL& ) const
{
}

ALResult Tr2OcclusionQueryAL::SetName( const char* )
{
	return S_OK;
}
}
#endif