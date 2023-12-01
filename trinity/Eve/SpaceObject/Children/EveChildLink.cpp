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
#include "Utilities/BoundingSphere.h"

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
	m_linkBarrier( 1.f ),
	m_targetRadius( 0.5f )
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
void EveChildLink::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	EveChildMesh::UpdateSyncronous( updateContext, params );

	// if we have a target, we can calc the direction
	if( m_target )
	{
		// target comes from attached ball
		Vector3 tgtPos;
		m_target->GetValueAt( &tgtPos, updateContext.GetTime() );
		// source is from parent
		Vector3 srcPos;

		if ( params.spaceObjectParent )
		{
			params.spaceObjectParent->GetModelCenterWorldPosition( srcPos );
		}
		else
		{
			return;
		}

		// and that gives the direcion and current distance
		m_currentDirection = tgtPos - srcPos;
		m_currentDistance = Length( m_currentDirection );

		m_currentDirection = Normalize( m_currentDirection );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Asynchronous updates happen here
// --------------------------------------------------------------------------------
void EveChildLink::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
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
	if( params.childParent )
	{
		params.childParent->GetLocalToWorldTransform( m_worldTransform );
	}
	else if( params.spaceObjectParent )
	{
		params.spaceObjectParent->GetLocalToWorldTransform( m_worldTransform );
		EveSpaceObject2Ptr p;
		if( params.spaceObjectParent->QueryInterface( BlueInterfaceIID<EveSpaceObject2>(), (void**)&p, BEQI_SILENT ) )
		{
			Vector3 t;
			p->GetShapeEllipsoid( shieldEllipsoidCenter, t );
		}
	}
	else 
	{
		return;
	}

	// link rotation comes from direction
	Vector3 linkMeshDir( 0.f, 1.f, 0.f );
	Matrix linkRotationMat;
	TriMatrixRotationArc( &linkRotationMat, &linkMeshDir, &m_currentDirection );

	if( m_currentDistance <= m_targetRadius )
	{
		// we are always at 100% when we are within the source radius
		m_linkStrength = 1.0;
	}
	else
	{
		// link strength comes from distance vs. barrier 
		float div = abs(m_linkBarrier - m_targetRadius ) ;
		m_linkStrength = TriClamp( 1.0f - ( m_currentDistance - m_targetRadius ) / div, 0.f, 1.f );
	}
	
	// need inverse rotation-only from worldmatrix
	Matrix invRotationWorldMat;
	TriMatrixRemoveTranslation( &invRotationWorldMat, &m_worldTransform );
	invRotationWorldMat = Inverse( invRotationWorldMat );

	// combine inverse to link matrix, so we can do the intersection calculation in axis-aligned space
	linkRotationMat = linkRotationMat * invRotationWorldMat;

	// the link rotation matrix is rotation only so far, so put the offset to the target in it's translation part
	Vector3 offsetToTarget = m_currentDistance * m_currentDirection;
	TriMatrixOverwriteTranslation( &linkRotationMat, &linkRotationMat, &offsetToTarget );

	// the link is to attach to the shield ellipsoid, so use ellipsoid center on worldmatrix
	Matrix ellipsoidCenterMat, finalWorldMat;
	ellipsoidCenterMat = TranslationMatrix( shieldEllipsoidCenter );
	finalWorldMat = ellipsoidCenterMat * m_worldTransform;

	// update perobject data buffers
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();
	
	if( params.spaceObjectParent )
	{
		params.spaceObjectParent->GetPerObjectStructs( m_vsData, m_psData );
	}
	m_vsData.worldTransform = Transpose( finalWorldMat );
	m_vsData.worldTransformLast = Transpose( linkRotationMat );
}

void EveChildLink::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}
	// HOTFIX: Don't LOD out the effect. This ONLY affects the tethering effect
	// TODO: Fix m_currentScreenSize calculation, should be the distance between dest and target in screenspace
	m_isVisible = false;
	m_currentScreenSize = -1;

	if( m_mesh )
	{
		m_isVisible = true;
	}
}

bool EveChildLink::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	// place the sphere in the center of the link
	sphere = Vector4( m_currentDirection, 1.0 ) * m_currentDistance / 2.0f;

	BoundingSphereTransform( m_worldTransform, sphere );
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the Local to World transformation matrix
// --------------------------------------------------------------------------------
void EveChildLink::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}