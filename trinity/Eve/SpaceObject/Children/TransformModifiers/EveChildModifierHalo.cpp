////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#include "StdAfx.h"
#include "EveChildModifierHalo.h"
#include "Tr2Renderer.h"
#include "include/TriMath.h"

EveChildModifierHalo::EveChildModifierHalo( IRoot* lockobj )
{
}

EveChildModifierHalo::~EveChildModifierHalo()
{
}

Matrix EveChildModifierHalo::ApplyTransform( const Matrix& transform, size_t, const granny_matrix_3x4* ) const
{
	float parentScaleX = Length( transform.GetX() );
	float parentScaleY = Length( transform.GetY() );
	float parentScaleZ = Length( transform.GetZ() );
	Vector3 finalScale( parentScaleX, parentScaleY, parentScaleZ );

	const Vector3& myPos = transform.GetTranslation();
	const Vector3& camPos = Tr2Renderer::GetViewPosition();
	Vector3 d = camPos - myPos;

	Vector3 backward;

	float scale = Dot(
		Normalize( d ),
		Normalize( *TriVectorRotatedBasisMatrix( &backward, TRITA_Z, &transform ) )
	);

	if (scale < 0.0f)
	{
		scale = 0.0f;
	}

	finalScale *= scale*scale;

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