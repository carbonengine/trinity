// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "TriCurveSampler.h"

BLUE_DEFINE_INTERFACE( ITr2SampledCurve );

TriCurveSampler TriCurveSampler::Find( IRoot* source, const Be::VarEntry* valueEntry )
{
	// The curve is looked up once here rather than on every sample. The raw
	// pointer stays valid for as long as source does: the smart pointers only
	// hold their own reference while we look.
	TriCurveSampler sampler;

	if( ITr2SampledCurvePtr keyed = BlueCastPtr( source ) )
	{
		sampler.m_kind = Kind::Keyed;
		sampler.m_curve.keyed = keyed.p;
	}
	else if( valueEntry->mType == Be::FLOAT )
	{
		if( ITriScalarFunctionPtr scalar = BlueCastPtr( source ) )
		{
			sampler.m_kind = Kind::Scalar;
			sampler.m_curve.scalar = scalar.p;
		}
	}
	else if( valueEntry->mType == Be::FLOATARRAY && valueEntry->GetFloatArraySize() == 3 )
	{
		if( ITriVectorFunctionPtr vector = BlueCastPtr( source ) )
		{
			sampler.m_kind = Kind::Vector;
			sampler.m_curve.vector = vector.p;
		}
	}
	else if( valueEntry->mType == Be::FLOATARRAY && valueEntry->GetFloatArraySize() == 4 )
	{
		if( ITriQuaternionFunctionPtr quaternion = BlueCastPtr( source ) )
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

Vector4 TriCurveSampler::Sample( double time )
{
	switch( m_kind )
	{
	case Kind::Keyed:
		return m_curve.keyed->SampleValue( time, m_segments );
	case Kind::Scalar:
		return Vector4( m_curve.scalar->GetValueAt( time ), 0.0f, 0.0f, 0.0f );
	case Kind::Vector:
	{
		Vector3 value;
		m_curve.vector->GetValueAt( &value, time );
		return Vector4( value.x, value.y, value.z, 0.0f );
	}
	case Kind::Quaternion:
	{
		Quaternion value;
		m_curve.quaternion->GetValueAt( &value, time );
		return Vector4( value.x, value.y, value.z, value.w );
	}
	case Kind::Color:
	{
		Color value;
		m_curve.color->GetValueAt( &value, time );
		return Vector4( value.r, value.g, value.b, value.a );
	}
	case Kind::None:
		break;
	}
	return Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
}
