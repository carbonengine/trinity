////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef EveChildTransform_H
#define EveChildTransform_H

#include "Resources/Tr2LodResource.h"

class EveChildTransform
{
public:
	EveChildTransform();

	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );
	void RebuildLocalTransform();
	
protected:
	Vector3 m_translation;
	Vector3 m_scaling;
	Quaternion m_rotation;
	Matrix m_localTransform;
	Matrix m_worldTransform;

	// Indicate that we don't have to update/recreate the transform.
	bool m_staticTransform;
	// Do we want to use the SRT fields or only the local transform.
	bool m_useSRT;
	
	void UpdateTransform( const Matrix& parentTransform );
};


#endif