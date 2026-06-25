// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPLutEffect.h"


Tr2PPLutEffect::Tr2PPLutEffect( IRoot* lockobj ) :
	m_influence( 0.0f ),
	m_path( "res:/dx9/scene/postprocess/LUTdefault.dds" )
{
}

Tr2PPLutEffect::~Tr2PPLutEffect()
{
}


bool Tr2PPLutEffect::IsActive()
{
	return m_display && m_influence > 0.0f;
}