// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#if ( TRINITY_PLATFORM == TRINITY_STUB )

#include "Tr2GpuTimerALStub.h"
#include "Tr2PrimaryRenderContextStub.h"

namespace TrinityALImpl
{
Tr2GpuTimerAL::Tr2GpuTimerAL() :
	m_isValid( false )
{
}

ALResult Tr2GpuTimerAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	m_isValid = true;
	return S_OK;
}

void Tr2GpuTimerAL::Destroy()
{
	m_isValid = false;
}

bool Tr2GpuTimerAL::IsValid() const
{
	return m_isValid;
}

bool Tr2GpuTimerAL::Begin( Tr2RenderContextAL& )
{
	return true;
}

void Tr2GpuTimerAL::End( Tr2RenderContextAL& )
{
}

float Tr2GpuTimerAL::GetTime( Tr2RenderContextAL& )
{
	return m_isValid ? 0.0001f : -1.f;
}

void Tr2GpuTimerAL::Describe( Tr2DeviceResourceDescriptionAL& ) const
{
}

ALResult Tr2GpuTimerAL::SetName( const char* )
{
	return S_OK;
}
}

#endif