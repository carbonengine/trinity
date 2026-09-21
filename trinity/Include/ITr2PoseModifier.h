// Copyright © 2026 Fenris Creations ehf.

#pragma once

#ifndef ITR2POSEMODIFIER_H
#define ITR2POSEMODIFIER_H

#include <cmf/animation.h>

class ITr2PoseModifier
{
public:
	virtual void ModifyPose( const cmf::Skeleton& skeleton, cmf::SkeletonPose& pose ) = 0;

protected:
	~ITr2PoseModifier() = default;
};

#endif //ITR2POSEMODIFIER_H
