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
 * source's up and down bob, scaled by the ratio of the two pelvis heights, then raised or lowered so that over the
 * cycle the feet (the mapped bones lowest in the target bind pose) come down exactly to where the sampled pose
 * holds them; the clips play in place.
 *
 * A second clip of the same rig (blendSourcePath) can be blended in by blend, for example a run over a walk.  Both
 * clips play at one shared phase, 0 to 1 over a cycle, each shifted by its own phase offset so their steps line up.
 * The result is blended over the sampled pose by weight.
 *
 * The bind poses come from their own files because a clip's skeleton and the animation's skeleton may hold a frame of
 * animation rather than the pose the meshes were bound in.
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

	const std::string& GetSourcePath() const;
	void SetSourcePath( const std::string& path );
	const std::string& GetBlendSourcePath() const;
	void SetBlendSourcePath( const std::string& path );
	const std::string& GetSourceBindPosePath() const;
	void SetSourceBindPosePath( const std::string& path );
	const std::string& GetTargetBindPosePath() const;
	void SetTargetBindPosePath( const std::string& path );

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

	struct Clip
	{
		const cmf::Skeleton* skeleton = nullptr;
		std::unique_ptr<cmf::AnimationPlayer> player;
		float duration = 0.f;
		std::vector<uint32_t> sourceBones;	// per link, the bone in this clip's skeleton
		uint32_t pelvisSource = 0xFFFFFFFF;
		float groundOffset = 0.f;	// pelvis height change that puts the feet on the ground
		cmf::SkeletonPose sourcePose;
		std::vector<cmf::Transform> sourceWorld;
	};

	void LoadRes( const std::string& path, TriGrannyResPtr& res );
	bool Bind( const cmf::Skeleton& target );
	void Unbind();
	void UpdateClock();
	void Retarget( uint32_t clipIndex, float phase, const cmf::Skeleton& skeleton, const cmf::SkeletonPose& base, cmf::SkeletonPose& out );
	void CalibrateGround( const cmf::Skeleton& skeleton, const cmf::SkeletonPose& base );

	Tr2GrannyAnimationPtr m_animation;

	std::string m_sourcePath;
	std::string m_blendSourcePath;
	std::string m_sourceBindPosePath;
	std::string m_targetBindPosePath;
	TriGrannyResPtr m_sourceRes;
	TriGrannyResPtr m_blendSourceRes;
	TriGrannyResPtr m_sourceBindPoseRes;
	TriGrannyResPtr m_targetBindPoseRes;

	std::vector<BoneMapping> m_boneMap;

	float m_weight;
	float m_speed;
	float m_blend;
	float m_sourcePhaseOffset;
	float m_blendSourcePhaseOffset;

	float m_phase;	// 0 to 1 over a cycle
	float m_lastTime;
	bool m_clockStarted;

	// Bound state, rebuilt when a resource, the bone map or the target skeleton changes
	bool m_needsBind;
	bool m_groundCalibrated;
	const cmf::Skeleton* m_boundTarget;
	size_t m_boundTargetBoneCount;
	Clip m_clips[2];
	uint32_t m_clipCount;
	std::vector<BoneLink> m_links;
	std::vector<int32_t> m_linkForTargetBone;	// index into m_links, -1 for unmapped bones
	std::vector<uint32_t> m_feet;	// target bones that touch the ground
	uint32_t m_pelvisTarget;
	float m_pelvisSourceBindHeight;
	Vector3 m_pelvisTargetBindPosition;
	float m_pelvisHeightScale;

	cmf::SkeletonPose m_retargetPose;
	cmf::SkeletonPose m_blendPose;
	std::vector<cmf::Transform> m_targetWorld;
};

TYPEDEF_BLUECLASS( Tr2AnimationRetargeter );

#endif
