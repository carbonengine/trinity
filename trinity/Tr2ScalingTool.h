// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2ScalingTool_h
#define Tr2ScalingTool_h

#include "Tr2ManipulationTool.h"

BLUE_DECLARE( Tr2LineSet );
BLUE_DECLARE( Tr2SolidSet );

BLUE_CLASS( Tr2ScalingTool ) :
	public Tr2ManipulationTool
{
public:
	EXPOSE_TO_BLUE();
	Tr2ScalingTool( IRoot* lockobj = NULL );

	void GenLineSets();
	void Update();
	void ResetPrimitiveColors();
	void ResetPrimitives();
	std::vector<ITr2Renderable*>& GetPrimitivesToRender();
	void Move( int mouseX, int mouseY, int mouseXDelta, int mouseYDelta, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix );

	Vector3 m_scale;
	Vector3 m_initialScale;
	float m_initialLength;
	void Init( Matrix & initialTransform );

	Tr2SolidSetPtr m_xBox;
	Tr2SolidSetPtr m_yBox;
	Tr2SolidSetPtr m_zBox;
	Tr2SolidSetPtr m_wBox;
	Tr2LineSetPtr m_xLine;
	Tr2LineSetPtr m_yLine;
	Tr2LineSetPtr m_zLine;

	void UpdateLines();
};

TYPEDEF_BLUECLASS( Tr2ScalingTool );
#endif //Tr2ScalingTool_h
