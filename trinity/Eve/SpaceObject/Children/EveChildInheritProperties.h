////////////////////////////////////////////////////////////
//
//    Created:   September 2018
//    Copyright: CCP 2018
//
#pragma once
#ifndef EveChildInheritProperties_H
#define EveChildInheritProperties_H

#include "Eve/SpaceObjectFactory/EveSOFData.h"

BLUE_CLASS( EveChildInheritProperties ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveChildInheritProperties( IRoot* lockobj = NULL );
	~EveChildInheritProperties();

	void SetProperties( const Color *colorSet );
    const Color* GetProperties();
protected:
	Color m_colorSet[SOFDataFactionColorChooser::TYPE_MAX];
};

TYPEDEF_BLUECLASS( EveChildInheritProperties );

#endif