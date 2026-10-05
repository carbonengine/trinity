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
 * @brief Plays a clip authored for one skeleton on another, on top of whatever a Tr2GrannyAnimation already plays.
 *
 * Bones are paired by name with MapBone.  Each mapped target bone takes the world-space rotation its source bone has
 * moved through since the source bind pose, after a fixed correction that lines up the bone directions of the two
 * bind poses.  The topmost mapped bone (the pelvis) also takes the source's up and down bob, scaled by the ratio of
 * the two pelvis heights; the clip plays in place.  The result is blended over the sampled pose by weight.
 *
 * The bind poses come from their own files because the clip's skeleton and the animation's skeleton usually hold a
 * frame of animation rather than the pose the meshes were bound in.
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
	const std::string& GetSourceBindPosePath() const;
	void SetSourceBindPosePath( const std::string& path );
	const std::string& GetTargetBindPosePath() const;
	void SetTargetBindPosePath( const std::string& path );

	void MapBone( const std::string& sourceBone, const std::string& targetBone );
	void ClearBoneMap();

	bool IsReady() const;

	// ITr2PoseModifier
	void ModifyPose( const cmf::Skeleton& skeleton, cmf::SkeletonPose& pose ) override;

	// IBlueAsyncResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* res ) override;
	void RebuildCachedData( BlueAsyncRes* res ) override;

private:
	struct BoneLink
	{
		uint32_t source;	// bone in the clip's skeleton
		Quaternion offset;	// takes the source bone's animated world rotation to the target bone's
	};

	void LoadRes( const std::string& path, TriGrannyResPtr& res );
	bool Bind( const cmf::Skeleton& target );
	void Unbind();
	void UpdateClock();

	Tr2GrannyAnimationPtr m_animation;

	std::string m_sourcePath;
	std::string m_sourceBindPosePath;
	std::string m_targetBindPosePath;
	TriGrannyResPtr m_sourceRes;
	TriGrannyResPtr m_sourceBindPoseRes;
	TriGrannyResPtr m_targetBindPoseRes;

	std::vector<std::pair<std::string, std::string>> m_boneMap;

	float m_weight;
	float m_speed;

	float m_phase;
	float m_lastTime;
	bool m_clockStarted;

	// Bound state, rebuilt when a resource, the bone map or the target skeleton changes
	bool m_needsBind;
	const cmf::Skeleton* m_boundTarget;
	size_t m_boundTargetBoneCount;
	const cmf::Skeleton* m_clipSkeleton;
	std::unique_ptr<cmf::AnimationPlayer> m_player;
	float m_duration;
	std::vector<BoneLink> m_links;
	std::vector<int32_t> m_linkForTargetBone;	// index into m_links, -1 for unmapped bones
	uint32_t m_pelvisTarget;
	uint32_t m_pelvisSource;
	float m_pelvisSourceBindHeight;
	Vector3 m_pelvisTargetBindPosition;
	float m_pelvisHeightScale;

	cmf::SkeletonPose m_sourcePose;
	cmf::SkeletonPose m_retargetPose;
	std::vector<cmf::Transform> m_sourceWorld;
	std::vector<cmf::Transform> m_targetWorld;
};

TYPEDEF_BLUECLASS( Tr2AnimationRetargeter );

#endif
