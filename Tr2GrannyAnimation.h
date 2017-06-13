#pragma once
#ifndef Tr2GrannyAnimation_h
#define Tr2GrannyAnimation_h

#include "Include/ITr2AnimationUpdater.h"
#include "GrannyBoneOffset.h"
#include "Tr2GrannyAnimationLayer.h"

BLUE_DECLARE( TriGrannyRes );
BLUE_DECLARE( TriGeometryRes );


struct GrannyBoneBindingBounds
{
	int m_boneIndex;
	Vector3 m_corners[8];
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

	bool IsAnimationEnabled() const;
	void SetAnimationEnabled( bool enabled );

	void	SetSharedGeometryRes( TriGeometryResPtr res );
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

	void AddAnimationLayer( const char* layerName, float layerWeight=1.0f );
	void AddAnimationLayerBone( const char* layerName, const char* boneName );
	void RemoveAnimationLayerBone( const char* layerName, const char* boneName );
	void AddAnimationLayerWithTrackMask( const char* layerName, const char* trackMask );
	float GetAnimationChainCompleteTimeForLayer( const char* layerName );
	float GetLayerWeight( const char* layerName );
	void SetLayerWeight (const char* layerName, float layerWeight );

	void PlayAnimationOnce( const char* animName );
	void PlayAnimationEx( const char* animName, int loopCount, float delay, float speed );
	void ChainAnimation( const char* animName );
	void ChainAnimationEx( const char* animName, int loopCount, float delay, float speed );

	bool GetDynamicBounds( Vector4& boundingSphere, Vector3 &aabbMin, Vector3 &aabbMax );
	void RenderDynamicBounds( const Matrix& modelTransform );
	Vector4 CalculateSkinnedBoundingSphere( granny_file_info* fi=nullptr );
	bool CalculateSkinnedBoundingBoxFromTransform( const Matrix& transform, Vector3& bbMin, Vector3& bbMax, granny_file_info* fi=nullptr );

	void RenderBones( const Matrix& modelTransform );

	std::pair<TriGeometryRes*, std::map<std::pair<TriGeometryRes*, uint32_t>, uint32_t>> CreateStaticGeometry( std::vector<TriGeometryRes*> grannies );

	int GetMeshBoneCount() const;
	const granny_matrix_3x4* GetMeshBoneMatrixList() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// ITr2AnimationUpdater
	void PrePhysicsAnimation( Be::Time time, const Matrix &modelTransform );
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
	
	granny_skeleton *m_skeleton;
	granny_world_pose *m_worldPose;
	granny_mesh_binding *m_meshBinding;

private:
	std::string			m_name;
	std::string			m_resPath;
	std::string			m_model;
	TriGrannyResPtr		m_grannyRes;
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

	typedef TrackableStdVector<std::string> BoneList_t;
	BoneList_t m_boneList;

	// bone matrix list in mesh-order
	granny_matrix_3x4* m_meshBoneMatrixList;
	int m_meshBoneCount;
	int m_modelIndex;
	int m_meshBindingIndex;

	bool m_debugRenderSkeleton;
	bool m_debugRenderJointNames;

	bool	m_useMeshBinding;
	bool m_animationEnabled;

	granny_file_info* GetFileInfo() const;
	Tr2GrannyAnimationLayer* GetAnimationLayer( const char* name );
	
	IBlueEventListenerPtr m_eventListener;
};

TYPEDEF_BLUECLASS( Tr2GrannyAnimation );

#endif //Tr2GrannyAnimation_h
