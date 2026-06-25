// Copyright © 2023 CCP ehf.

#pragma once
#ifndef ITR2SKINNEDOBJECT_H
#define ITR2SKINNEDOBJECT_H

BLUE_INTERFACE( ITr2SkinnedObject ) :
	public IRoot
{
	virtual const Vector3 GetScaling() const = 0;
	virtual const Quaternion GetRotation() const = 0;
	virtual const Vector3& GetPosition() const = 0;
};

#endif // TR2SKINNEDOBJECT_H