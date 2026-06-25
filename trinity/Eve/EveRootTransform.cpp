// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveRootTransform.h"
#include "EveUpdateContext.h"

#include <ITriFunction.h>

EveRootTransform::EveRootTransform( IRoot* lockobj ) :
	m_boundingSphereRadius( -1.0f ),
	m_lastUpdateMatrix( IdentityMatrix() )
{
}

void EveRootTransform::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Quaternion rotation;
	Vector3 translation;
	Be::Time time = updateContext.GetTime();
	if( m_ballPosition )
	{
		m_ballPosition->Update( &translation, time );
	}
	else
	{
		translation = Vector3( 0.0f, 0.0f, 0.0f );
	}

	if( m_ballRotation )
	{
		m_ballRotation->Update( &rotation, time );
	}
	else
	{
		rotation = Quaternion( 0.0f, 0.0f, 0.0f, 1.0f );
	}

	if( m_modelRotation )
	{
		Quaternion modelRotation;
		m_modelRotation->Update( &modelRotation, time );
		rotation = modelRotation * rotation;
	}

	m_lastUpdateMatrix = RotationMatrix( rotation ) * TranslationMatrix( translation );

	if( m_modelTranslation )
	{
		Vector3 modelTranslation;
		m_modelTranslation->Update( &modelTranslation, time );
		modelTranslation = TransformCoord( modelTranslation, m_lastUpdateMatrix );
		m_lastUpdateMatrix.GetTranslation() = modelTranslation;
	}
	EveTransform::UpdateSyncronous( updateContext );
	EveTransform::UpdateAsyncronous( updateContext );
}

void EveRootTransform::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
}

void EveRootTransform::Update( const EveUpdateContext& updateContext )
{
	UpdateSyncronous( updateContext );
	UpdateAsyncronous( updateContext );
}

void EveRootTransform::UpdateViewDependentData( const TriFrustum& frustum, const Matrix& /*parentTransform*/ )
{
	// parentTransform is identity, since we're the root transform
	EveTransform::UpdateViewDependentData( frustum, m_lastUpdateMatrix );
}

// --------------------------------------------------------------------------------
// Description:
//   Is mostly used for effects, so no damage locators at all!
// --------------------------------------------------------------------------------
unsigned int EveRootTransform::GetDamageLocatorCount() const
{
	return 0;
}

bool EveRootTransform::GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace )
{
	*out = m_worldTransform.GetTranslation();
	return true;
}

bool EveRootTransform::GetDamageLocatorDirection( Vector3* out, int index, bool inWorldSpace )
{
	*out = Vector3( 0.f, 1.f, 0.f );
	return true;
}

bool EveRootTransform::GetImpactPosition( Vector3& out, int locator, const Vector3& posPrev, const Vector3& posNow, float epsilon )
{
	GetDamageLocatorPosition( &out, locator, true );
	return LengthSq( posNow - out ) < epsilon;
}

bool EveRootTransform::HasImpactConfigurationShield() const
{
	return false;
}

int EveRootTransform::GetClosestDamageLocatorIndex( const Vector3* position )
{
	return 0;
}

int EveRootTransform::GetGoodDamageLocatorIndex( const Vector3& position )
{
	return 0;
}

float EveRootTransform::GetRadius() const
{
	return m_boundingSphereRadius;
}

// -----------------------------------------------------------------------------
// Description:
//   Create an impact effect on this object
//   Is empty for transforms!
// -----------------------------------------------------------------------------
int EveRootTransform::CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size )
{
	return -1;
}

// -----------------------------------------------------------------------------
// Description:
//   Update the effect on this object
//   Is empty for transforms!
// -----------------------------------------------------------------------------
bool EveRootTransform::UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex )
{
	return false;
}

void EveRootTransform::GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out )
{
	GetDamageLocatorPosition( out, -1, true );

	if( hit && source )
	{
		Vector3 local( *hit - *out );
		Vector3 dir = Normalize( *hit - *source );

		local -= dir * Dot( dir, local );

		local = Normalize( local );
		const Vector3 off = local * m_boundingSphereRadius * 1.125f;
		*out += off;
	}
}
