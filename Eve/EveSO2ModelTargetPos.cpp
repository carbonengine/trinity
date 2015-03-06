#include "StdAfx.h"
#include "EveSO2ModelTargetPos.h"
#include "Vector3d.h"


EveSO2ModelTargetPos::EveSO2ModelTargetPos(IRoot* lockobj):
	m_index( -1 )
{
}

EveSO2ModelTargetPos::~EveSO2ModelTargetPos()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////


Vector3* EveSO2ModelTargetPos::Update( Vector3* in, Be::Time t )
{	
	if( m_parentObject && m_targetObject )
	{
		if ( m_index == -1 )
		{
			Vector3 parentPos;
			m_parentObject->GetValueAt( &parentPos, t );
			m_index = m_targetObject->GetClosestDamageLocatorIndex( &parentPos );
		}

		Vector3 locatorPos;
		m_targetObject->GetDamageLocatorPosition( &locatorPos, m_index );

		in->x = locatorPos.x;
		in->y = locatorPos.y;
		in->z = locatorPos.z;
		
		m_value.x = locatorPos.x;
		m_value.y = locatorPos.y;
		m_value.z = locatorPos.z;
	}
	return in;
}

Vector3* EveSO2ModelTargetPos::Update( Vector3* in, double t )
{
	return in;
}


Vector3* EveSO2ModelTargetPos::GetValueAt( Vector3* in, Be::Time now )
{
	in->x = m_value.x;
	in->y = m_value.y;
	in->z = m_value.z;
	Update( in, now );
	return in;
}



Vector3* EveSO2ModelTargetPos::GetValueAt( Vector3* in, double pos )
{
	in->x = m_value.x;
	in->y = m_value.y;
	in->z = m_value.z;

	return in;
}

Vector3* EveSO2ModelTargetPos::GetValueDoubleDotAt( Vector3* in, Be::Time now )
{
	return in;
}


Vector3* EveSO2ModelTargetPos::GetValueDoubleDotAt( Vector3* in, double pos )
{
	return in;
}


Vector3* EveSO2ModelTargetPos::GetValueDotAt( Vector3* in, Be::Time now )
{
	return in;
}

Vector3* EveSO2ModelTargetPos::GetValueDotAt( Vector3* in, double pos )
{
	return in;
}

Vector3d*  EveSO2ModelTargetPos::InterpolatedPosition( Vector3d* out, Be::Time time )
{
	out->x = m_value.x;
	out->y = m_value.y;
	out->z = m_value.z;

	return out;
}