// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2GrannyAnimationLayer_h
#define Tr2GrannyAnimationLayer_h

#if WITH_GRANNY
#include "granny.h"
#endif
#include "Include/ITr2AnimationUpdater.h"
#include "GrannyBoneOffset.h"
#include <BlueAsyncRes.h>

#if WITH_GRANNY
#include "./Curves/Tr2GrannyVectorTrack.h"
#endif

class Tr2GrannyAnimation;

#if WITH_GRANNY
struct TextEventTrack
{
	TextEventTrack( granny_text_track* grannyTrack ) :
		m_grannyTrack( grannyTrack ), m_lastIndex( -1 ), m_lastLoop( -1 )
	{
	}

	const char* SampleTrack( float time, int loop );

private:
	int m_lastIndex;
	int m_lastLoop;

	granny_text_track* m_grannyTrack;
};
#endif

struct MorphTrack
{
#if WITH_GRANNY
	MorphTrack( granny_vector_track* grannyTrack ) :
		m_grannyTrack( grannyTrack )
	{
	}
#endif
	MorphTrack( const cmf::AnimationChannel* cmfChannel, const cmf::AnimationCurve* cmfCurve ) :
		m_cmfChannel( cmfChannel ),
		m_cmfCurve( cmfCurve ),
		m_cmfTargetName( cmf::ToStdString( cmfChannel->target ) )
	{
	}

	float SampleTrack( float time, float duration ) const;

#if WITH_GRANNY
	granny_vector_track* m_grannyTrack = nullptr;
#endif
	const cmf::AnimationChannel* m_cmfChannel = nullptr;
	const cmf::AnimationCurve* m_cmfCurve = nullptr;
	std::string m_cmfTargetName;
};

class Tr2GrannyAnimationLayer
{
public:
	Tr2GrannyAnimationLayer();
	Tr2GrannyAnimationLayer( float defaultBoneWeight );
	Tr2GrannyAnimationLayer( float defaultBoneWeight, float layerWeight );

	bool PlayAnimation( const Tr2GrannyAnimation* grannyAnimation, const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone );
	void QueueAnimation( const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone );
	void EndAnimation();
	void ClearAnimations();
	float GetAnimationChainCompleteTime();
	float GetAnimationRemainingTime();
	void InitializeAnimationLayer( const Tr2GrannyAnimation* grannyAnimation );
	void ConsumeAnimationQueue( const Tr2GrannyAnimation* grannyAnimation );
	void Cleanup();

#if WITH_GRANNY
	void SampleAnimation( float animationTime, granny_local_pose* resultPose, IBlueEventListener* listener, std::unordered_map<std::string, float>& morphAnimations );
	void SampleAnimation( float animationTime, granny_local_pose* compositePose, granny_local_pose* resultPose, IBlueEventListener* listener, std::unordered_map<std::string, float>& morphAnimations, bool additive = false );
#endif
	void SampleAnimation( float animationTime, cmf::SkeletonPose* resultPose, IBlueEventListener* listener, std::unordered_map<std::string, float>& morphAnimations );
	void SampleAnimation( float animationTime, cmf::SkeletonPose* compositePose, cmf::SkeletonPose* resultPose, IBlueEventListener* listener, std::unordered_map<std::string, float>& morphAnimations, bool additive = false );
	void AddBone( const Tr2GrannyAnimation* grannyAnimation, const char* name );
	void AddAllBones( const Tr2GrannyAnimation* grannyAnimation );
	void RemoveBone( const Tr2GrannyAnimation* grannyAnimation, const char* name );
	void ExtractTrackMask( const Tr2GrannyAnimation* grannyAnimation, const char* name );

	float GetLayerWeight() const;
	void SetLayerWeight( float layerWeight );
	void SetControlParam( float fixedTime );
	void SetControlParamSkewRate( float skewRate );

	void TogglePauseAnimation( bool pause );

	std::string m_name;

#if WITH_GRANNY
	granny_model_instance* m_modelInstance;
#endif
	std::unique_ptr<cmf::AnimationSequencer> m_sequencer;

private:
	struct AnimationRequest
	{
		std::string m_animationName;
		bool m_replace;
		bool m_clearWhenDone;
		int m_loopCount;
		float m_start;
		float m_speed;
	};
	std::vector<AnimationRequest> m_animationQueue;
#if WITH_GRANNY
	std::map<granny_control*, std::vector<TextEventTrack>> m_controlTextTracks;
	std::map<granny_control*, std::vector<MorphTrack>> m_controlMorphTracks;
	void ClearTextTracks( granny_control* control );
	void ClearMorphTracks( granny_control* control );
	void RegisterTextTracks( granny_control* control, const granny_animation* animation );
	void RegisterMorphTracks( granny_control* control, const granny_animation* animation );
#endif
	bool m_useCMF;
	std::map<cmf::AnimationPlayer*, std::vector<MorphTrack>> m_morphTracks;
	bool IsUsingCMF() const;
	void ClearMorphTracks( cmf::AnimationPlayer* player );
	void RegisterMorphTracks( cmf::AnimationPlayer* player, const cmf::Animation* animation );

#if WITH_GRANNY
	void SampleTextTracks( IBlueEventListener* listener );
#endif

	void SampleMorphTracks( std::unordered_map<std::string, float>& morphAnimations, bool additive = false );
	void UpdateControlParam( float control_increment );
	float GetLayerAnimationTime();

	void FreeCompletedControls();
#if WITH_GRANNY
	granny_track_mask* m_trackMask;
#endif
	std::vector<std::string> m_bones;

#if WITH_GRANNY
	int m_boneCount = 0;
#endif
	float m_defaultBoneWeight;
	float m_layerWeight;
	float m_controlParam;
	float m_controlParamTarget;
	bool m_controlParamEnabled;
	float m_lastControlUpdateTime;
#if WITH_GRANNY
	granny_local_pose* m_basePose;
#endif
	cmf::SkeletonPose m_cmfBasePose;
	float m_skewRate;

	float m_pauseTime;
	bool m_paused;
	float m_totalPauseOffset;

	const char* m_trackMaskName;

	cmf::BoneWeights m_boneMask;
};

#endif //Tr2GrannyAnimationLayer_h
