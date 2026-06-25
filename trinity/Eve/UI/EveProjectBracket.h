// Copyright © 2010 CCP ehf.

#pragma once

#ifndef EveProjectBracket_h
#define EveProjectBracket_h

#include "IWorldPosition.h"

#include <ITriFunction.h>

BLUE_DECLARE( EveProjectBracket );
BLUE_DECLARE( Tr2Sprite2dContainer );
BLUE_DECLARE( EveSprite2dBracket );
BLUE_DECLARE_INTERFACE( IWorldPosition );

class EveProjectBracket : public ITriFunction
{
public:
	EXPOSE_TO_BLUE();

	EveProjectBracket( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ITriFunction
	void UpdateValue( double time );

	void SetBracketDisplayState( bool state );

protected:
	std::wstring m_name;

	//////////////////////////////////////////////////////////////////////////
	// Inputs - control how UpdateValue works

	// The source position as a destiny ball
	ITriVectorFunctionPtr m_trackBall;
	float m_ballTrackingScaling;

	// The source position in 3D space
	IWorldPositionPtr m_trackTransform;

	// Fixed position, used if neither trackball or transform are set
	Vector3 m_trackPosition;

	// Should the bracket dock on the sides if source is off the screen?
	// If not, the visible flag is toggled depending on off-screen status
	bool m_dock;
	bool m_isVisible;
	bool m_isInFront;

	// Has the visible state been set? Used to ensure that callback gets issued
	// the first time this is set.
	bool m_isVisibleStateSet;

	// Should the coordinates be rounded to the nearest integer? Defaults to true.
	bool m_integerCoordinates;

	float m_marginLeft;
	float m_marginRight;
	float m_marginTop;
	float m_marginBottom;

	// Bracket is hidden if it lies outside this display range
	float m_minDispRange;
	float m_maxDispRange;

	// Distance to camera in last projection
	float m_cameraDistance;

	// Extra offset that can be used to correct subpixel issues, or just to move
	// things around without Transform headaches.
	float m_offsetX;
	float m_offsetY;

	// The bracket container
	Tr2Sprite2dContainerPtr m_parent;

	// An optional callback to call when the display state is changed
	BlueScriptCallback m_displayChangeCallback;

	// An optional callback to call when projectBracket is updated
	BlueScriptCallback m_bracketUpdateCallback;

	//////////////////////////////////////////////////////////////////////////
	// Outputs - the results from UpdateValue

	Tr2Sprite2dContainerPtr m_bracket;

	// The bracket icon
	EveSprite2dBracketPtr m_bracketIcon;

	Vector2 m_projectedPosition;
	Vector2 m_rawProjectedPosition;
};

TYPEDEF_BLUECLASS( EveProjectBracket );

#endif
