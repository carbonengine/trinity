// Copyright © 2015 CCP ehf.

#pragma once
#ifndef EveChildLink_H
#define EveChildLink_H

#include "EveChildMesh.h"

// forwards
BLUE_DECLARE_INTERFACE( ITr2ValueBinding );
BLUE_DECLARE_INTERFACE( ITriFunction );
BLUE_DECLARE_IVECTOR( ITriFunction );
BLUE_DECLARE_IVECTOR( ITr2ValueBinding );

BLUE_CLASS( EveChildLink ) :
	public EveChildMesh
{
public:
	EXPOSE_TO_BLUE();

	EveChildLink( IRoot* lockobj = NULL );
	~EveChildLink();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetLocalToWorldTransform( Matrix & transform ) const;
	void ChangeLOD( Tr2Lod lod ) {};
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query ) const;

	bool IsCastingShadow( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, Tr2RenderReason renderReason, float& sizeInShadow ) const override;


private:
	// what's it pointing at?
	Vector3 m_currentDirection;
	float m_currentDistance;
	// how string is the link?
	float m_linkStrength;
	// where is the barrier
	float m_linkBarrier;
	// what is the radius of the target (at that point we are at 100% strength)
	float m_targetRadius;

	// this is the target
	ITriVectorFunctionPtr m_target;

	// a link has it's own set of curves for link strength lookup
	PITr2ValueBindingVector m_linkStrengthBindings;
	PITriFunctionVector m_linkStrengthCurves;
};

TYPEDEF_BLUECLASS( EveChildLink );

#endif