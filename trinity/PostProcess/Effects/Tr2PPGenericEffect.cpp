#include "Tr2PPGenericEffect.h"
#include "PostProcess/Tr2PostProcessRenderer.h"

Tr2PPGenericEffect::Tr2PPGenericEffect( IRoot* lockobj ) :
	m_shaderPath( "" ),
	m_quality( PostProcess::Quality::MEDIUM )
{
}
