// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionSetShaderOption.h"
#include "Controllers/Tr2Controller.h"
#include "Shader/IShaderConfigurer.h"

void Tr2ActionSetShaderOption::Start( ITr2ActionController& controller )
{
	IShaderConfigurerPtr owner = BlueCastPtr( controller.GetOwner() );
	if( nullptr == owner )
	{
		return;
	}
	owner->SetShaderOption( m_optionKey, m_optionValue );
}
