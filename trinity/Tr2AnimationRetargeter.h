// Copyright © 2026 CCP ehf.

#pragma once
#ifndef Tr2AnimationRetargeter_h
#define Tr2AnimationRetargeter_h

#include "Include/ITr2PoseModifier.h"

BLUE_DECLARE( TriGrannyRes );
BLUE_DECLARE( Tr2GrannyAnimation );
BLUE_DECLARE( Tr2AnimationRetargeter );

/**
 * @class Tr2AnimationRetargeter
 * @brief Plays clips authored for one skeleton on another, on top of whatever a Tr2GrannyAnimation already plays.
 *
 * Bones are paired by name with MapBone.  Each mapped target bone takes the world-space rotation its source bone has
 * moved through since the source bind pose, after a fixed correction that lines up the bone directions of the two
 * bind poses.  Bones paired with MapBoneFromBind skip that correction: both rigs hold them the same way in their bind
 * poses (feet standing flat), so each keeps its own shape.  The topmost mapped bone (the pelvis) also takes the
 * source's up and down bob, scaled by the ratio of the two pelvis heights above the feet (the mapped bones lowest in
 * each bind pose), then raised or lowered so that over each clip the target's feet come down exactly to where the
 * sampled pose holds them; the clips play in place.
 *
 * Clips of the source rig are added by name with AddClip, and driven from outside: each has a weight and a phase, 0 to
 * 1 through the clip, set every frame.  The clips with weight are blended by their weights, and their total weight,
 * up to 1, is how much of them replaces the sampled pose.
 *
 * The target bind pose comes from its own file because the animation's skeleton may hold a frame of animation rather
 * than the pose the meshes were bound in.  The source bind pose may come from its own file too; left empty, it is the
 * first clip's skeleton, for clips whose rest pose is the bind pose.
 */
BLUE_CLASS( Tr2AnimationRetargeter ) :
	public IRoot,
	public ITr2PoseModifier,
	public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();

	Tr2AnimationRetargeter( IRoot* lockobj = NULL );
	~Tr2AnimationRetargeter();

	Tr2GrannyAnimation* GetAnimation() const;
	void SetAnimation( Tr2GrannyAnimation* animation );

	const std::string& GetSourceBindPosePath() const;
	void SetSourceBindPosePath( const std::string& path );
	const std::string& GetTargetBindPosePath() const;
	void SetTargetBindPosePath( const std::string& path );

	void AddClip( const std::string& name, const std::string& path, bool looping );
	void ClearClips();
	bool SetClipWeight( const std::string& name, float weight );
	bool SetClipPhase( const std::string& name, float phase );

	void MapBone( const std::string& sourceBone, const std::string& targetBone );
	void MapBoneFromBind( const std::string& sourceBone, const std::string& targetBone );
	void ClearBoneMap();

	bool IsReady() const;

	// ITr2PoseModifier
	void ModifyPose( const cmf::Skeleton& skeleton, cmf::SkeletonPose& pose ) override;

	// IBlueAsyncResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* res ) override;
	void RebuildCachedData( BlueAsyncRes* res ) override;

private:
	static constexpr uint32_t NO_BONE = 0xFFFFFFFF;	// a bone index that names no bone

	struct BoneMapping
	{
		std::string source;
		std::string target;
		bool fromBind;
	};

	struct BoneLink
	{
		Quaternion offset;	// takes the source bone's animated world rotation to the target bone's
	};

	// A clip of the source rig: its file and how it plays, and what is built from it while bound
	struct Clip
	{
		std::string name;
		std::string path;
		TriGrannyResPtr res;
		bool looping = true;	// the phase wraps round; otherwise it stops at the end
		float weight = 0.f;
		float phase = 0.f;	// 0 to 1 through the clip

		const cmf::Skeleton* skeleton = nullptr;
		std::unique_ptr<cmf::AnimationPlayer> player;
		float duration = 0.f;
		std::vector<uint32_t> sourceBones;	// per link, the bone in this clip's skeleton
		uint32_t pelvisSource = NO_BONE;
		float groundOffset = 0.f;	// pelvis height change that puts the feet on the ground
		cmf::SkeletonPose sourcePose;
		std::vector<cmf::Transform> sourceWorld;
	};

	static uint32_t FindBone( const cmf::Skeleton& skeleton, const std::string& name );

	Clip* FindClip( const std::string& name );
	void LoadRes( const std::string& path, TriGrannyResPtr& res );
	void ReleaseRes( TriGrannyResPtr& res );
	uint32_t CountSlotsHolding( const TriGrannyRes* res ) const;
	bool Bind( const cmf::Skeleton& target );
	void Unbind();
	void Retarget( Clip& clip, float phase, const cmf::Skeleton& skeleton, const cmf::SkeletonPose& base, cmf::SkeletonPose& out );
	void CalibrateGround( const cmf::Skeleton& skeleton, const cmf::SkeletonPose& base );

	Tr2GrannyAnimationPtr m_animation;

	std::vector<Clip> m_clips;
	std::string m_sourceBindPosePath;
	std::string m_targetBindPosePath;
	TriGrannyResPtr m_sourceBindPoseRes;
	TriGrannyResPtr m_targetBindPoseRes;

	std::vector<BoneMapping> m_boneMap;

	// Bound state, rebuilt when a file, a clip, the bone map or the animation's skeleton changes
	bool m_needsBind = true;
	bool m_groundCalibrated = false;
	bool m_bound = false;
	size_t m_boundTargetBoneCount = 0;	// bone count of the skeleton last bound to, whether or not binding succeeded
	std::vector<BoneLink> m_links;
	std::vector<int32_t> m_linkForTargetBone;	// index into m_links, -1 for unmapped bones
	std::vector<uint32_t> m_feet;	// target bones that touch the ground
	uint32_t m_pelvisTarget = NO_BONE;
	float m_pelvisSourceBindHeight = 0.f;
	Vector3 m_pelvisTargetBindPosition = Vector3( 0.f, 0.f, 0.f );
	float m_pelvisHeightScale = 1.f;

	cmf::SkeletonPose m_retargetPose;
	cmf::SkeletonPose m_blendPose;
	std::vector<cmf::Transform> m_targetWorld;
};

TYPEDEF_BLUECLASS( Tr2AnimationRetargeter );

#endif
