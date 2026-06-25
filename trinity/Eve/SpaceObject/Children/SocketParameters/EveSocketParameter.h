// Copyright © 2019 CCP ehf.

#pragma once
#ifndef EveSocketParameter_H
#define EveSocketParameter_H

#include "IEveSocketParameter.h"
#include "include/ITr2ValueBinding.h"


BLUE_DECLARE_IVECTOR( ITr2ValueBinding );
BLUE_DECLARE_VECTOR( Tr2ExternalParameter );


BLUE_CLASS( EveSocketParameterBindingBase ) :
	public IEveSocketParameter
{
public:
	EXPOSE_TO_BLUE();

	EveSocketParameterBindingBase( IRoot* lockobj = nullptr );
	~EveSocketParameterBindingBase();

	virtual const char* GetName() const
	{
		return m_name.c_str();
	}
	virtual void SetName( const char* name )
	{
		m_name = name;
	}

	virtual void ClearBindings();
	virtual bool BindToExternalParameter( Tr2ExternalParameter & externalParameter );
	bool Used() const;

	virtual void Propagate();

protected:
	virtual bool ExtractDefault( const Tr2ExternalParameter& externalParameter ) = 0;

	std::string m_name;
	PITr2ValueBindingVector m_bindings;
};

TYPEDEF_BLUECLASS( EveSocketParameterBindingBase );


#define SOCKET_PARAM_DECLARE( _className, _valueType )                                \
	BLUE_CLASS( _className ) :                                                        \
		public EveSocketParameterBindingBase                                          \
	{                                                                                 \
	public:                                                                           \
		EXPOSE_TO_BLUE();                                                             \
		_className( IRoot* lockobj = nullptr );                                       \
		~_className();                                                                \
		virtual void ClearBindings();                                                 \
		virtual void Reset();                                                         \
		virtual void SetValueToDefault();                                             \
                                                                                      \
	protected:                                                                        \
		virtual bool ExtractDefault( const Tr2ExternalParameter& externalParameter ); \
                                                                                      \
	private:                                                                          \
		_valueType m_value;                                                           \
		std::vector<_valueType> m_defaults;                                           \
	};                                                                                \
	TYPEDEF_BLUECLASS( _className );

SOCKET_PARAM_DECLARE( EveSocketParameterBool, bool );
SOCKET_PARAM_DECLARE( EveSocketParameterInt, int32_t );
SOCKET_PARAM_DECLARE( EveSocketParameterFloat, float );
SOCKET_PARAM_DECLARE( EveSocketParameterVector2, Vector2 );
SOCKET_PARAM_DECLARE( EveSocketParameterVector3, Vector3 );
SOCKET_PARAM_DECLARE( EveSocketParameterVector4, Vector4 );
SOCKET_PARAM_DECLARE( EveSocketParameterColor, Color );


BLUE_CLASS( EveSocketParameterString ) :
	public IEveSocketParameter
{
public:
	EXPOSE_TO_BLUE();

	EveSocketParameterString( IRoot* lockobj = nullptr );
	~EveSocketParameterString();

	bool Initialize();

	const char* GetName() const
	{
		return m_name.c_str();
	}
	void SetName( const char* name )
	{
		m_name = name;
	}

	void ClearBindings();
	bool BindToExternalParameter( Tr2ExternalParameter & externalParameter );
	void SetValueToDefault();
	bool Used() const;

	void Propagate();

protected:
	virtual bool ExtractDefault( const Tr2ExternalParameter& externalParameter );

	std::string m_name;
	std::string m_value;
	Tr2ExternalParameterPtr m_valueExposure;
	PTr2ExternalParameterVector m_externalParameters;
	std::vector<std::string> m_defaults;
};

TYPEDEF_BLUECLASS( EveSocketParameterString );

BLUE_CLASS( EveSocketParameterFilePath ) :
	public EveSocketParameterString
{
public:
	EXPOSE_TO_BLUE();
};

TYPEDEF_BLUECLASS( EveSocketParameterFilePath );

#endif