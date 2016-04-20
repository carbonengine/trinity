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
void EveChildLink::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent )
{
	EveChildMesh::UpdateSyncronous( updateContext, spaceObjectParent, childParent );

	// if we have a target, we can calc the direction
	if( m_target )
	{
		// target comes from attached ball
		Vector3 tgtPos;
		m_target->GetValueAt( &tgtPos, updateContext.GetTime() );
		// source is from parent
		Vector3 srcPos;

		if ( spaceObjectParent )
		{
			spaceObjectParent->GetModelCenterWorldPosition( srcPos );
		}
		else
		{
			return;
		}

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
void EveChildLink::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent )
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

	// get parent worldmatrix and parent's shield ellipsoid offset
	Vector3 shieldEllipsoidCenter( 0.f, 0.f, 0.f );
	if( spaceObjectParent )
	{
		spaceObjectParent->GetLocalToWorldTransform( m_worldTransform );
		EveSpaceObject2Ptr p;
		if( spaceObjectParent->QueryInterface( BlueInterfaceIID<EveSpaceObject2>(), (void**)&p, BEQI_SILENT ) )
		{
			Vector3 t;
			p->GetShapeEllipsoid( shieldEllipsoidCenter, t );
		}
	}
	else if ( childParent )
	{
		childParent->GetLocalToWorldTransform( m_worldTransform );
	}
	else
	{
		return;
	}

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

	// the link rotation matrix is rotation only so far, so put the offset to the target in it's translation part
	Vector3 offsetToTarget = m_currentDistance * m_currentDirection;
	TriMatrixOverwriteTranslation( &linkRotationMat, &linkRotationMat, &offsetToTarget );

	// the link is to attach to the shield ellipsoid, so use ellipsoid center on worldmatrix
	Matrix ellipsoidCenterMat, finalWorldMat;
	D3DXMatrixTranslation( &ellipsoidCenterMat, shieldEllipsoidCenter.x, shieldEllipsoidCenter.y, shieldEllipsoidCenter.z );
	D3DXMatrixMultiply( &finalWorldMat, &ellipsoidCenterMat, &m_worldTransform );

	// update perobject data buffers
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();
	
	if( spaceObjectParent )
	{
		spaceObjectParent->GetPerObjectStructs( m_vsData, m_psData );
	}
	D3DXMatrixTranspose( &m_vsData.worldTransform, &finalWorldMat );
	D3DXMatrixTranspose( &m_vsData.worldTransformLast, &linkRotationMat );
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the Local to World transformation matrix
// --------------------------------------------------------------------------------
void EveChildLink::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}