////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#pragma once
#ifndef Tr2PPEffect_H
#define Tr2PPEffect_H


BLUE_CLASS( Tr2PPEffect ) :
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPEffect( IRoot* lockobj = NULL );
	~Tr2PPEffect();

	// INotify
	virtual bool OnModified( Be::Var* value );

	virtual bool IsActive();
	bool IsDirty();
	void SetDirty( bool dirty );

protected:
	bool m_display;
	bool m_isDirty;

};

TYPEDEF_BLUECLASS( Tr2PPEffect );

#endif