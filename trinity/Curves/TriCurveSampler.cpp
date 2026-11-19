// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "TriCurveSampler.h"
#include "Tr2CurveColor.h"
#include "Tr2CurveEulerRotation.h"
#include "Tr2CurveQuaternion.h"
#include "Tr2CurveScalar.h"
#include "Tr2CurveVector3.h"

TriCurveSampler TriCurveSampler::Find( IRoot* source, const Be::VarEntry* valueEntry )
{
	// The curve is looked up once here rather than on every sample. The raw
	// pointer stays valid for as long as source does: the smart pointers only
	// hold their own reference while we look.
	TriCurveSampler sampler;

	if( valueEntry->mType == Be::FLOAT )
	{
		if( Tr2CurveScalarPtr keyed = BlueCastPtr( source ) )
		{
			sampler.m_kind = Kind::KeyedScalar;
			sampler.m_curve.keyedScalar = keyed.p;
		}
		else if( ITriScalarFunctionPtr scalar = BlueCastPtr( source ) )
		{
			sampler.m_kind = Kind::Scalar;
			sampler.m_curve.scalar = scalar.p;
		}
	}
	else if( valueEntry->mType == Be::FLOATARRAY && valueEntry->GetFloatArraySize() == 3 )
	{
		if( Tr2CurveVector3Ptr keyed = BlueCastPtr( source ) )
		{
			sampler.m_kind = Kind::KeyedVector3;
			sampler.m_curve.keyedVector3 = keyed.p;
		}
		else if( ITriVectorFunctionPtr vector = BlueCastPtr( source ) )
		{
			sampler.m_kind = Kind::Vector;
			sampler.m_curve.vector = vector.p;
		}
	}
	else if( valueEntry->mType == Be::FLOATARRAY && valueEntry->GetFloatArraySize() == 4 )
	{
		if( Tr2CurveQuaternionPtr keyed = BlueCastPtr( source ) )
		{
			sampler.m_kind = Kind::KeyedQuaternion;
			sampler.m_curve.keyedQuaternion = keyed.p;
		}
		else if( Tr2CurveEulerRotationPtr euler = BlueCastPtr( source ) )
		{
			sampler.m_kind = Kind::KeyedEulerRotation;
			sampler.m_curve.keyedEulerRotation = euler.p;
		}
		else if( Tr2CurveColorPtr keyedColor = BlueCastPtr( source ) )
		{
			sampler.m_kind = Kind::KeyedColor;
			sampler.m_curve.keyedColor = keyedColor.p;
		}
		else if( ITriQuaternionFunctionPtr quaternion = BlueCastPtr( source ) )
		{
			sampler.m_kind = Kind::Quaternion;
			sampler.m_curve.quaternion = quaternion.p;
		}
		else if( ITriColorFunctionPtr color = BlueCastPtr( source ) )
		{
			sampler.m_kind = Kind::Color;
			sampler.m_curve.color = color.p;
		}
	}

	return sampler;
}

void TriCurveSampler::Sample( double time, Vector4& out )
{
	switch( m_kind )
	{
	case Kind::KeyedScalar:
		out.x = m_curve.keyedScalar->SampleValue( time, m_segments[0] );
		break;
	case Kind::KeyedQuaternion:
		*reinterpret_cast<Quaternion*>( &out ) = m_curve.keyedQuaternion->SampleValue( time, m_segments[0] );
		break;
	case Kind::KeyedVector3:
		*reinterpret_cast<Vector3*>( &out ) = m_curve.keyedVector3->SampleValue( time, m_segments );
		break;
	case Kind::KeyedColor:
		*reinterpret_cast<Color*>( &out ) = m_curve.keyedColor->SampleValue( time, m_segments );
		break;
	case Kind::KeyedEulerRotation:
		*reinterpret_cast<Quaternion*>( &out ) = m_curve.keyedEulerRotation->SampleValue( time, m_segments );
		break;
	case Kind::Scalar:
		out.x = m_curve.scalar->GetValueAt( time );
		break;
	case Kind::Vector:
		m_curve.vector->GetValueAt( reinterpret_cast<Vector3*>( &out ), time );
		break;
	case Kind::Quaternion:
		m_curve.quaternion->GetValueAt( reinterpret_cast<Quaternion*>( &out ), time );
		break;
	case Kind::Color:
		m_curve.color->GetValueAt( reinterpret_cast<Color*>( &out ), time );
		break;
	case Kind::None:
		break;
	}
}
