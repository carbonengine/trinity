/* 
	*************************************************************************************

	TriVariableParameter.h

	Created:	2007-10-04
	Project:	Trinity

	Note: This is a platform independent file and cannot expose any special code that is
	not shared between D3D9 and D3D10.  If any Python methods are to be exposed for
	this class this is the place to do it.

	(c) CCP 2007

	*************************************************************************************
*/

#pragma once
#ifndef TriVariableParameter_H
#define TriVariableParameter_H

#include "include/ITriEffectParameter.h"

class TriVariable;
BLUE_DECLARE( TriVariableParameter );
BLUE_CLASS_ALLOW_DELAYED_DELETE( TriVariableParameter );

BLUE_DECLARE_INTERFACE( ITr2ShaderState );

class TriVariableParameter:
	public ITriEffectParameter,
	public INotify,
	public IInitialize
{

public:
	EXPOSE_TO_BLUE();

	TriVariableParameter(IRoot* lockobj = NULL);
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
	size_t GetValueSize() const;
	void CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
							unsigned char* destHandle, 
							size_t size,
							Tr2RenderContext &renderContext ) const; 
	const char* GetParameterName() const;
	virtual bool IsZeroOrNull( void ) const;
	void RebuildEffectHandles( ITr2ShaderState* effectRes );

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
	ITr2ShaderStatePtr m_cachedEffect;
};

TYPEDEF_BLUECLASS(TriVariableParameter);

#endif 
