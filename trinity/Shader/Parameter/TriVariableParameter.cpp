// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriVariableParameter.h"
#include "Tr2VariableStore.h"
#include "Shader/Tr2Shader.h"

TriVariableParameter::TriVariableParameter( IRoot* lockobj ) :
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

unsigned TriVariableParameter::GetHashValue( unsigned startingHash ) const
{
	auto name = m_name.c_str();
	return CcpHashFNV1( &name, sizeof( name ), startingHash );
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

// ---------------------------------------------------------------
bool TriVariableParameter::CopyToResourceSet(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex,
	ResourceFlags flags ) const
{
	if( !m_variable )
	{
		return false;
	}
	return m_variable->CopyToResourceSet( resourceDesc, stage, registerIndex, flags );
}

// ---------------------------------------------------------------
bool TriVariableParameter::ApplyUav(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex ) const
{
	if( m_variable )
	{
		return m_variable->ApplyUav( resourceDesc, stage, registerIndex );
	}
	return resourceDesc.SetUav( stage, registerIndex, Tr2TextureAL() );
}

void TriVariableParameter::CopyValueToEffect( Tr2RenderContextEnum::ShaderType inputType,
											  unsigned char* destHandle,
											  size_t size,
											  Tr2RenderContext& renderContext ) const
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

void TriVariableParameter::RebuildEffectHandles( Tr2Shader* effectRes )
{
	m_cachedEffect = effectRes;

	m_isUsedByEffect = false;

	if( m_name.empty() || !effectRes )
	{
		return;
	}

	if( m_variable )
	{
		if( m_variable->GetType() == TRIVARIABLE_TEXTURE_RES )
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
