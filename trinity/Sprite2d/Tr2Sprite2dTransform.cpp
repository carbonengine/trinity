#include "StdAfx.h"
#include "Tr2Sprite2dTransform.h"
#include "Tr2Sprite2dScene.h"
#include "Tr2Sprite2dPickingMask.h"

Tr2Sprite2dTransform::Tr2Sprite2dTransform( IRoot* lockobj ) :
	Tr2Sprite2dContainerBase( lockobj ),
	m_rotationCenter( 0.0f, 0.0f ),
	m_rotation( 0.0f ),
	m_scalingCenter( 0.0f, 0.0f ),
	m_scalingRotation( 0.0f ),
	m_scale( 1.0f, 1.0f ),
	m_depthMin( 0.0f ),
	m_depthMax( 0.0f )
{
}

void Tr2Sprite2dTransform::GatherSprites( Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return;
	}

	renderer->PushTransform( GetTransformationMatrix() );
	float oldOpacity = renderer->GetAccumulatedAlpha();
	renderer->SetAccumulatedAlpha( m_opacity * oldOpacity );

	for( ITr2SpriteObjectVector::reverse_iterator it = m_background.rbegin(); it != m_background.rend(); ++it )
	{
		(*it)->GatherSprites( renderer );
	}
	for( ITr2SpriteObjectVector::reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it )
	{
		(*it)->GatherSprites( renderer );
	}

	renderer->SetAccumulatedAlpha( oldOpacity );
	renderer->PopTransform();
}

ITr2SpriteObject* Tr2Sprite2dTransform::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return NULL;
	}
	if( m_pickState == TR2_SPS_OFF )
	{
		return NULL;
	}

	// Check that we're within the bounding box of the transform object itself
	renderer->PushTranslation( m_translation );
	bool isInside = renderer->IsInside( Vector2( x, y ), Vector2( 0.0f, 0.0f ), m_displayWidth, m_displayHeight, 0.0 );
	if( isInside && m_pickingMask )
	{
		isInside = m_pickingMask->SampleMask( renderer->InverseTransformPoint( Vector2( x, y ) ), Vector2( 0.0f, 0.0f ), m_displayWidth, m_displayHeight );
	}
	renderer->PopTranslation();
	if( !isInside )
	{
		return NULL;
	}
	
	ITr2SpriteObject* obj = NULL;
	renderer->PushTransform( GetTransformationMatrix() );

	for( ITr2SpriteObjectVector::iterator it = m_children.begin(); it != m_children.end(); ++it )
	{
		obj = (*it)->PickPoint( x, y, renderer );
		if( obj )
		{
			break;
		}
	}

	// If transform itself is pickable and no child was found
	if( m_pickState == TR2_SPS_ON )
	{
		this->m_auxMouseover = NULL;
		if ( !obj )
		{
			obj = this;
		}
		else
		{
			if ( obj->IsAuxMouseover() )
			{
				// The auxiliary mouseover can add content to the mouseover text or context menu
				this->m_auxMouseover = obj;
				obj = this;
			}
		}
	}
	renderer->PopTransform();

	return obj;
}

Matrix Tr2Sprite2dTransform::GetTransformationMatrix()
{
	Matrix m;
	// Rounding the results here helps with ensuring pixel perfect sprites with 90 degree rotations
	Vector2 absScalingCenter( floor( m_scalingCenter.x * m_displayWidth + 0.5f ), floor( m_scalingCenter.y * m_displayHeight + 0.5f ) );
	Vector2 absRotationCenter( floor( m_rotationCenter.x * m_displayWidth + 0.5f ), floor( m_rotationCenter.y * m_displayHeight + 0.5f ) );
	m = Transformation2DMatrix( &absScalingCenter, m_scalingRotation, &m_scale, &absRotationCenter, m_rotation, &m_translation );
	return m;
}

Vector2 Tr2Sprite2dTransform::TransformPoint(float x, float y)
{
	Matrix m;
	// Rounding the results here helps with ensuring pixel perfect sprites with 90 degree rotations
	Vector2 absScalingCenter( floor( m_scalingCenter.x * m_displayWidth + 0.5f ), floor( m_scalingCenter.y * m_displayHeight + 0.5f ) );
	Vector2 absRotationCenter( floor( m_rotationCenter.x * m_displayWidth + 0.5f ), floor( m_rotationCenter.y * m_displayHeight + 0.5f ) );
	m = Transformation2DMatrix( &absScalingCenter, m_scalingRotation, &m_scale, &absRotationCenter, m_rotation, NULL );

	Vector2 vec = Vector2(x, y);
	Vector4 ret = Transform( vec, m );
	return Vector2(ret.x, ret.y);
}
