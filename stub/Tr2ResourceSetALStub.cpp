#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_STUB

#include "Tr2ResourceSetALStub.h"

namespace TrinityALImpl
{
	Tr2ResourceSetAL::Tr2ResourceSetAL()
		:m_isValid( false )
	{
	}

	ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, Tr2PrimaryRenderContextAL& renderContext )
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
}

#endif