#include "StdAfx.h"
#include "EveSpaceObjectFxAttributes.h"
#include "Tr2Renderer.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/SpaceObject/EveShip2.h"

EveSpaceObjectFxAttributes::EveSpaceObjectFxAttributes( IRoot* lockobj ) :
    m_activationStrength( 1.f ),
	m_distanceToShip( 0 ),
	m_boundingSphereRadius( 0 ),
	m_distanceToChildParent( 0 ),
    m_killCount( 0 ),
	m_initialized( false ),
	m_generatedShapeEllipsoidCenter( 0.f, 0.f, 0.f ),
	m_generatedShapeEllipsoidRadius( 0.f, 0.f, 0.f )
{
}

EveSpaceObjectFxAttributes::~EveSpaceObjectFxAttributes()
{
}

void EveSpaceObjectFxAttributes::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
    if (nullptr == params.spaceObjectParent)
    {
        return;
    }

	if( !m_initialized )
	{
		// things we only want to calculate once
        if ( EveSpaceObject2Ptr rootParent = BlueCastPtr( params.spaceObjectParent ) )
        {
            rootParent->GetShapeEllipsoid(m_generatedShapeEllipsoidCenter, m_generatedShapeEllipsoidRadius );
        }

        if ( EveShip2Ptr rootParent = BlueCastPtr( params.spaceObjectParent ) )
        {
            m_killCount = rootParent->GetKillCounterValue();
			m_activeTurretCount = float(rootParent->GetActiveTurretCount());
        }

		m_initialized = true;
	}
	
	// gather attributes
	Vector3 objPos;
	Vector4 sphere;
	params.spaceObjectParent->GetBoundingSphere( sphere );
	params.spaceObjectParent->GetModelCenterWorldPosition( objPos );

    m_activationStrength = params.activationStrength;
	m_distanceToShip = Length( objPos ) - m_boundingSphereRadius;
	m_boundingSphereRadius = sphere.w;
}