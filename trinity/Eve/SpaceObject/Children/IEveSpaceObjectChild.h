// Copyright © 2023 CCP ehf.

#ifndef IEveSpaceObjectChild_H
#define IEveSpaceObjectChild_H

#include "Eve/IEveSpaceObject2.h"

class TriFrustum;
struct ITr2Renderable;
class EveSpaceObject2;
class EveUpdateContext;
class Tr2LightManager;

BLUE_DECLARE_INTERFACE( IEveSpaceObjectChild );
BLUE_DECLARE( EveSpaceObjectChild );
BLUE_DECLARE_INTERFACE( IEveChildTransformModifier );
BLUE_DECLARE_INTERFACE( ITriVectorFunction );

struct EveChildUpdateParams
{
	EveChildUpdateParams() :
		spaceObjectParent( nullptr ),
		childParent( nullptr ),
		boneCount( 0 ),
		bones( nullptr ),
		ownerMaxSpeed( 0 ),
		activationStrength( 1 ),
		isVisible( true ),
		localToWorldTransform( IdentityMatrix() ),
		controllerUpdateFrequency( 0.5f ),
		worldVelocity( 0.f, 0.f, 0.f ),
		ballPosition( nullptr )
	{
	}

	IEveSpaceObject2* spaceObjectParent;
	EveSpaceObjectChild* childParent;
	size_t boneCount;
	const Float4x3* bones;
	float ownerMaxSpeed;
	float activationStrength;
	float controllerUpdateFrequency;
	bool isVisible;
	Matrix localToWorldTransform;
	Vector3 worldVelocity;
	/// Translation curve of the root space object, usually its destiny ball. May be nullptr.
	ITriVectorFunction* ballPosition;
};

/**
 * @brief Deprecated interface for EveSpaceObjectChild. Classes deriving from EveSpaceObjectChild should still declare
 * this interface in MAP_INTERFACE macro for backward compatibility with Python code, but otherwise this interface should not be used directly.
 */
BLUE_INTERFACE( IEveSpaceObjectChild ) :
	public IRoot{};

BLUE_DECLARE_IVECTOR( IEveSpaceObjectChild );

#endif