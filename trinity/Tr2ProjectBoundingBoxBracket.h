// Copyright © 2011 CCP ehf.

#pragma once

#ifndef Tr2ProjectBoundingBoxBracket_h
#define Tr2ProjectBoundingBoxBracket_h

#include <ITriFunction.h>

BLUE_DECLARE( Tr2ProjectBoundingBoxBracket );
BLUE_DECLARE( Tr2Sprite2dContainer );
BLUE_DECLARE_INTERFACE( ITr2BoundingBox );

class Tr2ProjectBoundingBoxBracket : public ITriFunction
{
public:
	EXPOSE_TO_BLUE();

	Tr2ProjectBoundingBoxBracket( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ITriFunction
	void UpdateValue( double time );

	void SetEmptyProjection();
	void UpdateBracket();

protected:
	void SetFullViewportProjection( const TriViewport& viewport );
	void ConstrainProjection( const Vector3& center, const Matrix& viewProjection, const TriViewport& viewport );
	void PublishProjection( const TriViewport& viewport );

	std::wstring m_name;

	//////////////////////////////////////////////////////////////////////////
	// Inputs - control how UpdateValue works
	ITr2BoundingBoxPtr m_object;

	// Should the coordinates be rounded to the nearest integer? Defaults to true.
	bool m_integerCoordinates;

	// Draw the consumed bounds and the published rect through the debug renderer.
	bool m_debugDraw;

	float m_minProjectedWidth;
	float m_minProjectedHeight;
	float m_maxProjectedWidth;
	float m_maxProjectedHeight;

	// The bracket container
	Tr2Sprite2dContainerPtr m_parent;

	//////////////////////////////////////////////////////////////////////////
	// Outputs - the results from UpdateValue

	Tr2Sprite2dContainerPtr m_bracket;

	float m_projectedX;
	float m_projectedY;
	float m_projectedZ;
	float m_projectedWidth;
	float m_projectedHeight;
	float m_cameraDistance;
	float m_screenMargin;

	bool m_isProjectionValid;
	bool m_containsCamera;
	bool m_extendsOffscreen;
	bool m_coversViewport;
};

TYPEDEF_BLUECLASS( Tr2ProjectBoundingBoxBracket );

#endif
