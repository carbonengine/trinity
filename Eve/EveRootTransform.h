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
	virtual int GetClosestDamageLocatorIndex( const Vector3* position );
	virtual bool GetDamageLocatorPosition( Vector3* out, int index );
	void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out );
	int GetInterestingDamageLocatorIndex( const Vector3 &position ) const;
	int GetGoodDamageLocatorIndex( const Vector3& position );

	/////////////////////////////////////////////////////////////////////////////////////
	// Tr2Transform
	virtual void Update( EveUpdateContext& updateContext );
	virtual void UpdateViewDependentData( const Matrix& parentTransform );

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
