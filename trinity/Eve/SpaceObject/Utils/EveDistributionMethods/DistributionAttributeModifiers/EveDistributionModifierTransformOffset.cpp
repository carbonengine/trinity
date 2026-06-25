// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionModifierTransformOffset.h"
#include "TriMath.h"

EveDistributionModifierTransformOffset::EveDistributionModifierTransformOffset( IRoot* lockobj ) :
	m_translation( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scale( 1.f, 1.f, 1.f )
{
}


bool EveDistributionModifierTransformOffset::AffectsTransform()
{
	return true;
}

DistributionEntityLifeTimeEvent EveDistributionModifierTransformOffset::ProcessDistributionModifier( PlacementDataWithIdentifier& placement, float deltaTime, const EveChildUpdateParams& params )
{
	if( m_translationCurve )
	{
		Vector3 posOffset;
		Quaternion rotation = placement.initialRotation * placement.additionalRotation;
		m_translationCurve->GetValueAt( &posOffset, double( placement.lifeTime ) );
		TriVectorRotateQuaternion( &posOffset, &posOffset, &rotation );
		placement.additionalTranslation += posOffset;
	}
	else
	{
		Vector3 posOffset;
		Quaternion rotation = placement.initialRotation * placement.additionalRotation;
		TriVectorRotateQuaternion( &posOffset, &m_translation, &rotation );
		placement.additionalTranslation += posOffset;
	}

	if( m_rotationCurve )
	{
		Quaternion rotation;
		m_rotationCurve->GetValueAt( &rotation, double( placement.lifeTime ) );
		placement.additionalRotation *= rotation;
	}
	else
	{
		placement.additionalRotation *= m_rotation;
	}

	if( m_scaleCurve )
	{
		Vector3 scale;
		m_scaleCurve->GetValueAt( &scale, double( placement.lifeTime ) );
		placement.additionalScale *= scale;
	}
	else
	{
		placement.additionalScale *= m_scale;
	}

	return DistributionEntityLifeTimeEvent::DO_NOTHING;
}