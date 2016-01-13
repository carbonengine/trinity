////////////////////////////////////////////////////////////
//
//    Created:   2015
//    Copyright: CCP 2015
//
#pragma once
#ifndef EveConnector_H
#define EveConnector_H

#include "include/ITriFunction.h"
#include "Eve/EveUpdateContext.h"

BLUE_DECLARE( EveCurveLineSet );
BLUE_DECLARE( EveConnector );
BLUE_DECLARE_VECTOR( EveConnector );

BLUE_CLASS( EveConnector ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveConnector( IRoot* lockobj = NULL );
	~EveConnector();

	void Update( EveUpdateContext& context );
	void AddLine( EveCurveLineSet* lineSet );

private:
	ITriVectorFunctionPtr m_sourceObject;
	ITriVectorFunctionPtr m_destObject;

	Vector3 m_sourcePosition;
	Vector3 m_destPosition;

	Color m_color;
	Color m_animationColor;
	
	float m_animationSpeed;
	float m_animationScale;
	float m_width;

	bool m_isAnimated;
};

TYPEDEF_BLUECLASS( EveConnector );

#endif