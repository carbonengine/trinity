// Copyright © 2019 CCP ehf.

#pragma once
#ifndef Tr2PPEffect_H
#define Tr2PPEffect_H

namespace PostProcess
{
enum Quality
{
	LOW,
	MEDIUM,
	HIGH,

	COUNT
};

extern const Be::VarChooser PostProcessQualityChooser[];
}


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