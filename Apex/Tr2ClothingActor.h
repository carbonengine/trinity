#pragma once
#ifndef Tr2ClothingActor_h
#define Tr2ClothingActor_h

#include "Utilities/BoundingBox.h"

BLUE_DECLARE( Tr2ClothingActor );
BLUE_DECLARE( Tr2ClothingRes );
BLUE_DECLARE_INTERFACE( ITr2ShaderMaterial );
BLUE_DECLARE( TriGrannyRes );
BLUE_DECLARE( Tr2ApexScene );

namespace physx
{
	class PxMat34;

	namespace apex {

		class NxClothingActor;
		class NxClothingMaterial;
		class NxApexRenderable;

	}
}

// The Tr2ClothingActor class is a wrapper around the NxClothingActor class
// from Apex. It is an instance of a piece of clothing coming from a clothing
// asset (NxClothingAsset), wrapped in a Tr2ClothingRes.
class Tr2ClothingActor:
	public IInitialize,
	public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();
	using IInitialize::Lock;
	using IInitialize::Unlock;

	Tr2ClothingActor( IRoot* lockobj = NULL );
	~Tr2ClothingActor();

	physx::apex::NxApexRenderable* GetApexRenderable();

	void AddToApexScene( Tr2ApexScene* apexScene );
	void RemoveFromApexScene();

	void BindToRig( const std::string* boneList, unsigned int numBones, bool forceRebind );

	enum UpdateStateFlag
	{
		IS_NOT_CONTINUOUS,
		IS_CONTINUOUS
	};
	void UpdateState( const Matrix& globalPose, float* skinningMatrices, unsigned int skinningMatrixCount, UpdateStateFlag continuousFlag, Tr2ApexScene* apexScene );

	float GetMaxDistanceBlendTime() const;
	void SetForcedLod( float lod );

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* p );
	void RebuildCachedData( BlueAsyncRes* p );

	AxisAlignedBoundingBox GetWorldBoundingBox() const;

#if APEX_ENABLED

	ITr2ShaderMaterial* GetEffect();
	ITr2ShaderMaterial* GetDepthEffect();
	ITr2ShaderMaterial* GetDepthNormalEffect();

	ITr2ShaderMaterial* GetEffectReversed();
	ITr2ShaderMaterial* GetDepthEffectReversed();
	ITr2ShaderMaterial* GetDepthNormalEffectReversed();

	bool GetUseTransparentBatches() const;
	bool GetUseSHLighting() const;

protected:
	// Sets the resource path, triggering the resource load via the resource
	// manager. Note that the resource is loaded asynchronously so if it wasn't
	// already in memory it may take some time before the actor is fully active.
	void SetResPath( const std::string& val );
	const std::string& GetResPath() const;

	void SetVisualize( bool b );
	bool GetVisualize();

	void SetBlendVelocity( float bv );
	float GetBlendVelocity();

	float GetBendingStiffness() const;
	void SetBendingStiffness(float val);

	float GetStretchingStiffness() const;
	void SetStretchingStiffness(float val);

	bool GetOrthoBending() const;
	void SetOrthoBending( bool val );

	float GetDamping() const;
	void SetDamping(float val);

	bool GetComDamping() const;
	void SetComDamping( bool val );

	float GetFriction() const;
	void SetFriction(float val);

	unsigned int GetSolverIterations() const;
	void SetSolverIterations(unsigned int val);

	float GetGravityScale() const;
	void SetGravityScale(float val);

	float GetHardStretchLimitation() const;
	void SetHardStretchLimitation(float val);

	float GetMaxDistanceBias() const;
	void SetMaxDistanceBias(float val);

	float GetMaxDistanceScale() const;
	void SetMaxDistanceScale(float val);

	unsigned int GetHierarchicalSolverIterations() const;
	void SetHierarchicalSolverIterations(unsigned int val);

	void Cleanup();

protected:
	std::string m_name;
	std::string m_resPath;

	// Data used for driving the apex morph target features.
	// If this is set before the apex actor is created (which happens
	// when it's added to the scene), these will apply to the actor.
	// 
	// Granny resource containing the base vertices (to figure out which
	// blend vertex maps to which cloth vertex) and the blend shapes.
	TriGrannyResPtr	m_morphRes;
	// Index of the mesh in m_morphRes that we want to use.
	unsigned		m_morphResMeshIndex;
	// Tolerance value for matching skinned vertices with cloth vertices.
	float			m_morphResEpsilon;
	// Weights used for blend shapes; should match the number of morph targets
	// in m_morphRes.  See PySetMorphResWeights.
	std::vector<float>	m_morphResWeights;
#if BLUE_WITH_PYTHON
	friend PyObject* PySetMorphResWeights( PyObject* self, PyObject* args );
#endif

	Tr2ClothingResPtr m_clothingRes;
	physx::apex::NxClothingActor* m_clothingActor;
	std::string m_clothingMaterialLibraryName;
	physx::apex::NxClothingMaterial* m_clothingMaterial;

	// Keep track of whether the object is in a scene. We may be added
	// to a scene before the resource has finished loading - in that
	// case we must create the actor as soon as the get the load finished
	// notification.
	bool m_isInScene;

	// An array of 4x4 matrices, in the order expected by the NxClothingActor 
	physx::PxMat44* m_bones;
	unsigned int m_boneCount;
	
	// Map from animation skeleton to bones in the clothing asset
	TrackableStdVector<unsigned int> m_boneMap;
	const std::string* m_lastBoundBoneList;

	ITr2ShaderMaterialPtr m_effect;
	ITr2ShaderMaterialPtr m_depthEffect;
	ITr2ShaderMaterialPtr m_depthNormalEffect;

	ITr2ShaderMaterialPtr m_effectReversed;
	ITr2ShaderMaterialPtr m_depthEffectReversed;
	ITr2ShaderMaterialPtr m_depthNormalEffectReversed;

	Vector3 m_windDirection;
	float m_windStrength;

	float m_blendVelocity;

	bool  m_resetCloth;	// if true, the next update resets the cloth to its skinned position

	bool m_useTransparentBatches;
	bool m_useSHLighting;

	// LOD weights
	float m_maxDistance;
	float m_distanceWeight;
	float m_bias;
	float m_benefitBias;

	float m_stretchingStiffness;
	float m_bendingStiffness;
	bool m_orthoBending;
	float m_damping;
	bool m_comDamping;
	float m_friction;
	unsigned int m_solverIterations;
	float m_gravityScale;
	float m_hardStretchLimitation;
	float m_maxDistanceBias;
	float m_maxDistanceScale;
	unsigned int m_hierarchicalSolverIterations;
#endif // APEX_ENABLED

};

TYPEDEF_BLUECLASS( Tr2ClothingActor );

#endif //Tr2ClothingActor_h
