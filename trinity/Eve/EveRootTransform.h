#pragma once

#ifndef EveRootTransform_h
#define EveRootTransform_h


#include "EveTransform.h"
#include "include/ITriTargetable.h"

BLUE_DECLARE( EveRootTransform );
BLUE_DECLARE_INTERFACE( ITriVectorFunction );
BLUE_DECLARE_INTERFACE( ITriQuaternionFunction );

class EveRootTransform:
	public EveTransform,
	public ITriTargetable
{
public:
    EXPOSE_TO_BLUE();
	using EveTransform::Lock;
	using EveTransform::Unlock;

	EveRootTransform( IRoot* lockobj = NULL );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	virtual void UpdateSyncronous( EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriTargetable
	unsigned int GetDamageLocatorCount() const;
	int GetClosestDamageLocatorIndex( const Vector3* position );
	bool GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace );
	bool GetDamageLocatorDirection( Vector3* out, int index, bool inWorldSpace );
	void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out );
	int GetGoodDamageLocatorIndex( const Vector3& position );
	float GetRadius() const;
	int CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size );
	bool UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex );
	bool GetImpactPosition( Vector3& out, int locator, const Vector3& posPrev, const Vector3& posNow, float epsilon );
	bool HasImpactConfigurationShield() const;

	/////////////////////////////////////////////////////////////////////////////////////
	// Tr2Transform
	virtual void Update( EveUpdateContext& updateContext );
	virtual void UpdateViewDependentData( const TriFrustum& frustum, const Matrix& parentTransform );

protected:
	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;
	ITriQuaternionFunctionPtr m_modelRotation;
	ITriVectorFunctionPtr m_modelTranslation;
	float m_boundingSphereRadius;

	// last known results from updating m_ballPosition and m_ballRotation
	Matrix m_lastUpdateMatrix;

	float GetBoundingSphereRadius() { return m_boundingSphereRadius; }
};

TYPEDEF_BLUECLASS( EveRootTransform );

#endif // EveRootTransform_h
