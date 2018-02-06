////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#include "StdAfx.h"
#include "EveChildModifierBillboard2D.h"
#include "Tr2Renderer.h"

EveChildModifierBillboard2D::EveChildModifierBillboard2D( IRoot* lockobj )
{
}

EveChildModifierBillboard2D::~EveChildModifierBillboard2D()
{
}

Matrix EveChildModifierBillboard2D::ApplyTransform( const Matrix& transform ) const
{
	float parentScaleX = Length( transform.GetX() );
	float parentScaleY = Length( transform.GetY() );
	float parentScaleZ = Length( transform.GetZ() );
	Vector3 finalScale( parentScaleX, parentScaleY, parentScaleZ );

	const Matrix& invView = Tr2Renderer::GetInverseViewTransform();

	Matrix result = transform;
	result._11 = invView._11 * finalScale.x;
	result._12 = invView._12 * finalScale.x;
	result._13 = invView._13 * finalScale.x;
	result._21 = invView._21 * finalScale.y;
	result._22 = invView._22 * finalScale.y;
	result._23 = invView._23 * finalScale.y;
	result._31 = invView._31 * finalScale.z;
	result._32 = invView._32 * finalScale.z;
	result._33 = invView._33 * finalScale.z;

	return result;
}