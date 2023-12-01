#pragma once

#ifndef Tr2Sprite2dTransform_h
#define Tr2Sprite2dTransform_h


#include "Tr2Sprite2dContainer.h"

BLUE_DECLARE( Tr2Sprite2dTransform );
BLUE_DECLARE_IVECTOR( ITr2SpriteObject );

class Tr2Sprite2dTransform:
     public Tr2Sprite2dContainerBase
{
public:
    EXPOSE_TO_BLUE();
    Tr2Sprite2dTransform( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	void GatherSprites( Tr2Sprite2dScene* renderer );
	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );

	Vector2 TransformPoint(float x, float y);

private:
	// Center of rotation as relative coordinates - (0,0) is top-left, (0.5,0.5) is
	// center.
	Vector2 m_rotationCenter;
	float m_rotation;

	// Center of scaling, again as relative coordinates.
	Vector2 m_scalingCenter;
	float m_scalingRotation;
	Vector2 m_scale;
	float m_depthMin;
	float m_depthMax;

	Matrix GetTransformationMatrix();
};

TYPEDEF_BLUECLASS( Tr2Sprite2dTransform );
#endif //Tr2Sprite2dTransform_h
