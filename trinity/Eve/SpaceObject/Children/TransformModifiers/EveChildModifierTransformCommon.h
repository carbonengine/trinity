// Copyright © 2018 CCP ehf.

#pragma once
#ifndef EveChildTransformCommon_H
#define EveChildTransformCommon_H

#include "StdAfx.h"
#include "include/TriMath.h"
#include "Tr2Renderer.h"

inline void DistanceBase( const Matrix& transform, Matrix& alignMat, float& distCenter, Vector3& d )
{
	const Vector3& myPos = transform.GetTranslation();
	const Vector3& camPos = Tr2Renderer::GetViewPosition();
	d = camPos - myPos;

	Vector3 camFwd = d;
	Matrix parentT = Transpose( transform );
	TriVectorRotateMatrix( &camFwd, &camFwd, &parentT );

	float lengthSq = LengthSq( transform.GetX() );
	if( lengthSq != 0 )
	{
		camFwd.x /= lengthSq;
	}
	else
	{
		camFwd.x = 0;
	}

	lengthSq = LengthSq( transform.GetY() );
	if( lengthSq != 0 )
	{
		camFwd.y /= lengthSq;
	}
	else
	{
		camFwd.y = 0;
	}

	lengthSq = LengthSq( transform.GetZ() );
	if( lengthSq != 0 )
	{
		camFwd.z /= lengthSq;
	}
	else
	{
		camFwd.z = 0;
	}

	distCenter = Length( camFwd );
	camFwd = Normalize( camFwd );

	const Matrix& viewMatrix = Tr2Renderer::GetViewTransform();
	Vector3 right( viewMatrix._11, viewMatrix._21, viewMatrix._31 );
	TriVectorRotateMatrix( &right, &right, &parentT );

	Vector3 up = Normalize( Cross( camFwd, right ) );

	TriMatrixChangeBase( &alignMat, &camFwd, &up );
}

inline Matrix Billboard2D( const Matrix& transform )
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

inline Matrix Billboard3D( const Vector3& position )
{
	const Vector3& camPos = Tr2Renderer::GetViewPosition();

	Vector3 toObject = Normalize( camPos - position );
	Vector3 right = Normalize( Cross( Vector3( 0, 1, 0 ), toObject ) );
	if( LengthSq( right ) == 0.0 )
	{
		right = Vector3( 1, 0, 0 );
	}
	Vector3 up = Normalize( Cross( toObject, right ) );

	return Matrix(
		right.x, right.y, right.z, 0, up.x, up.y, up.z, 0, toObject.x, toObject.y, toObject.z, 0, 0, 0, 0, 1 );
}


#endif