// Copyright © 2014 CCP ehf.

#include "StdAfx.h"

#include "EveCustomMask.h"

#include "Eve/SpaceObject/EveSpaceObject2.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveCustomMask::EveCustomMask( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_materialIndex( 0 ),
	m_targetMaterials( 1.f, 1.f, 1.f, 1.f ),
	m_isMirrored( false ),
	m_clampU( false ),
	m_clampV( false )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Destruction!
// --------------------------------------------------------------------------------
EveCustomMask::~EveCustomMask()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Set values
// --------------------------------------------------------------------------------
void EveCustomMask::Setup( const Vector3& position, const Vector3& scaling, const Quaternion& rotation, bool isMirrored, bool clampU, bool clampV, uint8_t srcID, const Vector4& targets )
{
	m_position = position;
	m_scaling = scaling;
	m_rotation = rotation;
	m_isMirrored = isMirrored;
	m_clampU = clampU;
	m_clampV = clampV;
	m_materialIndex = srcID;
	m_targetMaterials = targets;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the matrix used to render the debug boudning box if the projection
// --------------------------------------------------------------------------------
void EveCustomMask::GetDebugDrawMatrix( Matrix* matrix, float objectRadius ) const
{
	// scaling includes size!
	Vector3 finalScale( 0.1f * objectRadius, m_scaling.y * objectRadius, m_scaling.z * objectRadius );

	// build matrix
	*matrix = TransformationMatrix( finalScale, m_rotation, m_position );
}

// --------------------------------------------------------------------------------
// Description:
//   Fill in the needed PPT data into the perobject data
// --------------------------------------------------------------------------------
void EveCustomMask::FillPerObjectData( size_t n, EveSpaceObjectVSData* vsData, EveSpaceObjectPSData* psData ) const
{
	// projection matrix goes into VS data
	Matrix customMaskTransform, invCustomMaskTransform;
	customMaskTransform = TransformationMatrix( m_scaling, m_rotation, m_position );
	invCustomMaskTransform = Inverse( customMaskTransform );
	vsData->customMaskMatrix[n] = Transpose( invCustomMaskTransform );
	// additional data
	vsData->customMaskData[n] = Vector4( 1.f, m_isMirrored ? 1.f : 0.f, 0.f, 0.f );
	// material source IDs go into PS data
	psData->customMaskMaterialIDs[n] = Vector4( (float)m_materialIndex, 0.f, 0.f, 0.f );
	// pattern targets go into PS data
	psData->customMaskTargets[n] = m_targetMaterials;

	psData->customMaskClamps[int32_t( n * 2 )] = m_clampU ? 1.f : 0.f;
	psData->customMaskClamps[int32_t( n * 2 + 1 )] = m_clampV ? 1.f : 0.f;
}

// --------------------------------------------------------------------------------
// Description:
//   Static function to zero out per-object data of PPT
// --------------------------------------------------------------------------------
void EveCustomMask::ZeroPerObjectData( size_t n, EveSpaceObjectVSData* vsData, EveSpaceObjectPSData* psData )
{
	// 0
	vsData->customMaskMatrix[n] = IdentityMatrix();
	psData->customMaskTargets[n] = psData->customMaskMaterialIDs[n] = vsData->customMaskData[n] = Vector4( 0.f, 0.f, 0.f, 0.f );
}
