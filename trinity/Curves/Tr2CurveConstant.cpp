////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveConstant.h"
#include "Utilities/Vector3d.h"


// --------------------------------------------------------------------------------
Tr2CurveConstant::Tr2CurveConstant( IRoot* )
	:m_value( 0, 0, 0, 0 )
{
}

// --------------------------------------------------------------------------------
void Tr2CurveConstant::UpdateValue( double )
{
}

// --------------------------------------------------------------------------------
float Tr2CurveConstant::Update( Be::Time )
{
	return m_value.x;
}

// --------------------------------------------------------------------------------
float Tr2CurveConstant::Update( double )
{
	return m_value.x;
}

// --------------------------------------------------------------------------------
float Tr2CurveConstant::GetValueAt( Be::Time )
{
	return m_value.x;
}

// --------------------------------------------------------------------------------
float Tr2CurveConstant::GetValueAt( double )
{
	return m_value.x;
}

// --------------------------------------------------------------------------------
void Tr2CurveConstant::ScaleTime( float )
{
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveConstant::Update( Vector3* in, Be::Time )
{
	*in = *reinterpret_cast<Vector3*>( &m_value );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveConstant::Update( Vector3* in, double )
{
	*in = *reinterpret_cast<Vector3*>( &m_value );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveConstant::GetValueAt( Vector3* in, Be::Time )
{
	*in = *reinterpret_cast<Vector3*>( &m_value );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveConstant::GetValueAt( Vector3* in, double )
{
	*in = *reinterpret_cast<Vector3*>( &m_value );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveConstant::GetValueDotAt( Vector3* in, Be::Time )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveConstant::GetValueDotAt( Vector3* in, double )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveConstant::GetValueDoubleDotAt( Vector3* in, Be::Time )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveConstant::GetValueDoubleDotAt( Vector3* in, double )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}

// --------------------------------------------------------------------------------
Vector3d* Tr2CurveConstant::InterpolatedPosition( Vector3d* out, Be::Time )
{
	out->x = m_value.x;
	out->y = m_value.y;
	out->z = m_value.z;
	return out;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveConstant::Update( Quaternion* in, Be::Time )
{
	*in = *reinterpret_cast<Quaternion*>( &m_value );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveConstant::Update( Quaternion* in, double )
{
	*in = *reinterpret_cast<Quaternion*>( &m_value );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveConstant::GetValueAt( Quaternion* in, Be::Time )
{
	*in = *reinterpret_cast<Quaternion*>( &m_value );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveConstant::GetValueAt( Quaternion* in, double )
{
	*in = *reinterpret_cast<Quaternion*>( &m_value );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveConstant::GetValueDotAt( Quaternion* in, Be::Time )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveConstant::GetValueDotAt( Quaternion* in, double )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveConstant::GetValueDoubleDotAt( Quaternion* in, Be::Time )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveConstant::GetValueDoubleDotAt( Quaternion* in, double time )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
Color* Tr2CurveConstant::Update( Color* in, Be::Time )
{
	*in = *reinterpret_cast<Color*>( &m_value );
	return in;
}

// --------------------------------------------------------------------------------
Color* Tr2CurveConstant::Update( Color* in, double )
{
	*in = *reinterpret_cast<Color*>( &m_value );
	return in;
}

// --------------------------------------------------------------------------------
Color* Tr2CurveConstant::GetValueAt( Color* in, Be::Time )
{
	*in = *reinterpret_cast<Color*>( &m_value );
	return in;
}

// --------------------------------------------------------------------------------
Color* Tr2CurveConstant::GetValueAt( Color* in, double )
{
	*in = *reinterpret_cast<Color*>( &m_value );
	return in;
}
