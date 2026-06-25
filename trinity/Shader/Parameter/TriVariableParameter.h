// Copyright © 2007 CCP ehf.

#pragma once
#ifndef TriVariableParameter_H
#define TriVariableParameter_H

#include "include/ITriEffectParameter.h"

class TriVariable;
BLUE_DECLARE( TriVariableParameter );
BLUE_CLASS_ALLOW_DELAYED_DELETE( TriVariableParameter );

BLUE_DECLARE( Tr2Shader );

class TriVariableParameter : public ITriEffectParameter,
							 public INotify,
							 public IInitialize
{

public:
	EXPOSE_TO_BLUE();

	TriVariableParameter( IRoot* lockobj = NULL );
	~TriVariableParameter();

	using ITriEffectParameter::Lock;
	using ITriEffectParameter::Unlock;

	BlueSharedString m_name;
	BlueSharedString m_variableName;
	TriVariable* m_variable;

	bool m_isUsedByEffect;


	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter
	/////////////////////////////////////////////////////////////////////////////////////
	virtual bool CopyToResourceSet(
		Tr2ResourceSetDescriptionAL& resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex,
		ResourceFlags flags ) const;
	virtual bool ApplyUav(
		Tr2ResourceSetDescriptionAL& resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex ) const;
	virtual void CopyValueToEffect( Tr2RenderContextEnum::ShaderType inputType,
									unsigned char* destHandle,
									size_t size,
									Tr2RenderContext& renderContext ) const;
	const char* GetParameterName() const;
	void RebuildEffectHandles( Tr2Shader* effectRes );
	unsigned GetHashValue( unsigned startingHash ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	/////////////////////////////////////////////////////////////////////////////////////
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	/////////////////////////////////////////////////////////////////////////////////////
	bool Initialize();

private:
	int GetVariableType() const;
	Tr2ShaderPtr m_cachedEffect;
};

TYPEDEF_BLUECLASS( TriVariableParameter );

#endif
