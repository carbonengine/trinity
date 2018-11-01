////////////////////////////////////////////////////////////
//
//    Created:   April 2014
//    Copyright: CCP 2014
//
#pragma once
#ifndef EveCustomMask_H
#define EveCustomMask_H

// forwards
struct EveSpaceObjectPSData;
struct EveSpaceObjectVSData;

// --------------------------------------------------------------------------------
// Description:
//   This class holds data about custom masks for spaceobjects
// SeeAlso:
//   EveSpaceObject2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveCustomMask ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveCustomMask( IRoot* lockobj = NULL );
	~EveCustomMask();

	// access
	void GetDebugDrawMatrix( Matrix* matrix, float objectRadius ) const;
	void FillPerObjectData( size_t n,  EveSpaceObjectVSData* vsData, EveSpaceObjectPSData* psData ) const;
	void Setup( const Vector3& position, const Vector3& scaling, const Quaternion& rotation, bool isMirrored, bool clampU, bool clampV, uint8_t srcID, const Vector4& targets );
	static void ZeroPerObjectData( size_t n, EveSpaceObjectVSData* vsData, EveSpaceObjectPSData* psData );

private:
	/////////////////////////////////////////////////////////////////////////////////////
	// mask projection data
	Vector3 m_position;
	Vector3 m_scaling;
	Quaternion m_rotation;

	// source material index
	uint8_t m_materialIndex;

	// target material
	Vector4 m_targetMaterials;

	// options
	bool m_isMirrored;
	bool m_clampU;
	bool m_clampV;

};

TYPEDEF_BLUECLASS( EveCustomMask );

#endif // EveCustomMask_H
