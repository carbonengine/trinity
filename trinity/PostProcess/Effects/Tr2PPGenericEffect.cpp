#include "Tr2PPGenericEffect.h"
#include "PostProcess/Tr2PostProcessRenderer.h"

Tr2PPGenericEffect::Tr2PPGenericEffect( IRoot* lockobj ) :
	m_shaderPath( "" ),
	m_quality( PostProcess::Quality::MEDIUM )
{
}

bool Tr2PPGenericEffect::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_shaderPath ) )
	{
		m_recreateEffect = true;
	}
	return true;
}

Tr2EffectPtr Tr2PPGenericEffect::GetEffect()
{
	if( m_recreateEffect )
	{
		m_effect = nullptr;
		if( !m_shaderPath.empty() )
		{
			m_effect.CreateInstance();
			m_effect->SetEffectPathName( m_shaderPath.c_str() );
		}
		m_recreateEffect = false;
	}
	return m_effect;
}
