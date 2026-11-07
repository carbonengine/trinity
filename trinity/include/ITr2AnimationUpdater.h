// Copyright © 2023 CCP ehf.

#pragma once
#ifndef ITr2AnimationUpdater_H
#define ITr2AnimationUpdater_H

BLUE_INTERFACE( ITr2AnimationUpdater ) :
	public IRoot
{
	virtual void PrePhysicsAnimation( Be::Time time, const Matrix& modelTransform ) = 0;
	virtual void PostPhysicsAnimation( Be::Time time, const Matrix& modelTransform ) = 0;

	virtual const Matrix* GetAnimationTransforms() = 0;
	virtual const std::string* GetAnimationBoneList( unsigned int& numBones ) const = 0;
};


#endif // ITr2AnimationUpdater_H