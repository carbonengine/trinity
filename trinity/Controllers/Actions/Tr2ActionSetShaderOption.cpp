////////////////////////////////////////////////////////////
//
//    Created:   September 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionSetShaderOption.h"
#include "Controllers/Tr2Controller.h"
#include "Shader/IShaderConfigurer.h"

void Tr2ActionSetShaderOption::Start( Tr2Controller& controller )
{
	IShaderConfigurerPtr owner = BlueCastPtr( controller.GetOwner() );
	if ( nullptr == owner )
	{
		return;
	}
	owner->SetShaderOption( m_optionKey, m_optionValue );
}
