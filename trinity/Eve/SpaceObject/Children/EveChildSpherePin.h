////////////////////////////////////////////////////////////
//
//    Created:   January 2019
//    Copyright: CCP 2019
//

#pragma once
#ifndef EveChildSpherePin_H
#define EveChildSpherePin_H

#include "EveChildMesh.h"
#include "Tr2PerObjectData.h"

// --------------------------------------------------------------------------------
// Description:
//   This class holds the per object data for spherepinChildren
// SeeAlso:
//   Tr2PerObjectData
// --------------------------------------------------------------------------------
class EveChildSpherePinPerObjectData : public Tr2PerObjectData
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;
	// per object data (shared to VS and PS)
	Matrix m_worldMatrix;
	Vector4 m_pinPosition;
	Vector4 m_pinRotation;
	Vector4 m_pinColor;
	Vector4 m_pinThreshold;
	Vector4 m_pinRadiusPrecalc;
	Vector4 m_pinUV;
};

BLUE_CLASS( EveChildSpherePin ) :
	public EveChildMesh
{
public:
	EXPOSE_TO_BLUE();
	EveChildSpherePin( IRoot* lockobj = nullptr );

	~EveChildSpherePin();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

private:

	Vector3 m_centerNormal;
	float m_pinMaxRadius;
	float m_pinRadius;
	float m_pinRotation;
	Color m_pinColor;
	float m_pinAlphaThreshold;
	PTriCurveSetVector m_curveSets;
};

TYPEDEF_BLUECLASS( EveChildSpherePin );

#endif