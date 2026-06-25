// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierBillboard3D.h"
#include "Tr2Renderer.h"
#include "EveChildModifierTransformCommon.h"

EveChildModifierBillboard3D::EveChildModifierBillboard3D( IRoot* lockobj ) :
	m_fixed( false )
{
}

EveChildModifierBillboard3D::~EveChildModifierBillboard3D()
{
}

Matrix EveChildModifierBillboard3D::ApplyTransform( const Matrix& transform, size_t, const Float4x3* ) const
{
	if( m_fixed )
	{
		float scaleX = Length( transform.GetX() );
		float scaleY = Length( transform.GetY() );
		float scaleZ = Length( transform.GetZ() );
		Matrix scale = ScalingMatrix( scaleX, scaleY, scaleZ );
		Matrix invScale = Inverse( scale );
		Matrix transformSansScale = invScale * transform;

		return scale * Billboard3D( transform.GetTranslation() ) * transformSansScale;
	}

	Matrix billboard = Billboard2D( transform );

	Matrix alignMat;
	float distCenter;
	Vector3 d;
	DistanceBase( billboard, alignMat, distCenter, d );

	return alignMat * billboard;
}