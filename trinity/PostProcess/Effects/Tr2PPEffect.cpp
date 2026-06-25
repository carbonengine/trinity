// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPEffect.h"

Tr2PPEffect::Tr2PPEffect( IRoot* lockobj ) :
	m_display( true )
{
}

Tr2PPEffect::~Tr2PPEffect()
{
}

bool Tr2PPEffect::IsActive()
{
	return m_display;
}
