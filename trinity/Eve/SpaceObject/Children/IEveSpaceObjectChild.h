#ifndef IEveSpaceObjectChild_H
#define IEveSpaceObjectChild_H

#include "Eve/IEveSpaceObject2.h"

class TriFrustum;
struct ITr2Renderable;
class EveSpaceObject2;
class EveUpdateContext;
class Tr2LightManager;

BLUE_DECLARE_INTERFACE( IEveSpaceObjectChild );
BLUE_DECLARE_INTERFACE( IEveChildTransformModifier );

struct EveChildUpdateParams
{
	EveChildUpdateParams()
		:spaceObjectParent( nullptr ),
		childParent( nullptr ),
		boneCount( 0 ),
		bones( nullptr ),
		ownerMaxSpeed( 0 ),
        activationStrength( 1 ),
		isVisible( true ),
		localToWorldTransform( IdentityMatrix() )
	{
	}

	IEveSpaceObject2* spaceObjectParent;
	IEveSpaceObjectChild* childParent;
	size_t boneCount;
	const granny_matrix_3x4* bones;
	float ownerMaxSpeed;
    float activationStrength;
	bool isVisible;
	Matrix localToWorldTransform;
};

BLUE_INTERFACE( IEveSpaceObjectChild ) : public IRoot
{
	enum Origin {
		SPACE,
		SOF,
	};

	virtual const char* GetName() const = 0;
	virtual void SetName( const char* name ) = 0;

	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod ) = 0;
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables ) = 0;
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const = 0;
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer ) {}
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const {}
	
	virtual void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params ) = 0;
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params ) = 0;

	virtual void GetLocalToWorldTransform( Matrix& transform ) const = 0;
	
	virtual bool IsAlwaysOn() const { return false; };

	virtual void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) = 0;

	virtual void ChangeLOD( Tr2Lod lod ) = 0;

	virtual void GetLights( Tr2LightManager& lightManager ) const = 0;

	virtual void SetControllerVariable( const char* name, float value ) {};
	virtual void HandleControllerEvent( const char* name ) {};
	virtual void StartControllers() {};

    virtual void SetProceduralContainerVariable( const char* name, float value ) {};

	virtual void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) {};

	virtual void SetOrigin( Origin origin ) {};

	virtual void AddTransformModifier( IEveChildTransformModifier* modifier ) {};
};

BLUE_DECLARE_IVECTOR( IEveSpaceObjectChild );

#endif