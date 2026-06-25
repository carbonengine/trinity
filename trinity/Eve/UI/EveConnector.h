// Copyright © 2016 CCP ehf.

#pragma once
#ifndef EveConnector_H
#define EveConnector_H

#include "Eve/EveUpdateContext.h"

#include <ITriFunction.h>

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

	enum ConnectorType
	{
		PointToPoint,
		XZ_CircleStraight,
		XZ_Circle,
		StraightAnchor,
		CurvedAnchor,
		Orbit,
		Circle,
		Ellipse
	};

	void Update( const EveUpdateContext& context );
	void AddLine( EveCurveLineSet * lineSet );

private:
	ITriVectorFunctionPtr m_sourceObject;
	ITriVectorFunctionPtr m_destObject;

	ConnectorType m_type;

	Vector3 m_sourcePosition;
	Vector3 m_destPosition;
	Vector3 m_normal;

	Color m_color;
	Color m_animationColor;

	float m_animationSpeed;
	float m_animationScale;
	float m_width;
	float m_lineLength;
	float m_length;

	bool m_isAnimated;
	bool m_autoScaleAnimation;

	// Some helper functions(inline)
	void AnimateSegment( EveCurveLineSet * lineSet, int lineID );
	void AddCircle( EveCurveLineSet * lineSet, const Vector3& center, float radius );
	void AddCircle( EveCurveLineSet * lineSet, const Vector3& center, float radius, const Vector3& normal );
	void AddEllipse( EveCurveLineSet * lineSet, const Vector3& center, float radiusX, float radiusY, float rotation, const Vector3& normal );
	void AddOrbit( EveCurveLineSet * lineSet, const Vector3& center, float radius, const Vector3& normal );
	void AddStraightLine( EveCurveLineSet * lineSet, const Vector3& source, const Vector3& destination, bool fadeEnd = false );
	void AddSpheredSegment( EveCurveLineSet * lineSet, const Vector3& p0, const Vector3& p1, const Vector3& center );
	void AddCurvedLine( EveCurveLineSet * lineSet, const Vector3& p0, const Vector3& p1, const Vector3& center, int segments );

	void CalculateSideAndFront( const Vector3& upDir, Vector3& outSide, Vector3& outFront );
};

TYPEDEF_BLUECLASS( EveConnector );

#endif