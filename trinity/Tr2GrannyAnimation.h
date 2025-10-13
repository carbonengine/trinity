#pragma once
#ifndef Tr2GrannyAnimation_h
#define Tr2GrannyAnimation_h

#include "Include/ITr2AnimationUpdater.h"
#include "GrannyBoneOffset.h"
#include "Tr2GrannyAnimationLayer.h"
#include "../Tr2MorphTargetAnimationDataBuffer.h"

BLUE_DECLARE( TriGrannyRes );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( Tr2GrannyAnimation );

class Tr2AnimationMeshBinding;

namespace Tr2GrannyAnimationUtils
{
	bool GetBoneList( Tr2GrannyAnimation* animationUpdater, const granny_matrix_3x4*& bones, size_t& boneCount );
};

struct GrannyBoneBindingBounds
{
	int m_boneIndex;
	Vector3 m_corners[8];
};

enum Tr2MorphTargetState
{
	DrivenByAnimation,
	Overwritten
};


BLUE_INTERFACE( ITr2GrannyAnimationOwner ) :
	public IRoot
{
	virtual Tr2GrannyAnimation* GetAnimationController() const = 0;
};


BLUE_CLASS( Tr2GrannyAnimation ):
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

	void	SetSharedGeometryRes( TriGeometryResPtr res );
	TriGeometryRes* GetSharedGeometryRes() const;
	void	SetUseMeshBinding( bool enable ) { m_useMeshBinding = enable; }

	const std::string& GetModel() const;
	void SetModel( const std::string& val);
	granny_model* GetGrannyModel() const;
	
	bool IsInitialized() const;

	bool	PlayAnimation( const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone=true );
	bool	PlayLayerAnimationByName( const char* layer, const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone );
	void	EndAnimation();
	void	ClearAnimations();
	float	GetAnimationChainCompleteTime();

	Tr2GrannyAnimationLayer* GetAnimationLayer( const char* name );

	void AddAnimationLayer( const char* layerName, float layerWeight=1.0f );
	void ClearAnimationLayers();
	void AddAnimationLayerBone( const char* layerName, const char* boneName );
	void AddAnimationLayerAllBones( const char* layerName );
	void RemoveAnimationLayerBone( const char* layerName, const char* boneName );
	void AddAnimationLayerWithTrackMask( const char* layerName, const char* trackMask );
	float GetAnimationChainCompleteTimeForLayer( const char* layerName );
	float GetLayerWeight( const char* layerName );
	void SetLayerWeight ( const char* layerName, float layerWeight );
	void SetLayerControlParam ( const char* layerName, float controlParam );
	void SetLayerControlParamSkewRate ( const char* layerName, float skewRate );
	void AimBone( const char* boneName, float target_x, float target_y, float target_z, float axis_x, float axis_y, float axis_z );
	void DisableAimBone();

	void SetAdditiveBlendMode( bool additive );
	bool GetAdditiveBlendMode();

	void PlayAnimationOnce( const char* animName );
	void PlayAnimationEx( const char* animName, int loopCount, float delay, float speed );
	void ChainAnimation( const char* animName );
	void ChainAnimationEx( const char* animName, int loopCount, float delay, float speed );

	void TogglePauseAnimations( bool pause );

	bool GetDynamicBounds( Vector4& boundingSphere, Vector3 &aabbMin, Vector3 &aabbMax );
	void RenderDynamicBounds( const Matrix& modelTransform );
	Vector4 CalculateSkinnedBoundingSphere( granny_file_info* fi=nullptr );
	bool CalculateSkinnedBoundingBoxFromTransform( const Matrix& transform, Vector3& bbMin, Vector3& bbMax, granny_file_info* fi=nullptr );

	void RenderBones( const Matrix& modelTransform, const Tr2AnimationMeshBinding* meshBinding = nullptr );

	int GetMeshBoneCount() const;
	const granny_matrix_3x4* GetMeshBoneMatrixList() const;

	std::vector<std::string> GetAnimationNames() const;
	
	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// ITr2AnimationUpdater
	void PrePhysicsAnimation( Be::Time time, const Matrix& modelTransform );
	void PostPhysicsAnimation( Be::Time time, const Matrix &modelTransform );
	const Matrix* GetAnimationTransforms();
	const std::string *GetAnimationBoneList( unsigned int& numBones ) const;

	//////////////////////////////////////////////////////////////////////////
	// IAsyncLoadedResNotifyTarget
	void	ReleaseCachedData( BlueAsyncRes* p );
	void	RebuildCachedData( BlueAsyncRes* p );

	bool	FindBoneByName( const char* name, unsigned int& ix ) const;
	granny_animation* FindAnimationByName( const char* name ) const;

	void	Cleanup();

	void AddNotifyTarget( IBlueAsyncResNotifyTarget * p );
	void RemoveNotifyTarget( IBlueAsyncResNotifyTarget * p );

	const std::unordered_map<std::string, float>& GetMorphAnimations() const;

	granny_skeleton *m_skeleton;
	granny_world_pose *m_worldPose;
	granny_mesh_binding *m_meshBinding;

private:
	std::string			m_name;
	std::string			m_resPath;
	std::string			m_model;
	TriGrannyResPtr		m_grannyRes;
	std::map<std::string, TriGrannyResPtr>	m_secondaryGrannyRes;
	TriGeometryResPtr	m_geometryRes;

	bool m_boneBoundsInitialized;
	std::vector<GrannyBoneBindingBounds> m_boneBounds;
	bool InitializeBoundingInfo();

	PGrannyBoneOffset m_boneOffset;
	granny_local_pose *m_localPose;
	granny_local_pose *m_compositePose;
	std::map<std::string, Tr2GrannyAnimationLayer> m_animationLayers;
	std::map<std::string, float> m_animationLayerWeights;
	Tr2GrannyAnimationLayer m_baseLayer;

	std::unordered_map<std::string, float> m_morphAnimations;

	typedef TrackableStdVector<std::string> BoneList_t;
	BoneList_t m_boneList;

	// bone matrix list in mesh-order
	granny_matrix_3x4* m_meshBoneMatrixList;
	int m_meshBoneCount;
	int m_modelIndex;
	int m_meshBindingIndex;

	bool m_debugRenderSkeleton;
	bool m_debugRenderJointNames;
	bool m_aimingBone;
	std::string m_aimBone;
	Vector3 m_aimBoneOrientation;
	Vector3 m_aimAxis;

	bool	m_useMeshBinding;
	bool m_animationEnabled;

	bool m_additiveMode;

	float GetAnimationTime();
	bool m_paused;
	float m_pauseTime;
	float m_totalPauseOffset;

	granny_file_info* GetFileInfo() const;
	void LoadSecondaryResPath( const std::string& val );
	void	ApplyBoneOffsets ( unsigned i );
	
	IBlueEventListenerPtr m_eventListener;
	std::vector<IBlueAsyncResNotifyTarget*> m_notifyTargets;
};

TYPEDEF_BLUECLASS( Tr2GrannyAnimation );


class Tr2AnimationMeshBinding : public IBlueAsyncResNotifyTarget
{
public:
	Tr2AnimationMeshBinding( Tr2GrannyAnimation* animationUpdater, TriGeometryRes* geometryRes, uint32_t meshIndex );
	virtual ~Tr2AnimationMeshBinding();

	std::pair<const granny_matrix_3x4*, size_t> GetBoneTransforms() const;
	//std::pair<const Tr2MorphTargetAnimationData*, size_t> GetMorphTargets() const;

	TriGeometryRes* GetGeometryRes() const;
	uint32_t GetMeshIndex() const;
	Tr2GrannyAnimation* GetAnimation() const;

	const granny_mesh_binding* GetGrannyMeshBinding() const;

private:
	void CreateBinding();

	void ReleaseCachedData( BlueAsyncRes* p ) override;
	void RebuildCachedData( BlueAsyncRes* p ) override;

	std::unique_ptr<granny_mesh_binding, decltype( &GrannyFreeMeshBinding )> m_meshBinding{ nullptr, &GrannyFreeMeshBinding };
	std::unique_ptr<granny_matrix_3x4[]> m_boneTransforms;
	granny_skeleton* m_meshSkeleton = nullptr;

	Tr2GrannyAnimationPtr m_animation;
	TriGeometryResPtr m_geometryRes;
	uint32_t m_meshIndex = 0;
};

#endif //Tr2GrannyAnimation_h
