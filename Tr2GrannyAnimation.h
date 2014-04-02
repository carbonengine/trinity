#pragma once
#ifndef Tr2GrannyAnimation_h
#define Tr2GrannyAnimation_h

#include "ITr2AnimationUpdater.h"
#include "GrannyBoneOffset.h"

BLUE_DECLARE( TriGrannyRes );
BLUE_DECLARE( TriGeometryRes );

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

	void	SetSharedGeometryRes( TriGeometryResPtr res );
	void	SetUseMeshBinding( bool enable ) { m_useMeshBinding = enable; }

	const std::string& GetModel() const;
	void SetModel( const std::string& val);

	bool	PlayAnimation( const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone=true );
	void	EndAnimation();
	void	ClearAnimations();
	float	GetAnimationChainCompleteTime();

	void PlayAnimationOnce( const char* animName );
	void PlayAnimationEx( const char* animName, int loopCount, float delay, float speed );
	void ChainAnimation( const char* animName );
	void ChainAnimationEx( const char* animName, int loopCount, float delay, float speed );

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
	const std::string *GetAnimationBoneList( unsigned int &numBones );

	//////////////////////////////////////////////////////////////////////////
	// IAsyncLoadedResNotifyTarget
	void	ReleaseCachedData( BlueAsyncRes* p );
	void	RebuildCachedData( BlueAsyncRes* p );

	bool	FindBoneByName( const char* name, unsigned int& ix );

	void	Cleanup();

protected:
	std::string			m_name;
	std::string			m_resPath;
	std::string			m_model;
	TriGrannyResPtr		m_grannyRes;
	TriGeometryResPtr	m_geometryRes;

	PGrannyBoneOffset	m_boneOffset;

	struct AnimationRequest
	{
		std::string m_animationName;
		bool m_replace;
		int m_loopCount;
		float m_start;
		float m_speed;
	};
	typedef TrackableStdList<AnimationRequest> AnimationRequestList;
	AnimationRequestList m_animationQueue;

public:
	granny_skeleton			*m_skeleton;
	granny_model_instance	*m_modelInstance;
	granny_local_pose		*m_localPose;
	granny_world_pose		*m_worldPose;
	granny_mesh_binding		*m_meshBinding;

protected:
	typedef TrackableStdVector<std::string> BoneList_t;
	BoneList_t m_boneList;

	// bone matrix list in mesh-order
	granny_matrix_3x4* m_meshBoneMatrixList;
	int m_meshBoneCount;

	bool m_debugRenderSkeleton;
	bool m_debugRenderJointNames;

	bool	m_useMeshBinding;

	granny_file_info* GetFileInfo();
};

TYPEDEF_BLUECLASS( Tr2GrannyAnimation );

#endif //Tr2GrannyAnimation_h
