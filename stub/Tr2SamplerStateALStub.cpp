#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "Tr2SamplerStateALStub.h"

// Force anisotropic filtering switch:
// 0 means use whatever is specified in the effect (default)
// 1 means turn off anisotropic filtering (fallback to linear)
// >1 - force max anisotropy to specified number
uint32_t g_forceAnisotropy = 0;

using namespace Tr2RenderContextEnum;

Tr2SamplerStateAL::Tr2SamplerStateAL()
{
	m_isValid = false;
}

ALResult Tr2SamplerStateAL::Create( 
	Tr2RenderContextAL& /*renderContext*/,
	const Tr2SamplerDescription& description )
{
	m_isValid = true;
	return S_OK;
}

bool Tr2SamplerStateAL::IsValid() const
{
	return m_isValid;
}



#endif // TRINITY_PLATFORM==TRINITY_STUB