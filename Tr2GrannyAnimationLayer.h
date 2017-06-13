#pragma once
#ifndef Tr2GrannyAnimationLayer_h
#define Tr2GrannyAnimationLayer_h

#include "granny.h"
#include "Include/ITr2AnimationUpdater.h"
#include "GrannyBoneOffset.h"
#include "blue/include/BlueAsyncRes.h"

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
	
	void SampleAnimation( float animationTime, granny_local_pose* resultPose, IBlueEventListener* listener );
	void SampleAnimation( float animationTime, granny_local_pose* compositePose, granny_local_pose* resultPose, IBlueEventListener* listener );
	void AddBone( const Tr2GrannyAnimation* grannyAnimation, const char* name );
	void RemoveBone( const Tr2GrannyAnimation* grannyAnimation, const char* name );
	void ExtractTrackMask( const Tr2GrannyAnimation* grannyAnimation, const char* name );

	float GetLayerWeight() const;
	void SetLayerWeight(float layerWeight);
	
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
	void ClearTextTracks( granny_control* control );
	void RegisterTextTracks( granny_control* control, const granny_animation* animation );
	void SampleTextTracks( IBlueEventListener* listener);

	void FreeCompletedControls();
	granny_track_mask* m_trackMask;
	std::vector<std::string> m_bones;

	int m_boneCount;
	float m_defaultBoneWeight;
	float m_layerWeight;

	const char* m_trackMaskName;
};

#endif //Tr2GrannyAnimationLayer_h
