// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveLocalPositionCurve.h"
#include "Utilities/Vector3d.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/SpaceObject/EveMobile.h"
#include "Eve/Turret/EveTurretSet.h"
#include "include/TriMath.h"


EveLocalPositionCurve::EveLocalPositionCurve( IRoot* lockobj ) :
	m_value( 0.f, 0.f, 0.f ),
	m_boundingBoxSize( 0.f, 0.f, 0.f ),
	m_positionOffset( 0.f, 0.f, 0.f ),
	m_damageLocatorIndex( -1 ),
	m_locatorIndex( -1 ),
	m_impactEffectIndex( -1 ),
	m_impactSize( 1.f ),
	m_behavior( POS_NONE ),
	m_offset( 0.f ),
	m_muzzleIndex( 0 )
{
}

EveLocalPositionCurve::~EveLocalPositionCurve()
{
}

Vector3* EveLocalPositionCurve::CalculateOffsetPlaneRotation( Vector3* in, Be::Time t )
{
	Vector3 parentPosition( 0.f, 0.f, 0.f );
	Vector3 offsetPosition;
	Vector3 normal( 0.f, 1.f, 0.f );
	if( m_parentPositionCurve )
	{
		m_parentPositionCurve->GetValueAt( &parentPosition, t );
	}

	if( m_alignPositionCurve )
	{
		m_alignPositionCurve->GetValueAt( &offsetPosition, t );
	}
	else
	{
		offsetPosition = m_positionOffset;
	}

	*in = TriVectorRotateToPlane( offsetPosition, parentPosition, normal );
	return in;
}

Vector3* EveLocalPositionCurve::CalculateOffsetPosition( Vector3* in, Be::Time t )
{
	Vector3 offset = m_positionOffset;
	if( m_parentRotationCurve )
	{
		Quaternion rotation;
		m_parentRotationCurve->GetValueAt( &rotation, t );
		TriVectorRotateQuaternion( &offset, &offset, &rotation );
	}
	if( !m_parentPositionCurve )
	{
		*in = offset;
	}
	else
	{
		Vector3 parentPos;
		m_parentPositionCurve->GetValueAt( &parentPos, t );
		*in = parentPos + offset;
	}
	return in;
}

Vector3* EveLocalPositionCurve::CalculateNearestBoundingPoint( Vector3* in, Be::Time t )
{
	if( m_parentPositionCurve && m_alignPositionCurve && m_parentRotationCurve )
	{

		Vector3 pt;
		Vector3 at;
		Quaternion parentRotation;

		m_parentPositionCurve->GetValueAt( &pt, t );
		m_alignPositionCurve->GetValueAt( &at, t );
		m_parentRotationCurve->GetValueAt( &parentRotation, t );

		Vector3 dir = Normalize( at - pt );

		/*	--------------------------------------------------------------------------------------------------
		*	What we actually want to calculate is the point of intersection with the line
		*	defined by the direction vector, but what we definitely don't need is the direction that
		*	this point lies on (we have it already). We want to rotate the direction vector into the coordinate
		*	system of the parent transform, then solve the question of how much it needs to be scaled by to reach
		*	the bounding sphere, then perform this on the un-transformed vector. (which, like the object itself, is pre-transformed)
		*	-------------------------------------------------------------------------------------------------- */

		Matrix matInv;
		parentRotation = Inverse( Normalize( parentRotation ) );
		matInv = RotationMatrix( parentRotation );

		Vector3 transformedDir = TransformCoord( dir, matInv );

		// Dir is now transformed into the direction in relation to the rotation of the ship
		// We can now compute the scaling that's required on the vector using a standard ellipsiod equation

		float scalingValue = m_offset;

		// if the object is really small (or the bounding size is erroring), just use the center of it, rather than
		// giving a NaN result in this formula
		if( ( m_boundingBoxSize.x > 10.0 ) && ( m_boundingBoxSize.y > 10.0 ) && ( m_boundingBoxSize.z > 10.0 ) )
		{
			// Forumula for an ellipsiod is
			// 1 = (x^2 / a^2) + (y^2 / b^2) + (z^2 / c^2)
			// now we need to solve the question, how much do we need to scale the vector
			// (all three axes equally) for the vector to satisfy the ellipsiod equation
			// solution to 1 = (x*theta)^2/a^2 + ....
			// (always want the positive answer)
			scalingValue += fabs(
				( m_boundingBoxSize.x * m_boundingBoxSize.y * m_boundingBoxSize.z ) /
				sqrt(
					( transformedDir.x * transformedDir.x ) * ( m_boundingBoxSize.y * m_boundingBoxSize.y ) * ( m_boundingBoxSize.z * m_boundingBoxSize.z ) +
					( transformedDir.y * transformedDir.y ) * ( m_boundingBoxSize.x * m_boundingBoxSize.x ) * ( m_boundingBoxSize.z * m_boundingBoxSize.z ) +
					( transformedDir.z * transformedDir.z ) * ( m_boundingBoxSize.x * m_boundingBoxSize.x ) * ( m_boundingBoxSize.y * m_boundingBoxSize.y ) ) );
		}

		// Apply the scaling to the original direction vector
		in->x = pt.x + ( dir.x * scalingValue );
		in->y = pt.y + ( dir.y * scalingValue );
		in->z = pt.z + ( dir.z * scalingValue );
	}
	else if( m_parentPositionCurve )
	{
		Vector3 pt;
		m_parentPositionCurve->GetValueAt( &pt, t );
		// If we don't have an align transform, just put it at the center of the Parent
		in->x = pt.x;
		in->y = pt.y;
		in->z = pt.z;
	}
	return in;
}

Vector3* EveLocalPositionCurve::GetCenterBoundingSphere( Vector3* in, Be::Time t )
{
	if( m_parentObject )
	{
		Vector3 tr;
		m_parentObject->UpdateModelCenterWorldPosition( tr, t );

		in->x = tr.x;
		in->y = tr.y;
		in->z = tr.z;
	}
	return in;
}

Vector3* EveLocalPositionCurve::GetDamageLocator( Vector3* in, Be::Time t )
{
	if( m_alignPositionCurve && m_parentObject )
	{
		ITriTargetable* target = dynamic_cast<ITriTargetable*>( m_parentObject.p );
		if( !target )
		{
			CCP_LOGERR( "Parent object is not targetable. Unable to get valid damage locators." );
			return in;
		}

		if( m_damageLocatorIndex == -1 )
		{
			Vector3 parentPos;
			m_alignPositionCurve->GetValueAt( &parentPos, t );
			m_damageLocatorIndex = target->GetGoodDamageLocatorIndex( parentPos );
		}

		Vector3 locatorPosWS;
		target->GetDamageLocatorPosition( &locatorPosWS, m_damageLocatorIndex, true );

		in->x = locatorPosWS.x;
		in->y = locatorPosWS.y;
		in->z = locatorPosWS.z;
	}
	return in;
}

Vector3* EveLocalPositionCurve::GetNearestFiringLocator( Vector3* in, Be::Time t )
{
	if( m_parentObject && m_locatorIndex != -1 && !m_locatorSetName.empty() )
	{
		if( EveSpaceObject2* target = dynamic_cast<EveSpaceObject2*>( m_parentObject.p ) )
		{
			Vector3 locatorPos( 0, 0, 0 );
			target->GetLocatorPosition( &locatorPos, m_locatorIndex, true, m_locatorSetName );

			in->x = locatorPos.x;
			in->y = locatorPos.y;
			in->y = locatorPos.y;
			in->z = locatorPos.z;
		}
	}
	return in;
}

// --------------------------------------------------------------------------------
// Description:
//   Calculate impact position onto the target aiming for a damage locator
// --------------------------------------------------------------------------------
Vector3* EveLocalPositionCurve::GetDamageLocatorImpact( Vector3* in, Be::Time t )
{
	if( m_alignPositionCurve && m_parentObject )
	{
		ITriTargetable* target = dynamic_cast<ITriTargetable*>( m_parentObject.p );

		if( !target )
		{
			CCP_LOGERR( "Parent object is not targetable. Unable to get valid damage locators." );
			return in;
		}

		// need shooter's pos
		Vector3 parentPos;
		m_alignPositionCurve->GetValueAt( &parentPos, t );

		// find a damage locator first
		if( m_damageLocatorIndex == -1 )
		{
			m_damageLocatorIndex = target->GetGoodDamageLocatorIndex( parentPos );
		}

		// get it's position
		Vector3 locatorPosWS;
		target->GetDamageLocatorPosition( &locatorPosWS, m_damageLocatorIndex, true );

		// create an impact on the target object
		if( m_impactEffectIndex == -1 )
		{
			m_impactEffectIndex = target->CreateImpact( m_damageLocatorIndex, parentPos - locatorPosWS, 2.f, 1.f );
		}

		// update impact effect and get the position from that impact effect or use locator pos
		target->UpdateImpact( locatorPosWS, parentPos - locatorPosWS, m_impactEffectIndex );

		// out
		in->x = locatorPosWS.x;
		in->y = locatorPosWS.y;
		in->z = locatorPosWS.z;
	}
	return in;
}

Vector3* EveLocalPositionCurve::GetFiringTurretPosition( Vector3* in, Be::Time t )
{
	if( m_turretSetObject )
	{
		auto turretWorld = m_turretSetObject->GetFiringBoneWorldTransform( m_muzzleIndex ).GetTranslation();

		// out
		in->x = turretWorld.x;
		in->y = turretWorld.y;
		in->z = turretWorld.z;
	}
	return in;
}

/////////////////////////////////////////////////////////////////////////////////////
// ITriFunction
/////////////////////////////////////////////////////////////////////////////////////

Vector3* EveLocalPositionCurve::Update(
	Vector3* in,
	Be::Time t )
{
	switch( m_behavior )
	{
	case POS_NEAREST_BOUNDING_POINT:
		return CalculateNearestBoundingPoint( in, t );
	case POS_CENTER_BOUNDING_POINT:
		return GetCenterBoundingSphere( in, t );
	case POS_TARGET_DMG_LOCATOR:
		return GetDamageLocator( in, t );
	case POS_TARGET_DMG_LOCATOR_IMPACT:
		return GetDamageLocatorImpact( in, t );
	case POS_OFFSET_POSITION:
		return CalculateOffsetPosition( in, t );
	case POS_OFFSET_PLANE_ROTATION:
		return CalculateOffsetPlaneRotation( in, t );
	case POS_NEAREST_FIRING_LOCATOR:
		return GetNearestFiringLocator( in, t );
	case POS_ACTIVE_TURRET:
		return GetFiringTurretPosition( in, t );
	default:
		break;
	}

	m_value.x = in->x;
	m_value.y = in->y;
	m_value.z = in->z;

	return in;
}

Vector3* EveLocalPositionCurve::Update( Vector3* in, double time )
{
	return in;
}

Vector3* EveLocalPositionCurve::GetValueAt( Vector3* in, Be::Time time )
{
	in->x = m_value.x;
	in->y = m_value.y;
	in->z = m_value.z;
	Update( in, time );
	return in;
}

Vector3* EveLocalPositionCurve::GetValueAt( Vector3* in, double time )
{
	in->x = m_value.x;
	in->y = m_value.y;
	in->z = m_value.z;

	return in;
}

Vector3* EveLocalPositionCurve::GetValueDoubleDotAt( Vector3* in, Be::Time time )
{
	return in;
}

Vector3* EveLocalPositionCurve::GetValueDoubleDotAt( Vector3* in, double time )
{
	return in;
}

Vector3* EveLocalPositionCurve::GetValueDotAt( Vector3* in, Be::Time time )
{
	return in;
}

Vector3* EveLocalPositionCurve::GetValueDotAt( Vector3* in, double time )
{
	return in;
}

Vector3d* EveLocalPositionCurve::InterpolatedPosition( Vector3d* out, Be::Time time )
{
	out->x = m_value.x;
	out->y = m_value.y;
	out->z = m_value.z;

	return out;
}

void EveLocalPositionCurve::SetBehavior( LocalPositionBehavior behavior )
{
	m_behavior = behavior;
}
