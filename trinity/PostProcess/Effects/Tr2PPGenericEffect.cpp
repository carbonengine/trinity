// Copyright © 2026 CCP ehf.

#include "Tr2PPGenericEffect.h"

Tr2PPGenericEffect::Tr2PPGenericEffect( IRoot* lockobj ) :
	m_quality( PostProcess::Quality::MEDIUM )
{
}

Tr2EffectPtr Tr2PPGenericEffect::GetEffect() const
{
	return m_effect;
}
