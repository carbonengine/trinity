#pragma once
#ifndef Tr2TranslationTool_h
#define Tr2TranslationTool_h
#include "Tr2ManipulationTool.h"

BLUE_CLASS( Tr2TranslationTool ):
     public Tr2ManipulationTool
{
public:
    EXPOSE_TO_BLUE();
    Tr2TranslationTool( IRoot* lockobj = NULL );

	void GenLineSets();
	void Update();
	void ResetPrimitiveColors();
	std::vector<ITr2Renderable*>& GetPrimitivesToRender( );
	void Move( int mouseX, int mouseY, int mouseXDelta, int mouseYDelta, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix );

	Vector3 m_translation;
};

TYPEDEF_BLUECLASS( Tr2TranslationTool );
#endif //Tr2TranslationTool_h
