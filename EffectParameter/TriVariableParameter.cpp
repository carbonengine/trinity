#include "StdAfx.h"
#include "TriVariableParameter.h"
#include "Tr2VariableStore.h"
#include "ITr2ShaderState.h"

TriVariableParameter::TriVariableParameter(IRoot* lockobj):
	m_isUsedByEffect( false ),
	m_variable( NULL )
{
}


TriVariableParameter::~TriVariableParameter()
{
}


const char* TriVariableParameter::GetParameterName() const
{
	return m_name.c_str();
}

// --------------------------------------------------------------------------------------
// Description:
//   Determines whether the TriVariable stored in this parameter is NULL and can be 
//   ignored when building the material situation.
// Return Value:
//   true, if the TriVariable is NULL
//   false, otherwise
// --------------------------------------------------------------------------------------
bool TriVariableParameter::IsZeroOrNull( void ) const
{
	return m_variable == NULL;
}

bool TriVariableParameter::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_name ) )
	{
		RebuildEffectHandles( m_cachedEffect );
	}
	else
	{
		Initialize();
	}
	return true;
}

// ---------------------------------------------------------------
bool TriVariableParameter::Initialize()
{
	if( !m_variableName.empty() )
	{
        m_variable = GlobalStore().GetVariable( m_variableName.c_str() );
	}
    else
    {
		m_variable = NULL;
    }
	return true;
}

size_t TriVariableParameter::GetValueSize() const
{
	if( m_variable )
	{
		return m_variable->GetValueSize();
	}
	else
	{
		return 0;
	}
}

void TriVariableParameter::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
												unsigned char* destHandle, 
												size_t size,
												Tr2RenderContext &renderContext ) const
{
	if( m_variable )
	{
		m_variable->CopyValueToEffect( inputType, destHandle, size, renderContext );
	}
}

int TriVariableParameter::GetVariableType() const
{ 
	return m_variable ? m_variable->GetType() : TRIVARIABLE_INVALID; 
}

void TriVariableParameter::RebuildEffectHandles( ITr2ShaderState* effectRes )
{
	m_cachedEffect = effectRes;

	m_isUsedByEffect = false;

	if ( m_name.empty() || !effectRes )
	{
		return;
	}

	if( m_variable )
	{
		if( m_variable->GetType() == TRIVARIABLE_TEXTURE_RES ||
			m_variable->GetType() == TRIVARIABLE_TEXTURE_AL  )
		{
			if( !effectRes->GetResource( m_name.c_str() ) )
			{
				return;
			}
		}
		else
		{
			if( !effectRes->GetConstant( m_name.c_str() ) )
			{
				return;
			}
		}
		m_isUsedByEffect = true;
	}
}
