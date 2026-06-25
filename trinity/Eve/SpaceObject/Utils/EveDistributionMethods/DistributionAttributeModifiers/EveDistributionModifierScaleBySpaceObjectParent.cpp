// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionModifierScaleBySpaceObjectParent.h"
#include "TriMath.h"

EveDistributionModifierScaleBySpaceObjectParent::EveDistributionModifierScaleBySpaceObjectParent( IRoot* lockobj ) :
	m_authoredForBoundingRadius( 1000 ),
	m_scaleFactor( 1.f )
{
}

bool EveDistributionModifierScaleBySpaceObjectParent::AffectsTransform()
{
	return m_scaleFactor != 0.f;
}

DistributionEntityLifeTimeEvent EveDistributionModifierScaleBySpaceObjectParent::ProcessDistributionModifier( PlacementDataWithIdentifier& placement, float deltaTime, const EveChildUpdateParams& params )
{
	if( params.spaceObjectParent )
	{
		Vector4 bounds;
		params.spaceObjectParent->GetBoundingSphere( bounds );

		if( m_scaleCurve != nullptr )
		{
			Vector3 finalScale;
			m_scaleCurve->GetValueAt( &finalScale, double( bounds.w ) );
			placement.additionalScale *= finalScale;
		}
		else
		{
			float assetRatio = bounds.w / max( m_authoredForBoundingRadius, 1.f );
			float finalScale;
			if( assetRatio > 1.f )
			{
				finalScale = 1.f + m_scaleFactor * ( assetRatio - 1.f );
			}
			else
			{
				finalScale = pow( assetRatio, m_scaleFactor );
			}

			placement.additionalScale *= finalScale;
		}
	}

	return DistributionEntityLifeTimeEvent::DO_NOTHING;
}
