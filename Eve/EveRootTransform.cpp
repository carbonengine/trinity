#include "StdAfx.h"
#include "EveRootTransform.h"
#include "EveUpdateContext.h"
#include "Include/ITriFunction.h"

EveRootTransform::EveRootTransform( IRoot* lockobj ):
	m_boundingSphereRadius( -1.0f )
{
	m_lastUpdateMatrix = Tr2Renderer::GetIdentityTransform();
}

void EveRootTransform::UpdateSyncronous( EveUpdateContext& updateContext )
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
		D3DXQuaternionMultiply( &rotation, &modelRotation, &rotation);
	}

	D3DXMatrixTransformation( &m_lastUpdateMatrix, NULL, NULL, NULL, NULL, &rotation, &translation );

	if( m_modelTranslation )
	{
		Vector3 modelTranslation;
		m_modelTranslation->Update( &modelTranslation, time );
		D3DXVec3TransformCoord( &modelTranslation, &modelTranslation, &m_lastUpdateMatrix );
		m_lastUpdateMatrix.GetTranslation() = modelTranslation;
	}
	EveTransform::UpdateSyncronous( updateContext );
}

void EveRootTransform::UpdateAsyncronous( EveUpdateContext& updateContext )
{
	EveTransform::UpdateAsyncronous( updateContext );
}

void EveRootTransform::Update( EveUpdateContext& updateContext )
{
	UpdateSyncronous( updateContext );
	UpdateAsyncronous( updateContext );
}

void EveRootTransform::UpdateViewDependentData( const Matrix& /*parentTransform*/, bool )
{
	// parentTransform is identity, since we're the root transform
	EveTransform::UpdateViewDependentData( m_lastUpdateMatrix );	
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

void EveRootTransform::GetImpactPosition( Vector3& out, int damageLocatorIndex, const Vector3& direction )
{
	GetDamageLocatorPosition( &out, damageLocatorIndex, true );
}

bool EveRootTransform::HasImpactConfigurationShield() const
{
	return false;
}

int EveRootTransform::GetClosestDamageLocatorIndex( const Vector3* position )
{
	return 0;
}

int EveRootTransform::GetGoodDamageLocatorIndex( const Vector3 &position )
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
	GetDamageLocatorPosition(out, -1, true );
	
	if( hit && source ) 
	{
		Vector3 local( *hit - *out );
		Vector3 dir( *hit - *source );
		
		D3DXVec3Normalize( &dir, &dir );
		local -= dir * D3DXVec3Dot( &dir, &local );

		D3DXVec3Normalize( &local, &local );
		const Vector3 off = local * m_boundingSphereRadius * 1.125f;
		*out += off;
	}
}
