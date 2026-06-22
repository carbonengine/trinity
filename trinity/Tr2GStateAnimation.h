// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2GStateAnimation_h
#define Tr2GStateAnimation_h

#if GSTATE_ENABLED

#include "Include/ITr2AnimationUpdater.h"
#include "Resources/Tr2GrannyStateRes.h"
#if WITH_GRANNY
#include "gstate.h"
#include "gstate_parameters.h"
#include "gstate_tokenized.h"
#include "gstate_state_machine.h"
#endif
#include "Tr2GStateParameter.h"

BLUE_DECLARE( TriGrannyRes );
BLUE_DECLARE( Tr2GrannyStateRes );
BLUE_DECLARE( TriGeometryRes );

BLUE_DECLARE( Tr2GStateParameter );
BLUE_DECLARE_VECTOR( Tr2GStateParameter );

struct GrannyBoneBindingBounds
{
	int m_boneIndex;
	Vector3 m_corners[8];
};

BLUE_CLASS( Tr2GStateAnimation ) :
	public IInitialize,
	public ITr2AnimationUpdater,
	public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();
	Tr2GStateAnimation( IRoot* lockobj = NULL );
	~Tr2GStateAnimation();

	const std::string& GetResPath() const;
	void SetResPath( const std::string& val );

	const std::string& GetGStateResPath() const;
	void SetGStateResPath( const std::string& val );
	const std::vector<std::string> GetGStateAnimFileRefPaths() const;

	void LoadModelFromGstate();
	bool IsFullyLoaded();

	bool IsAnimationEnabled() const;
	void SetAnimationEnabled( bool enabled );

	void SetSharedGeometryRes( TriGeometryResPtr res );
	void SetUseMeshBinding( bool enable )
	{
		m_useMeshBinding = enable;
	}

	const std::string& GetModel() const;
	void SetModel( const std::string& val );
	granny_model* GetGrannyModel() const;

	bool IsInitialized() const;

	std::vector<std::string> GetTopLevelNodeNames();
	std::vector<std::string> GetTopLevelParameterNodeNames();
	std::vector<std::string> GetTopLevelStateNodeNames();
	std::vector<std::string> GetParameters( const std::string& param_node );
	std::vector<float> GetParameterRange( const std::string& param_node, const std::string& param_name );
	gstate::tokenized* GetActiveMachineElement();
	const std::string GetActiveMachineElementName();
	int GetStartStateIdx();
	void SetStartStateIdx( int StartState );
	void SetStartStateByName( const std::string& name );
	bool RequestChangeToState( const std::string& name );
	bool ForceChangeToState( const std::string& name );
	bool StartTransitionByName( const std::string& name );
	float GetParameter( const std::string& param_node_name, granny_int32x param_idx );
	void SetParameter( const std::string& param_node_name, granny_int32x param_idx, float value );
	void RequestParameter( const std::string& param_node_name, granny_int32x param_idx, float value );
	void RequestParameterByName( const std::string& param_node_name, const std::string& param_name, float value );
	float GetParameterByName( const std::string& param_node_name, const std::string& param_name );
	granny_int32x GetParameterIndexByName( const std::string& param_node_name, const std::string& param_name );

	bool GetDynamicBounds( Vector4 & boundingSphere, Vector3 & aabbMin, Vector3 & aabbMax );
	void RenderDynamicBounds( const Matrix& modelTransform );
	Vector4 CalculateSkinnedBoundingSphere( granny_file_info* fi = nullptr );
	bool CalculateSkinnedBoundingBoxFromTransform( const Matrix& transform, Vector3& bbMin, Vector3& bbMax, granny_file_info* fi = nullptr );

	void RenderBones( const Matrix& modelTransform );

	int GetMeshBoneCount() const;
	const Float4x3* GetMeshBoneMatrixList() const;

	void TogglePauseAnimations( bool pause );
	void InstantiateCharacter();

	float GetAnimationTime();
	void StartScrub();
	void ClearScrub();
	void PlayFromScrub();
	void StopPlayFromScrub();
	void SetScrubOffset( float scrub_offset );
	float GetScrubOffset();

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
	void Cleanup();


	bool FindBoneByName( const char* name, unsigned int& ix ) const;
	granny_skeleton* m_skeleton;
	granny_world_pose* m_worldPose;
	granny_mesh_binding* m_meshBinding;


private:
	std::string m_name;
	std::string m_resPath;
	std::string m_gStateResPath;
	std::string m_model;
	granny_model_instance* m_modelInstance;
	TriGrannyResPtr m_grannyRes;
	Tr2GrannyStateResPtr m_gStateRes;
	granny_pose_cache* m_gstate_pose_cache;
	granny_real32 m_last_gstate_time;
	TriGeometryResPtr m_geometryRes;

	bool InitializeBoundingInfo();
	bool m_animationBound;
	bool m_boneBoundsInitialized;
	std::vector<GrannyBoneBindingBounds> m_boneBounds;

	gstate_character_instance* m_gStateCharacterInstance;
	state_machine* m_state_machine;
	granny_local_pose* m_localPose;
	granny_local_pose* m_compositePose;
	std::map<std::string, float> m_animationLayerWeights;

	typedef TrackableStdVector<std::string> BoneList_t;
	BoneList_t m_boneList;

	// bone matrix list in mesh-order
	Float4x3* m_meshBoneMatrixList;
	int m_meshBoneCount;
	int m_modelIndex;
	int m_meshBindingIndex;

	bool m_debugRenderSkeleton;
	bool m_debugRenderJointNames;

	bool m_useMeshBinding;
	bool m_animationEnabled;

	bool m_paused;
	float m_pauseTime;
	float m_totalPauseOffset;

	bool m_scrubbing;
	float m_scrub_time;
	float m_scrub_offset;
	float m_scrub_play_offset;
	bool m_scrub_playing;

	granny_file_info* GetFileInfo() const;
	void LoadGrannyRes();
	void ResetParamsToDefault();

	IBlueEventListenerPtr m_eventListener;

	///////////////////////////////////////////////
	// Tr2GstateAnimationParameter
	void InitializeParametersFromGState();
	Tr2GStateParameterPtr GetParameterObjectByName( const std::string nodeName, const std::string name );
	PTr2GStateParameterVector m_gStateParameterList;
	PTr2GStateParameterVector m_gStateParameterDefaultList;
	// 2d map/dictionary to store parameter indices given parameter node name and parameter name
	std::map<std::tuple<std::string, std::string>, int> m_gStateParameterCachedList;
};

TYPEDEF_BLUECLASS( Tr2GStateAnimation );

#endif

#endif //Tr2GStateAnimation_h