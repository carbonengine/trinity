////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

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
	void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void ChangeLOD( Tr2Lod lod ) {};

private:
	// what's it pointing at?
	Vector3 m_currentDirection;
	float m_currentDistance;
	// how string is the link?
	float m_linkStrength;
	// where is the barrier
	float m_linkBarrier;
	float m_linkBarrierZone;

	// this is the target
	ITriVectorFunctionPtr m_target;

	// a link has it's own set of curves for link strength lookup
	PITr2ValueBindingVector m_linkStrengthBindings;
	PITriFunctionVector m_linkStrengthCurves;
};

TYPEDEF_BLUECLASS( EveChildLink );

#endif