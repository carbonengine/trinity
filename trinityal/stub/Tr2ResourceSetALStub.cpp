// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_STUB

#include "Tr2ResourceSetALStub.h"

namespace TrinityALImpl
{
Tr2ResourceSetAL::Tr2ResourceSetAL() :
	m_isValid( false )
{
}

ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL&, const ::Tr2ShaderProgramAL&, Tr2PrimaryRenderContextAL& )
{
	m_isValid = true;
	return S_OK;
}

bool Tr2ResourceSetAL::IsValid() const
{
	return m_isValid;
}

void Tr2ResourceSetAL::Destroy()
{
	m_isValid = false;
}

Tr2ALMemoryType Tr2ResourceSetAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

void Tr2ResourceSetAL::Describe( Tr2DeviceResourceDescriptionAL& ) const
{
}

ALResult Tr2ResourceSetAL::SetName( const char* )
{
	return S_OK;
}
}

#endif