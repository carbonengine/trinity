////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveChildLink.h"

#include "include/TriMath.h"
#include "TriValueBinding.h"
#include "Eve/EveUpdateContext.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveChildLink::EveChildLink( IRoot* lockobj ) :
	EveChildMesh( lockobj ),
	PARENTLOCK( m_linkStrengthCurves ),
	PARENTLOCK( m_linkStrengthBindings ),
	m_currentDirection( 0.f, 0.f, 1.f ),
	m_currentDistance( 0.f ),
	m_linkStrength( 0.f ),
	m_linkBarrier( 0.f ),
	m_linkBarrierZone( 1.f )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Byebye
// --------------------------------------------------------------------------------
EveChildLink::~EveChildLink()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Synchronous updates happen here
// --------------------------------------------------------------------------------
void EveChildLink::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent )
{
	EveChildMesh::UpdateSyncronous( updateContext, parent );

	// if we have a target, we can calc the direction
	if( m_target )
	{
		// target comes from attached ball
		Vector3 tgtPos;
		m_target->GetValueAt( &tgtPos, updateContext.GetTime() );
		// source is form parent
		Vector3 srcPos;
		parent->GetCurrentModelCenterWorldPosition( srcPos );

		// and that gives the direcion and current distance
		m_currentDirection = tgtPos - srcPos;
		m_currentDistance = D3DXVec3Length( &m_currentDirection );

		D3DXVec3Normalize( &m_currentDirection, &m_currentDirection );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Asynchronous updates happen here
// --------------------------------------------------------------------------------
void EveChildLink::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent )
{
	// update the special link curves
	for( ITriFunctionVector::const_iterator it = m_linkStrengthCurves.begin(); it != m_linkStrengthCurves.end(); ++it )
	{
		(*it)->UpdateValue( m_linkStrength );
	}

	for( ITr2ValueBindingVector::const_iterator it = m_linkStrengthBindings.begin(); it != m_linkStrengthBindings.end(); ++it )
	{
		(*it)->CopyValue();
	}

	// get parent worldmatrix
	parent->GetLocalToWorldTransform( m_worldTransform );

	// link rotation comes from direction
	Vector3 linkMeshDir( 0.f, 1.f, 0.f );
	Matrix linkRotationMat;
	TriMatrixRotationArc( &linkRotationMat, &linkMeshDir, &m_currentDirection );

	// link strength comes from distance vs. barrier
	m_linkStrength = Clamp( ( m_linkBarrierZone - m_currentDistance + m_linkBarrier ) / ( 2.f * m_linkBarrierZone ), 0.f, 1.f );

	// need inverse rotation-only from worldmatrix
	Matrix invRotationWorldMat;
	TriMatrixRemoveTranslation( &invRotationWorldMat, &m_worldTransform );
	D3DXMatrixInverse( &invRotationWorldMat, nullptr, &invRotationWorldMat );

	// combine inverse to link matrix, so we can do the intersection calculation in axis-aligned space
	D3DXMatrixMultiply( &linkRotationMat, &linkRotationMat, &invRotationWorldMat );

	// update perobject data buffers
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();
	
	EveSpaceObject2Ptr parentSpaceObject;
	if( parent->QueryInterface( BlueInterfaceIID<EveSpaceObject2>(), (void**)&parentSpaceObject ) )
	{
		parentSpaceObject->GetPerObjectStructs( m_vsData, m_psData );
	}
	m_vsData.worldTransformLast = linkRotationMat;
	D3DXMatrixTranspose( &m_vsData.worldTransform, &m_worldTransform );
	D3DXMatrixTranspose( &m_vsData.worldTransformLast, &linkRotationMat );
}


// --------------------------------------------------------------------------------
// Description:
//   Synchronous updates happen here
// --------------------------------------------------------------------------------
void EveChildLink::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Asynchronous updates happen here
// --------------------------------------------------------------------------------
void EveChildLink::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the Local to World transformation matrix
// --------------------------------------------------------------------------------
void EveChildLink::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}