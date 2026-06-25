// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2GrannyAnimation_h
#define Tr2GrannyAnimation_h

#include "Include/ITr2AnimationUpdater.h"
#include "GrannyBoneOffset.h"
#include "Tr2GrannyAnimationLayer.h"

BLUE_DECLARE( TriGrannyRes );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( Tr2GrannyAnimation );

class Tr2AnimationMeshBinding;

namespace Tr2GrannyAnimationUtils
{
bool GetBoneList( Tr2GrannyAnimation* animationUpdater, const Float4x3*& bones, size_t& boneCount );
std::vector<int32_t> CreateMapping( const cmf::Skeleton& skeleton, cmf::Span<cmf::BoneBinding> boneBindings, uint32_t meshBoneCount );
};

struct GrannyBoneBindingBounds
{
	int m_boneIndex;
	Vector3 m_corners[8];
};


BLUE_INTERFACE( ITr2GrannyAnimationOwner ) :
	public IRoot
{
	virtual Tr2GrannyAnimation* GetAnimationController() const = 0;
};


BLUE_CLASS( Tr2GrannyAnimation ) :
	public IInitialize,
	public ITr2AnimationUpdater,
	public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();
	Tr2GrannyAnimation( IRoot* lockobj = NULL );
	~Tr2GrannyAnimation();

	const std::string& GetResPath() const;
	void SetResPath( const std::string& val );

	void AddSecondaryResPath( const std::string& val );
	const std::string GetSecondaryAnimationName( const std::string& resPath, int index ) const;

	bool IsAnimationEnabled() const;
	void SetAnimationEnabled( bool enabled );

	void SetSharedGeometryRes( TriGeometryResPtr res );
	TriGeometryRes* GetSharedGeometryRes() const;
	void SetUseMeshBinding( bool enable )
	{
		m_useMeshBinding = enable;
	}

	const std::string& GetModel() const;
	void SetModel( const std::string& val );

	const cmf::Skeleton* GetSkeleton() const;
	const std::vector<Matrix>& GetWorldTransforms() const;

#if WITH_GRANNY
	granny_model* GetGrannyModel() const;
#endif

	bool IsInitialized() const;

	bool PlayAnimation( const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone = true );
	bool PlayLayerAnimationByName( const char* layer, const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone );
	void EndAnimation();
	void ClearAnimations();
	float GetAnimationChainCompleteTime();

	Tr2GrannyAnimationLayer* GetAnimationLayer( const char* name );

	void AddAnimationLayer( const char* layerName, float layerWeight = 1.0f );
	void ClearAnimationLayers();
	void AddAnimationLayerBone( const char* layerName, const char* boneName );
	void AddAnimationLayerAllBones( const char* layerName );
	void RemoveAnimationLayerBone( const char* layerName, const char* boneName );
	void AddAnimationLayerWithTrackMask( const char* layerName, const char* trackMask );
	float GetAnimationChainCompleteTimeForLayer( const char* layerName );
	float GetLayerWeight( const char* layerName );
	void SetLayerWeight( const char* layerName, float layerWeight );
	void SetLayerControlParam( const char* layerName, float controlParam );
	void SetLayerControlParamSkewRate( const char* layerName, float skewRate );
	void AimBone( const char* boneName, float target_x, float target_y, float target_z, float axis_x, float axis_y, float axis_z );
	void DisableAimBone();

	void SetAdditiveBlendMode( bool additive );
	bool GetAdditiveBlendMode();

	void PlayAnimationOnce( const char* animName );
	void PlayAnimationEx( const char* animName, int loopCount, float delay, float speed, Be::OptionalWithDefaultValue<bool, true> clearWhenDone );
	void ChainAnimation( const char* animName );
	void ChainAnimationEx( const char* animName, int loopCount, float delay, float speed );

	void TogglePauseAnimations( bool pause );

	bool GetDynamicBounds( Vector4 & boundingSphere, Vector3 & aabbMin, Vector3 & aabbMax );
	void RenderDynamicBounds( const Matrix& modelTransform );

#if WITH_GRANNY
	Vector4 CalculateSkinnedBoundingSphere( granny_file_info* fi = nullptr );
	bool CalculateSkinnedBoundingBoxFromTransform( const Matrix& transform, Vector3& bbMin, Vector3& bbMax, granny_file_info* fi = nullptr );
#endif

	void RenderBones( const Matrix& modelTransform, const Tr2AnimationMeshBinding* meshBinding = nullptr );

	int GetMeshBoneCount() const;
	const Float4x3* GetMeshBoneMatrixList() const;

	std::vector<std::string> GetAnimationNames() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// ITr2AnimationUpdater
	void PrePhysicsAnimation( Be::Time time, const Matrix& modelTransform );
	void PostPhysicsAnimation( Be::Time time, const Matrix& modelTransform );
	const Matrix* GetAnimationTransforms();
	const std::string* GetAnimationBoneList( unsigned int& numBones ) const;

	//////////////////////////////////////////////////////////////////////////
	// IAsyncLoadedResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes * p );
	void RebuildCachedData( BlueAsyncRes * p );

	bool FindBoneByName( const char* name, unsigned int& ix ) const;
	const cmf::Animation* FindCMFAnimationByName( const char* name ) const;
#if WITH_GRANNY
	granny_animation* FindGrannyAnimationByName( const char* name ) const;
#endif
	float FindAnimationDurationByName( const char* name ) const;

	void Cleanup();

	void AddNotifyTarget( IBlueAsyncResNotifyTarget * p );
	void RemoveNotifyTarget( IBlueAsyncResNotifyTarget * p );

	const std::unordered_map<std::string, float>& GetMorphAnimations() const;

#if WITH_GRANNY
	granny_skeleton* m_skeleton;
	granny_world_pose* m_worldPose;
	granny_mesh_binding* m_meshBinding;
#endif

	bool IsUsingCMF() const;
	const cmf::Data* GetCMFData() const;

	const std::vector<int32_t>& GetSkeletonBoneIndices();

	bool HasMeshBinding() const;
	const std::pair<const int32_t*, size_t> GetMeshBindingIndices() const;

	bool GetBoneWorldTransform( const char* boneName, Matrix& transform ) const;

private:
	std::string m_name;
	std::string m_resPath;
	std::string m_model;
	TriGrannyResPtr m_grannyRes;
	std::map<std::string, TriGrannyResPtr> m_secondaryGrannyRes;
	TriGeometryResPtr m_geometryRes;

	bool m_boneBoundsInitialized;
	std::vector<GrannyBoneBindingBounds> m_boneBounds;
	bool InitializeBoundingInfo();

#if WITH_GRANNY
	void UpdateAimingBone();
	void UpdateDebugRenderer( const Matrix& modelTransform );
#endif
	void UpdateAimingBone( const cmf::Skeleton& skeleton );
	void UpdateDebugRenderer( const cmf::Skeleton& skeleton, const Matrix& modelTransform );

	PGrannyBoneOffset m_boneOffset;

#if WITH_GRANNY
	granny_local_pose* m_localPose;
	granny_local_pose* m_compositePose;
#endif
	cmf::SkeletonPose m_pose;
	cmf::SkeletonPose m_tmpPose;
	std::vector<int32_t> m_skeletonBoneIndices;
	std::vector<Matrix> m_worldTransforms;

	std::map<std::string, Tr2GrannyAnimationLayer> m_animationLayers;
	std::map<std::string, float> m_animationLayerWeights;
	Tr2GrannyAnimationLayer m_baseLayer;

	std::unordered_map<std::string, float> m_morphAnimations;

	typedef TrackableStdVector<std::string> BoneList_t;
	BoneList_t m_boneList;

	// bone matrix list in mesh-order
	Float4x3* m_meshBoneMatrixList;
	int m_meshBoneCount;
	int m_modelIndex;
#if WITH_GRANNY
	int m_meshBindingIndex;
#endif

	bool m_debugRenderSkeleton;
	bool m_debugRenderJointNames;
	bool m_aimingBone;
	std::string m_aimBone;
	Vector3 m_aimBoneOrientation;
	Vector3 m_aimAxis;

	bool m_useMeshBinding;
	bool m_animationEnabled;

	bool m_additiveMode;

	float GetAnimationTime();
	bool m_paused;
	float m_pauseTime;
	float m_totalPauseOffset;

#if WITH_GRANNY
	granny_file_info* GetFileInfo() const;
#endif

	void LoadSecondaryResPath( const std::string& val );
	void ApplyBoneOffsets( unsigned i );

	IBlueEventListenerPtr m_eventListener;
	std::vector<IBlueAsyncResNotifyTarget*> m_notifyTargets;
};

TYPEDEF_BLUECLASS( Tr2GrannyAnimation );


class Tr2AnimationMeshBinding : public IBlueAsyncResNotifyTarget
{
public:
	Tr2AnimationMeshBinding( Tr2GrannyAnimation* animationUpdater, TriGeometryRes* geometryRes, uint32_t meshIndex );
	virtual ~Tr2AnimationMeshBinding();

	std::pair<const Float4x3*, size_t> GetBoneTransforms() const;

	TriGeometryRes* GetGeometryRes() const;
	uint32_t GetMeshIndex() const;
	Tr2GrannyAnimation* GetAnimation() const;

	const std::vector<int32_t>& GetAnimBoneIndices() const;
	bool HasMeshBinding() const;
	const std::pair<const int32_t*, size_t> GetMeshBindingIndices() const;

#if WITH_GRANNY
	const granny_mesh_binding* GetGrannyMeshBinding() const;
#endif

private:
	void CreateBinding();

	void ReleaseCachedData( BlueAsyncRes* p ) override;
	void RebuildCachedData( BlueAsyncRes* p ) override;

	const cmf::Skeleton* m_cmfSkeleton;
	std::vector<int32_t> m_bindBoneIndices;
	std::vector<int32_t> m_animBoneIndices;

#if WITH_GRANNY
	std::unique_ptr<granny_mesh_binding, decltype( &GrannyFreeMeshBinding )> m_meshBinding{ nullptr, &GrannyFreeMeshBinding };
	granny_skeleton* m_meshSkeleton = nullptr;
#endif
	std::unique_ptr<Float4x3[]> m_boneTransforms;

	Tr2GrannyAnimationPtr m_animation;
	TriGeometryResPtr m_geometryRes;
	uint32_t m_meshIndex = 0;
};

#endif //Tr2GrannyAnimation_h
