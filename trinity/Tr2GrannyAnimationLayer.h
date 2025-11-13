#pragma once
#ifndef Tr2GrannyAnimationLayer_h
#define Tr2GrannyAnimationLayer_h

#include "granny.h"
#include "Include/ITr2AnimationUpdater.h"
#include "GrannyBoneOffset.h"
#include <BlueAsyncRes.h>

#include "./Curves/Tr2GrannyVectorTrack.h"

class Tr2GrannyAnimation;

struct TextEventTrack
{
	TextEventTrack( granny_text_track* grannyTrack ) : m_grannyTrack( grannyTrack ), m_lastIndex(-1), m_lastLoop(-1) {}

	const char* SampleTrack( float time, int loop );

private:
	int m_lastIndex;
	int m_lastLoop;
	granny_text_track* m_grannyTrack;
};

struct MorphTrack
{
	MorphTrack( granny_vector_track* grannyTrack ) : m_grannyTrack( grannyTrack ) {}

	const float SampleTrack( float time, float duration );

	granny_vector_track* m_grannyTrack;
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
	
	void SampleAnimation( float animationTime, granny_local_pose* resultPose, IBlueEventListener* listener, std::unordered_map<std::string, float>& morphAnimations );
	void SampleAnimation( float animationTime, granny_local_pose* compositePose, granny_local_pose* resultPose, IBlueEventListener* listener, std::unordered_map<std::string, float>& morphAnimations, bool additive = false );
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
	
	granny_model_instance* m_modelInstance;
	
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
	std::map<granny_control*, std::vector<TextEventTrack>> m_controlTextTracks;
	std::map<granny_control*, std::vector<MorphTrack>> m_controlMorphTracks;
	void ClearTextTracks( granny_control* control );
	void ClearMorphTracks( granny_control* control );
	void RegisterTextTracks( granny_control* control, const granny_animation* animation );
	void RegisterMorphTracks( granny_control* control, const granny_animation* animation );
	void SampleTextTracks( IBlueEventListener* listener);
	void SampleMorphTracks( float animationTime, std::unordered_map<std::string, float>& morphAnimations, bool additive = false );
	void UpdateControlParam( float control_increment );
	float GetLayerAnimationTime();

	void FreeCompletedControls();
	granny_track_mask* m_trackMask;
	std::vector<std::string> m_bones;

	int m_boneCount;
	float m_defaultBoneWeight;
	float m_layerWeight;
	float m_controlParam;
	float m_controlParamTarget;
	bool m_controlParamEnabled;
	float m_lastControlUpdateTime;
	granny_local_pose *m_basePose;
	float m_skewRate;

	float m_pauseTime;
	bool m_paused;
	float m_totalPauseOffset;

	const char* m_trackMaskName;
};

#endif //Tr2GrannyAnimationLayer_h
