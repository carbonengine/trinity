#include "StdAfx.h"
#include "Tr2QuaternionLerpCurve.h"
#include "include/TriMath.h"


Tr2QuaternionLerpCurve::Tr2QuaternionLerpCurve(IRoot* lockobj) :
    m_start( 0 ),
	m_length( 0 ),
    m_value( 0.0f, 0.0f, 0.0f, 1.0f )
{
}


Tr2QuaternionLerpCurve::~Tr2QuaternionLerpCurve()
{
}


Quaternion* Tr2QuaternionLerpCurve::Update(
    Quaternion* in,
    Be::Time t
    )
{
    GetValueAt( &m_value, t );
    *in = m_value;
    return in;
}


Quaternion* Tr2QuaternionLerpCurve::Update(
    Quaternion* in,
    double t
    )
{
    GetValueAt( &m_value, t );
    *in = m_value;
    return in;
}


Quaternion* Tr2QuaternionLerpCurve::GetValueAt(
    Quaternion* in,
    Be::Time now 
    )
{
    if( !m_startCurve || !m_endCurve || m_length <= 0 )
    {
        return in;
    }

    Quaternion start;
    Quaternion end;

    float delta = TimeAsFloat( now - m_start );
    float ratio = TriClamp( delta / m_length, 0.0f, 1.0f );

    *in = Slerp( *m_startCurve->GetValueAt( &start, now ), *m_endCurve->GetValueAt( &end, now ), ratio );

    return in;
}


Quaternion* Tr2QuaternionLerpCurve::GetValueAt(
    Quaternion* in,
    double pos
    )
{
    if( !m_startCurve || !m_endCurve || m_length <= 0 )
    {
        return in;
    }

    Quaternion start;
    Quaternion end;

    float delta = float( pos - TimeAsDouble( m_start ) );
	float ratio = TriClamp( delta / m_length, 0.0f, 1.0f );

    *in = Slerp( *m_startCurve->GetValueAt( &start, pos ), *m_endCurve->GetValueAt( &end, pos ), ratio );

    return in;
}


Quaternion* Tr2QuaternionLerpCurve::GetValueDotAt(
    Quaternion* in,
    Be::Time time
    )
{
    return in;
}


Quaternion* Tr2QuaternionLerpCurve::GetValueDotAt(
    Quaternion* in,
    double time
    )
{
    return in;
}


Quaternion* Tr2QuaternionLerpCurve::GetValueDoubleDotAt(
    Quaternion* in,
    Be::Time time
    )
{
    return in;
}


Quaternion* Tr2QuaternionLerpCurve::GetValueDoubleDotAt(
    Quaternion* in,
    double time
    )
{
    return in;
}
