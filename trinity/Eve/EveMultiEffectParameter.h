////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//    Copyright: CCP 2019
//

#pragma once

#ifndef EveMultiEffectParameter_h
#define EveMultiEffectParameter_h

#include "EveMultiEffect.h"
BLUE_DECLARE( EveMultiEffect );

BLUE_CLASS( EveMultiEffectParameter ) :
	public INotify
{
public:
	// parameter type
	enum ParameterType
	{
		TYPE_EVESPACEOBJECT = 0,
		TYPE_EVEEFFECTROOT,
		TYPE_ANYTHING,
		TYPE_UNDEFINED
	};

	EXPOSE_TO_BLUE();
	EveMultiEffectParameter( IRoot* lockobj = NULL );

	void SetParameterObject( IRoot* object );
	bool IsValid();
	void SetOwner( EveMultiEffect* owner );

	IRoot* GetParameterObject();
	BlueSharedString GetName();
	
	//////////////////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* value );

private:
	BlueSharedString m_name;
	IRootPtr m_object;
	EveMultiEffect* m_owner;
	ParameterType m_type;

};
TYPEDEF_BLUECLASS( EveMultiEffectParameter );

#endif // EveMultiEffectParameter_h
