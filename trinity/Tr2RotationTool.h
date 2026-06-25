// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2RotationTool_h
#define Tr2RotationTool_h
#include "Tr2ManipulationTool.h"

BLUE_DECLARE( Tr2LineSet );

BLUE_CLASS( Tr2RotationTool ) :
	public Tr2ManipulationTool
{
public:
	EXPOSE_TO_BLUE();
	Tr2RotationTool( IRoot* lockobj = NULL );
	void GenLineSets();
	void Update();
	void ResetPrimitiveColors();
	std::vector<ITr2Renderable*>& GetPrimitivesToRender();
	void Move( int mouseX, int mouseY, int mouseXDelta, int mouseYDelta, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix );
	void GetUnTransformedBaseVectors( Vector3 & x, Vector3 & y, Vector3 & z );
	Vector3 Hemisphere( int mouseX, int mouseY, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix ) const;
	Vector3 GetDesiredPlaneNormal( Vector3 & ray, Matrix & viewMatrix );

	float m_angle;
	float m_precision;
	Quaternion m_rotation;

	Tr2LineSetPtr m_xLine;
	Tr2LineSetPtr m_yLine;
	Tr2LineSetPtr m_zLine;
	Tr2LineSetPtr m_wLine;
	Tr2LineSetPtr m_wwLine;
};

TYPEDEF_BLUECLASS( Tr2RotationTool );
#endif //Tr2RotationTool_h
