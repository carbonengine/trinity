////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#pragma once
#ifndef Tr2PPEffect_H
#define Tr2PPEffect_H


BLUE_CLASS( Tr2PPEffect ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPEffect( IRoot* lockobj = NULL );
	~Tr2PPEffect();

	virtual bool IsActive();

protected:
	bool m_display;

};

TYPEDEF_BLUECLASS( Tr2PPEffect );

#endif